#ifndef SERVICES_STATUS_SERVICE_H
#define SERVICES_STATUS_SERVICE_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Status
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

    void start();

    QJsonObject appStatus() const;
    QJsonObject backendStatus() const;

    void setApplicationStatus(const QJsonObject& params);

  private:
    QElapsedTimer m_uptime;

    QJsonObject m_appStatus;
};

} // namespace Services::Status

#endif // SERVICES_STATUS_SERVICE_H
