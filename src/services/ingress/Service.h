#ifndef SERVICES_INGRESS_SERVICE_H
#define SERVICES_INGRESS_SERVICE_H

#include <QHash>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTcpSocket)

namespace Common::Communication::Rest::Server
{
class Service;
}

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Ingress
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::Rest::Server::Service& rest,
                     Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

    bool start(quint16 port = 5000);

  private slots:
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();

  private:
    void processBufferedRequest(QTcpSocket* socket);
    static QHash<QByteArray, QByteArray> parseHeaders(const QList<QByteArray>& headerLines);

    Common::Communication::Rest::Server::Service& m_rest;
    Common::Communication::WebSocket::Server::Service& m_websocket;
    QTcpServer* m_server;
};

} // namespace Services::Ingress

#endif // SERVICES_INGRESS_SERVICE_H
