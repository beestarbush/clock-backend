#include "Service.h"

#include <QLoggingCategory>
#include <QTcpServer>
#include <QTcpSocket>

#include "rest/server/Service.h"
#include "websocket/server/Service.h"

Q_LOGGING_CATEGORY(IngressService, "IngressService")

namespace Services::Ingress
{

Service::Service(Common::Communication::Rest::Server::Service& rest,
                 Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent),
      m_rest(rest),
      m_websocket(websocket),
      m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Service::onNewConnection);
}

bool Service::start(quint16 port)
{
    if (m_server->isListening()) {
        m_server->close();
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        qCCritical(IngressService) << "Failed to listen on ingress port" << port << ":" << m_server->errorString();
        return false;
    }

    qCInfo(IngressService) << QStringLiteral("Ingress listening on 0.0.0.0:%1 (REST: /media/*, /api/media; WebSocket: /ws)").arg(port);
    return true;
}

void Service::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        qCDebug(IngressService) << "Accepted TCP connection from" << socket->peerAddress().toString() << ":" << socket->peerPort()
                                << "state=" << socket->state() << "bytesAvailable=" << socket->bytesAvailable();
        connect(socket, &QTcpSocket::readyRead, this, &Service::onSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &Service::onSocketDisconnected);

        // Try to process immediately in case request bytes were already buffered before signal wiring.
        processBufferedRequest(socket);
    }
}

void Service::onSocketReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    qCDebug(IngressService) << "readyRead from" << socket->peerAddress().toString() << ":" << socket->peerPort()
                            << "bytesAvailable=" << socket->bytesAvailable();

    processBufferedRequest(socket);
}

void Service::onSocketDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    qCDebug(IngressService) << "Socket disconnected from" << socket->peerAddress().toString() << ":" << socket->peerPort();

    socket->deleteLater();
}

void Service::processBufferedRequest(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }

    // Guard against duplicate processing during websocket handover races.
    if (socket->property("ingress.websocketHandoverComplete").toBool()) {
        return;
    }

    // Important: for websocket upgrades we must NOT consume the HTTP upgrade bytes here.
    // QWebSocketServer::handleConnection expects to parse the original handshake request
    // from the socket. If ingress reads those bytes first, websocket connection setup fails.
    // We therefore inspect the full currently buffered request via peek() and only read()
    // for normal REST requests.
    const qint64 bufferedBytes = socket->bytesAvailable();
    if (bufferedBytes <= 0) {
        return;
    }

    const QByteArray buffer = socket->peek(bufferedBytes);
    const int headerEnd = buffer.indexOf("\r\n\r\n");
    qCDebug(IngressService) << "processBufferedRequest peer=" << socket->peerAddress().toString() << ":" << socket->peerPort()
                            << "peekSize=" << buffer.size() << "headerEnd=" << headerEnd;
    if (headerEnd < 0) {
        qCDebug(IngressService) << "Waiting for full HTTP headers from" << socket->peerAddress().toString() << ":" << socket->peerPort();
        return;
    }

    const QByteArray headerBlob = buffer.left(headerEnd);
    const QList<QByteArray> lines = headerBlob.split('\n');
    if (lines.isEmpty()) {
        socket->disconnectFromHost();
        return;
    }

    const QByteArray requestLine = lines.first().trimmed();
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 3) {
        qCWarning(IngressService) << "Invalid HTTP request line from" << socket->peerAddress().toString() << ":" << socket->peerPort()
                                  << "requestLine=" << requestLine;
        socket->disconnectFromHost();
        return;
    }

    const QByteArray method = requestParts.at(0).trimmed();
    const QString path = QString::fromUtf8(requestParts.at(1).trimmed());
    const QHash<QByteArray, QByteArray> headers = parseHeaders(lines.mid(1));
    qCDebug(IngressService) << "Parsed HTTP request method=" << method << "path=" << path
                            << "from" << socket->peerAddress().toString() << ":" << socket->peerPort();

    const QByteArray upgradeHeader = headers.value("upgrade").toLower();
    const QByteArray connectionHeader = headers.value("connection").toLower();
    const bool websocketUpgradeRequested =
        method == "GET" && path == QStringLiteral("/ws") && upgradeHeader.contains("websocket") && connectionHeader.contains("upgrade");

    if (websocketUpgradeRequested) {
        // Hand off the untouched socket to websocket service so Qt can complete the handshake.
        socket->setProperty("ingress.websocketHandoverComplete", true);
        qCInfo(IngressService) << "WebSocket upgrade request accepted from" << socket->peerAddress().toString() << ":" << socket->peerPort();
        disconnect(socket, nullptr, this, nullptr);
        m_websocket.attachUpgradedSocket(socket);
        return;
    }

    if (method == "GET" && path == QStringLiteral("/ws")) {
        qCWarning(IngressService) << "Rejected /ws request without valid websocket upgrade headers."
                                  << "Upgrade:" << upgradeHeader
                                  << "Connection:" << connectionHeader;
    }

    const int contentLength = headers.value("content-length").toInt();
    const int fullLength = headerEnd + 4 + contentLength;
    if (buffer.size() < fullLength) {
        qCDebug(IngressService) << "Waiting for full HTTP body from" << socket->peerAddress().toString() << ":" << socket->peerPort()
                                << "bufferSize=" << buffer.size() << "required=" << fullLength;
        return;
    }

    // For non-websocket HTTP traffic, consume the buffered request bytes and forward to REST.
    const QByteArray fullRequest = socket->read(fullLength);
    const QByteArray body = fullRequest.mid(headerEnd + 4, contentLength);

    qCDebug(IngressService) << "Forwarding REST request method=" << method << "path=" << path
                            << "bodyBytes=" << body.size() << "peer=" << socket->peerAddress().toString() << ":" << socket->peerPort();

    m_rest.handleHttpRequest(socket, method, path, headers, body);
}

QHash<QByteArray, QByteArray> Service::parseHeaders(const QList<QByteArray>& headerLines)
{
    QHash<QByteArray, QByteArray> headers;
    for (const QByteArray& line : headerLines) {
        const int sep = line.indexOf(':');
        if (sep <= 0) {
            continue;
        }
        const QByteArray key = line.left(sep).trimmed().toLower();
        const QByteArray value = line.mid(sep + 1).trimmed();
        headers.insert(key, value);
    }
    return headers;
}

} // namespace Services::Ingress
