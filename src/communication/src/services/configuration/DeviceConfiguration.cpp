#include "DeviceConfiguration.h"

#include <QJsonArray>

#include <algorithm>

namespace Services::Configuration
{

DeviceConfiguration::DeviceConfiguration()
    : version("1.0")
{
    lastModified = QDateTime::currentDateTime();
}

QJsonObject DeviceConfiguration::toJson() const
{
    QJsonObject json;

    json["version"] = version;

    if (!deviceId.isEmpty()) {
        json["device_id"] = deviceId;
    }

    if (lastModified.isValid()) {
        json["last_modified"] = lastModified.toString(Qt::ISODate);
    }

    if (!activeAppId.isEmpty()) {
        json["active_app_id"] = activeAppId;
    }

    if (!systemConfiguration.isEmpty()) {
        json["system-configuration"] = systemConfiguration;
    }

    QJsonArray appsArray;
    for (const QJsonObject& appConfig : applications) {
        appsArray.append(appConfig);
    }
    json["applications"] = appsArray;

    return json;
}

DeviceConfiguration DeviceConfiguration::fromJson(const QJsonObject& json)
{
    QJsonObject configJson = json;
    if (json.contains("configuration") && json["configuration"].isObject()) {
        configJson = json["configuration"].toObject();
    }

    DeviceConfiguration config;

    config.version = configJson["version"].toString("1.0");
    config.deviceId = configJson["device_id"].toString();
    config.activeAppId = configJson["active_app_id"].toString();

    const QString lastModifiedStr = configJson["last_modified"].toString();
    if (!lastModifiedStr.isEmpty()) {
        config.lastModified = QDateTime::fromString(lastModifiedStr, Qt::ISODate);
    }

    if (configJson.contains("system-configuration")) {
        config.systemConfiguration = configJson["system-configuration"].toObject();
    }

    const QJsonArray appsArray = configJson["applications"].toArray();
    for (const QJsonValue& value : appsArray) {
        if (value.isObject()) {
            config.applications.append(value.toObject());
        }
    }

    return config;
}

bool DeviceConfiguration::isValid() const
{
    return !version.isEmpty();
}

int DeviceConfiguration::applicationCount() const
{
    return applications.count();
}

bool DeviceConfiguration::hasApplications() const
{
    return !applications.isEmpty();
}

void DeviceConfiguration::addApplication(const QJsonObject& appConfig)
{
    applications.append(appConfig);
    lastModified = QDateTime::currentDateTime();
}

void DeviceConfiguration::removeApplication(const QString& appId)
{
    applications.erase(
        std::remove_if(applications.begin(), applications.end(), [&appId](const QJsonObject& appConfig) {
            return appConfig["id"].toString() == appId;
        }),
        applications.end());
    lastModified = QDateTime::currentDateTime();
}

void DeviceConfiguration::updateApplication(const QJsonObject& appConfig)
{
    const QString appId = appConfig["id"].toString();
    for (int i = 0; i < applications.size(); ++i) {
        if (applications[i]["id"].toString() == appId) {
            applications[i] = appConfig;
            lastModified = QDateTime::currentDateTime();
            return;
        }
    }

    addApplication(appConfig);
}

} // namespace Services::Configuration
