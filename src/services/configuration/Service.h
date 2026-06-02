#ifndef SERVICES_CONFIGURATION_SERVICE_H
#define SERVICES_CONFIGURATION_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include "services/configuration/DeviceConfiguration.h"

namespace Drivers::Hardware
{
class BrightnessDriver;
class VolumeDriver;
}

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
    explicit Service(Drivers::Hardware::BrightnessDriver& brightness,
                     Drivers::Hardware::VolumeDriver& volume,
                     const QString& dataDir,
                     Services::WebSocket::Service* websocket = nullptr,
                     QObject* parent = nullptr);

    bool load();
    bool save();

    QJsonObject asJson() const;
    int volume() const;

    QJsonObject setBrightness(int value);
    QJsonObject setVolume(int value);
    QJsonObject setDeviceId(const QString& deviceId);
    QJsonObject updateSystemConfig(const QJsonObject& params);
    QJsonObject addApp(const QJsonObject& app);
    QJsonObject updateApp(const QJsonObject& app);
    QJsonObject removeApp(const QString& appId);

  private:
    QString dataPath(const QString& relative) const;

    Drivers::Hardware::BrightnessDriver& m_brightness;
    Drivers::Hardware::VolumeDriver& m_volume;
    QString m_dataDir;
    Services::Configuration::DeviceConfiguration m_configuration;
};

} // namespace Services::Configuration

#endif //SERVICES_CONFIGURATION_SERVICE_H
