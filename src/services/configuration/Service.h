#ifndef SERVICES_CONFIGURATION_SERVICE_H
#define SERVICES_CONFIGURATION_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include "configuration/DeviceConfiguration.h"

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Configuration
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

    bool load();
    bool save();

    QJsonObject asJson() const;
    QJsonObject asSystemConfigJson() const;
    QJsonObject asApplicationListJson() const;
    QJsonObject asApplicationDetailJson(const QString& appId) const;
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
    Common::Communication::Configuration::DeviceConfiguration m_configuration;
};

} // namespace Services::Configuration

#endif // SERVICES_CONFIGURATION_SERVICE_H
