#ifndef SERVICES_STATUS_SERVICE_H
#define SERVICES_STATUS_SERVICE_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>

namespace Drivers::Hardware
{
class EnvironmentDriver;
class TemperatureDriver;
}

namespace Services::WebSocket
{
class Service;
}

namespace Services::Status
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Drivers::Hardware::EnvironmentDriver& environment,
                     Drivers::Hardware::TemperatureDriver& temperature,
                     const QString& dataDir,
                     Services::WebSocket::Service* websocket = nullptr,
                     QObject* parent = nullptr);

    void start();

    QJsonObject appStatus() const;
    QJsonObject environment() const;
    QJsonObject backendStatus() const;
    QJsonObject processorTemperature() const;

    void setApplicationStatus(const QJsonObject& params);

  private:
    Drivers::Hardware::EnvironmentDriver& m_environmentDriver;
    Drivers::Hardware::TemperatureDriver& m_temperatureDriver;
    QString m_dataDir;
    QElapsedTimer m_uptime;

    QJsonObject m_appStatus;
    QJsonObject m_environment;
};

} // namespace Services::Status

#endif //SERVICES_STATUS_SERVICE_H
