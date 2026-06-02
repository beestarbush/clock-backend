#include "Service.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

#include <algorithm>

#include "drivers/audio/VolumeDriver.h"
#include "drivers/display/BrightnessDriver.h"
#include "services/websocket/Service.h"

namespace Services::Configuration
{

Service::Service(Drivers::Hardware::BrightnessDriver& brightness,
                 Drivers::Hardware::VolumeDriver& volume,
                 const QString& dataDir,
                 Services::WebSocket::Service* websocket,
                 QObject* parent)
    : QObject(parent),
      m_brightness(brightness),
      m_volume(volume),
      m_dataDir(dataDir)
{
    using Result = Services::WebSocket::Service::MethodResult;

    if (websocket == nullptr) {
        return;
    }

    websocket->registerMethodHandler(Services::WebSocket::Method::GetConfig, [this](const QJsonObject&) {
        return Result::success(asJson());
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::SetBrightness, [this](const QJsonObject& params) {
        const QJsonObject result = setBrightness(params.value("value").toInt());
        if (result.contains("__error")) {
            return Result::error(-32000, result.value("__error").toString());
        }
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::SetVolume, [this](const QJsonObject& params) {
        const QJsonObject result = setVolume(params.value("value").toInt());
        if (result.contains("__error")) {
            return Result::error(-32000, result.value("__error").toString());
        }
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::SetDeviceId, [this, websocket](const QJsonObject& params) {
        const QJsonObject result = setDeviceId(params.value("device_id").toString());
        websocket->publish(Services::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::UpdateSystemConfig, [this, websocket](const QJsonObject& params) {
        const QJsonObject result = updateSystemConfig(params);
        websocket->publish(Services::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::AddApp, [this, websocket](const QJsonObject& params) {
        const QJsonObject result = addApp(params);
        websocket->publish(Services::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::UpdateApp, [this, websocket](const QJsonObject& params) {
        const QJsonObject result = updateApp(params);
        websocket->publish(Services::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::RemoveApp, [this, websocket](const QJsonObject& params) {
        const QJsonObject result = removeApp(params.value("id").toString());
        websocket->publish(Services::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });
}

bool Service::load()
{
    QFile file(dataPath(QStringLiteral("configuration.json")));

    if (!file.exists()) {
        m_configuration.version = QStringLiteral("1.0");
        m_configuration.deviceId = QStringLiteral("SN-XXXX");
        m_configuration.systemConfiguration = {
            {"brightness", 75},
            {"volume", 75},
            {"pendulum-bob-color", "#009950"},
            {"pendulum-rod-color", "#333333"},
            {"pendulum-background-color", "#3d3846"},
            {"base-color", "#000000"},
            {"accent-color", "#009950"},
        };
        m_configuration.lastModified = QDateTime::currentDateTimeUtc();
        return save();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    m_configuration = DeviceConfiguration::fromJson(doc.object());
    if (!m_configuration.systemConfiguration.contains("brightness")) {
        m_configuration.systemConfiguration["brightness"] = 75;
    }
    if (!m_configuration.systemConfiguration.contains("volume")) {
        m_configuration.systemConfiguration["volume"] = 75;
    }
    return true;
}

bool Service::save()
{
    m_configuration.lastModified = QDateTime::currentDateTimeUtc();

    QSaveFile file(dataPath(QStringLiteral("configuration.json")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument doc(m_configuration.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return file.commit();
}

QJsonObject Service::asJson() const
{
    return m_configuration.toJson();
}

int Service::volume() const
{
    return m_configuration.systemConfiguration.value("volume").toInt(75);
}

QJsonObject Service::setBrightness(int value)
{
    QString error;
    const int clamped = std::clamp(value, 0, 100);
    if (!m_brightness.setBrightness(clamped, m_dataDir, &error)) {
        return QJsonObject{{"__error", error}};
    }

    m_configuration.systemConfiguration["brightness"] = clamped;
    save();
    return QJsonObject{{"brightness", clamped}};
}

QJsonObject Service::setVolume(int value)
{
    QString error;
    const int clamped = std::clamp(value, 0, 100);
    if (!m_volume.setVolume(clamped, &error)) {
        return QJsonObject{{"__error", error}};
    }

    m_configuration.systemConfiguration["volume"] = clamped;
    save();
    return QJsonObject{{"volume", clamped}};
}

QJsonObject Service::setDeviceId(const QString& deviceId)
{
    m_configuration.deviceId = deviceId;
    save();
    return QJsonObject{{"device_id", deviceId}};
}

QJsonObject Service::updateSystemConfig(const QJsonObject& params)
{
    auto& sc = m_configuration.systemConfiguration;
    const QStringList keys = {
        QStringLiteral("pendulum-bob-color"),
        QStringLiteral("pendulum-rod-color"),
        QStringLiteral("pendulum-background-color"),
        QStringLiteral("base-color"),
        QStringLiteral("accent-color"),
    };
    for (const QString& key : keys) {
        if (params.contains(key)) {
            sc[key] = params.value(key);
        }
    }
    save();
    return QJsonObject{{"status", "updated"}};
}

QJsonObject Service::addApp(const QJsonObject& app)
{
    m_configuration.addApplication(app);
    save();
    return QJsonObject{{"status", "added"}};
}

QJsonObject Service::updateApp(const QJsonObject& app)
{
    m_configuration.updateApplication(app);
    save();
    return QJsonObject{{"status", "updated"}};
}

QJsonObject Service::removeApp(const QString& appId)
{
    m_configuration.removeApplication(appId);
    save();
    return QJsonObject{{"status", "removed"}};
}

QString Service::dataPath(const QString& relative) const
{
    return QDir(m_dataDir).filePath(relative);
}

} // namespace Services::Configuration
