#include "Service.h"

#include <QLoggingCategory>
#include <QTcpServer>
#include <QTcpSocket>

#include "services/rest/Service.h"
#include "services/websocket/Service.h"

Q_LOGGING_CATEGORY(BackendQtIngressService, "BackendQtIngressService")

namespace Services::Ingress
{

Service::Service(Services::Rest::Service& rest,
                 Services::WebSocket::Service& websocket,
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
        qCCritical(BackendQtIngressService) << "Failed to listen on ingress port" << port << ":" << m_server->errorString();
        return false;
    }

    qCInfo(BackendQtIngressService) << "clock-backend (Qt) ingress listening on 0.0.0.0:" << port;
    return true;
}

void Service::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        m_requestBuffers.insert(socket, QByteArray());
        connect(socket, &QTcpSocket::readyRead, this, &Service::onSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &Service::onSocketDisconnected);
    }
}

void Service::onSocketReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    m_requestBuffers[socket].append(socket->readAll());
    processBufferedRequest(socket);
}

void Service::onSocketDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    m_requestBuffers.remove(socket);
    socket->deleteLater();
}

void Service::processBufferedRequest(QTcpSocket* socket)
{
    if (!m_requestBuffers.contains(socket)) {
        return;
    }

    const QByteArray buffer = m_requestBuffers.value(socket);
    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QByteArray headerBlob = buffer.left(headerEnd);
    const QList<QByteArray> lines = headerBlob.split('\n');
    if (lines.isEmpty()) {
        m_requestBuffers.remove(socket);
        socket->disconnectFromHost();
        return;
    }

    const QByteArray requestLine = lines.first().trimmed();
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 3) {
        m_requestBuffers.remove(socket);
        socket->disconnectFromHost();
        return;
    }

    const QByteArray method = requestParts.at(0).trimmed();
    const QString path = QString::fromUtf8(requestParts.at(1).trimmed());
    const QHash<QByteArray, QByteArray> headers = parseHeaders(lines.mid(1));

    const QByteArray upgradeHeader = headers.value("upgrade").toLower();
    if (method == "GET" && path == QStringLiteral("/ws") && upgradeHeader == "websocket") {
        m_requestBuffers.remove(socket);
        disconnect(socket, nullptr, this, nullptr);
        m_websocket.attachUpgradedSocket(socket);
        return;
    }

    const int contentLength = headers.value("content-length").toInt();
    const int fullLength = headerEnd + 4 + contentLength;
    if (buffer.size() < fullLength) {
        return;
    }

    const QByteArray body = buffer.mid(headerEnd + 4, contentLength);
    m_requestBuffers.remove(socket);

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
