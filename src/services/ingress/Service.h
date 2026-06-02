#ifndef SERVICES_INGRESS_SERVICE_H
#define SERVICES_INGRESS_SERVICE_H

#include <QHash>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTcpSocket)

namespace Services::Rest
{
class Service;
}

namespace Services::WebSocket
{
class Service;
}

namespace Services::Ingress
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Services::Rest::Service& rest,
                     Services::WebSocket::Service& websocket,
                     QObject* parent = nullptr);

    bool start(quint16 port = 5000);

  private slots:
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();

  private:
    void processBufferedRequest(QTcpSocket* socket);
    static QHash<QByteArray, QByteArray> parseHeaders(const QList<QByteArray>& headerLines);

    Services::Rest::Service& m_rest;
    Services::WebSocket::Service& m_websocket;
    QTcpServer* m_server;
};

} // namespace Services::Ingress

#endif // SERVICES_INGRESS_SERVICE_H
