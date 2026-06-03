#ifndef SERVICES_ENVIRONMENT_SERVICE_H
#define SERVICES_ENVIRONMENT_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <optional>

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Environment
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

    void start();
    QJsonObject environment() const;

  private:
    std::optional<double> readDoubleFile(const QString& path) const;
    QJsonObject processorTemperature() const;
    QJsonObject refreshEnvironment();

    QJsonObject m_environment;
};

} // namespace Services::Environment

#endif // SERVICES_ENVIRONMENT_SERVICE_H
