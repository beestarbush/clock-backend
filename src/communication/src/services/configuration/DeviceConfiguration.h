#ifndef SERVICES_CONFIGURATION_DEVICECONFIGURATION_H
#define SERVICES_CONFIGURATION_DEVICECONFIGURATION_H

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace Services::Configuration
{

class DeviceConfiguration
{
  public:
    DeviceConfiguration();

    QString version;
    QDateTime lastModified;
    QString deviceId;
    QString activeAppId;

    QJsonObject systemConfiguration;
    QList<QJsonObject> applications;

    QJsonObject toJson() const;
    static DeviceConfiguration fromJson(const QJsonObject& json);
    bool isValid() const;

    int applicationCount() const;
    bool hasApplications() const;
    void addApplication(const QJsonObject& appConfig);
    void removeApplication(const QString& appId);
    void updateApplication(const QJsonObject& appConfig);
};

} // namespace Services::Configuration

#endif // SERVICES_CONFIGURATION_DEVICECONFIGURATION_H
