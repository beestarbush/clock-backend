#include "Service.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QTcpSocket>
#include <QWebSocket>
#include <QWebSocketServer>

#include "services/websocket/Frame.h"

Q_LOGGING_CATEGORY(WebSocketService, "WebSocketService")

namespace Services::WebSocket
{

namespace
{
using namespace ::Services::WebSocket;

QJsonObject makeResultFrame(const QJsonValue& id, const QJsonObject& result)
{
    return Frame::buildResponse(id, result);
}

QJsonObject makeErrorFrame(const QJsonValue& id, int code, const QString& message)
{
    return Frame::buildErrorResponse(id, code, message);
}

} // namespace

Service::Service(QObject* parent)
    : QObject(parent),
      m_handoffServer(new QWebSocketServer(QStringLiteral("clock-backend-handoff"), QWebSocketServer::NonSecureMode, this))
{
    connect(m_handoffServer, &QWebSocketServer::newConnection, this, &Service::onNewConnection);
    qCInfo(WebSocketService) << "WebSocket upgrade handler ready at GET /ws (via ingress)";
}

Service::~Service()
{
    for (QWebSocket* socket : m_clients) {
        socket->close();
        socket->deleteLater();
    }
    m_clients.clear();
    m_subscriptions.clear();

    for (QWebSocketServer* server : m_servers) {
        if (!server) {
            continue;
        }
        server->close();
        server->deleteLater();
    }
    m_servers.clear();
}

bool Service::start(quint16 port)
{
    return start(QList<quint16>{port});
}

bool Service::start(const QList<quint16>& ports)
{
    if (ports.isEmpty()) {
        qCCritical(WebSocketService) << "No websocket ports configured";
        return false;
    }

    for (QWebSocketServer* server : m_servers) {
        if (!server) {
            continue;
        }
        server->close();
        server->deleteLater();
    }
    m_servers.clear();

    for (quint16 port : ports) {
        auto* server = new QWebSocketServer(QStringLiteral("clock-backend"), QWebSocketServer::NonSecureMode, this);
        connect(server, &QWebSocketServer::newConnection, this, &Service::onNewConnection);

        if (!server->listen(QHostAddress::Any, port)) {
            qCCritical(WebSocketService) << "Failed to listen on port" << port << ":" << server->errorString();
            server->deleteLater();
            for (QWebSocketServer* opened : m_servers) {
                opened->close();
                opened->deleteLater();
            }
            m_servers.clear();
            return false;
        }

        m_servers.append(server);
        qCInfo(WebSocketService) << QStringLiteral("WebSocket listening on ws://127.0.0.1:%1").arg(port);
    }

    return true;
}

void Service::attachUpgradedSocket(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }

    qCInfo(WebSocketService) << "Handling upgraded TCP socket from" << socket->peerAddress().toString() << ":" << socket->peerPort();
    m_handoffServer->handleConnection(socket);
}

void Service::registerMethodHandler(::Services::WebSocket::Method method, MethodHandler handler)
{
    m_methodHandlers.insert(static_cast<int>(method), std::move(handler));
}

void Service::registerPublishHandler(::Services::WebSocket::Topic topic, PublishHandler handler)
{
    m_publishHandlers.insert(static_cast<int>(topic), std::move(handler));
}

void Service::registerPeriodicPublisher(::Services::WebSocket::Topic topic, int intervalMs, TopicPublisher publisher)
{
    const int key = static_cast<int>(topic);

    if (m_periodicPublishers.contains(key)) {
        auto existing = m_periodicPublishers.take(key);
        if (existing.timer) {
            existing.timer->stop();
            existing.timer->deleteLater();
        }
    }

    auto* timer = new QTimer(this);
    timer->setInterval(intervalMs);
    connect(timer, &QTimer::timeout, this, [this, topic]() {
        const auto it = m_periodicPublishers.constFind(static_cast<int>(topic));
        if (it == m_periodicPublishers.cend() || !it->publisher) {
            return;
        }

        publishToSubscribed(topic, it->publisher());
    });

    m_periodicPublishers.insert(key, PeriodicPublisherRegistration{std::move(publisher), timer});
    timer->start();
}

void Service::publish(::Services::WebSocket::Topic topic, const QJsonObject& params)
{
    publishToSubscribed(topic, params);
}

QJsonObject Service::processRequestForTest(const QString& id, ::Services::WebSocket::Method method, const QJsonObject& params)
{
    return processRequest(id, method, params, nullptr);
}

void Service::onNewConnection()
{
    auto* server = qobject_cast<QWebSocketServer*>(sender());
    if (!server) {
        return;
    }

    while (server->hasPendingConnections()) {
        QWebSocket* socket = server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        qCInfo(WebSocketService) << "WebSocket client connected from" << socket->peerAddress().toString() << ":" << socket->peerPort();

        m_clients.append(socket);
        m_subscriptions.insert(socket, {});

        connect(socket, &QWebSocket::textMessageReceived, this, &Service::onTextMessageReceived);
        connect(socket, &QWebSocket::disconnected, this, &Service::onSocketDisconnected);
    }
}

void Service::onTextMessageReceived(const QString& message)
{
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject msg = doc.object();

    if (Frame::isPublish(msg)) {
        handlePublish(msg);
        return;
    }

    bool looksLikeRequest = Frame::isRequest(msg);
    if (!looksLikeRequest) {
        looksLikeRequest = msg.contains("method");
    }
    if (!looksLikeRequest) {
        return;
    }

    const QJsonValue id = Frame::parseId(msg);
    if (!Frame::isValidId(id)) {
        sendJson(socket, makeErrorFrame(QJsonValue(QJsonValue::Null), -32600, QStringLiteral("Invalid request id")));
        return;
    }

    const ::Services::WebSocket::Method method = Frame::parseMethod(msg);
    const QJsonObject params = Frame::parseParams(msg);

    QSet<::Services::WebSocket::Topic>* subscriptions = m_subscriptions.contains(socket) ? &m_subscriptions[socket] : nullptr;
    sendJson(socket, processRequest(id, method, params, subscriptions));
}

void Service::onSocketDisconnected()
{
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        return;
    }

    qCInfo(WebSocketService) << "WebSocket client disconnected from" << socket->peerAddress().toString() << ":" << socket->peerPort();

    m_subscriptions.remove(socket);
    m_clients.removeAll(socket);
    socket->deleteLater();
}

QJsonObject Service::processRequest(const QJsonValue& id,
                                    ::Services::WebSocket::Method method,
                                    const QJsonObject& params,
                                    QSet<::Services::WebSocket::Topic>* subscriptions)
{
    using Method = ::Services::WebSocket::Method;
    using Topic = ::Services::WebSocket::Topic;

    switch (method) {
    case Method::Subscribe: {
        if (!subscriptions) {
            return makeErrorFrame(id, -32000, QStringLiteral("Subscription context missing"));
        }

        const Topic topic = topicFromString(params.value("topic").toString());
        if (topic == Topic::UnknownTopic) {
            return makeErrorFrame(id, -32000, QStringLiteral("Invalid topic"));
        }

        subscriptions->insert(topic);
        return makeResultFrame(id, QJsonObject{{"subscribed", topicToString(topic)}});
    }
    case Method::Unsubscribe: {
        if (!subscriptions) {
            return makeErrorFrame(id, -32000, QStringLiteral("Subscription context missing"));
        }

        const Topic topic = topicFromString(params.value("topic").toString());
        subscriptions->remove(topic);
        return makeResultFrame(id, QJsonObject{{"unsubscribed", topicToString(topic)}});
    }
    default:
        break;
    }

    const auto it = m_methodHandlers.constFind(static_cast<int>(method));
    if (it == m_methodHandlers.cend() || !it.value()) {
        return makeErrorFrame(id, -32601, QStringLiteral("Method not found"));
    }

    const MethodResult methodResult = it.value()(params);
    if (!methodResult.ok) {
        return makeErrorFrame(id,
                              methodResult.errorCode != 0 ? methodResult.errorCode : -32000,
                              methodResult.errorMessage.isEmpty() ? QStringLiteral("Request failed") : methodResult.errorMessage);
    }
    return makeResultFrame(id, methodResult.payload);
}

void Service::handlePublish(const QJsonObject& message)
{
    const ::Services::WebSocket::Topic topic = Frame::parseTopic(message);
    const auto it = m_publishHandlers.constFind(static_cast<int>(topic));
    if (it != m_publishHandlers.cend() && it.value()) {
        it.value()(Frame::parseParams(message));
    }
}

void Service::sendJson(QWebSocket* socket, const QJsonObject& message)
{
    if (!socket) {
        return;
    }
    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));
}

void Service::publishToSubscribed(::Services::WebSocket::Topic topic, const QJsonObject& params)
{
    const QJsonObject frame = Frame::buildPublish(topic, params);
    for (QWebSocket* client : m_clients) {
        if (!m_subscriptions.contains(client)) {
            continue;
        }
        if (m_subscriptions[client].contains(topic)) {
            sendJson(client, frame);
        }
    }
}

} // namespace Services::WebSocket
