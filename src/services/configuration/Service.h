#ifndef SERVICES_CONFIGURATION_SERVICE_H
#define SERVICES_CONFIGURATION_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include "services/configuration/DeviceConfiguration.h"

namespace Services::WebSocket
{
class Service;
}

namespace Services::Configuration
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Services::WebSocket::Service& websocket,
                     QObject* parent = nullptr);

    bool load();
    bool save();

    QJsonObject asJson() const;
    int brightness() const;
    int volume() const;

    QJsonObject setBrightness(quint8 value);
    QJsonObject setVolume(int value);
    QJsonObject setDeviceId(const QString& deviceId);
    QJsonObject updateSystemConfig(const QJsonObject& params);
    QJsonObject addApp(const QJsonObject& app);
    QJsonObject updateApp(const QJsonObject& app);
    QJsonObject removeApp(const QString& appId);

  private:
    Services::Configuration::DeviceConfiguration m_configuration;
};

} // namespace Services::Configuration

#endif // SERVICES_CONFIGURATION_SERVICE_H
