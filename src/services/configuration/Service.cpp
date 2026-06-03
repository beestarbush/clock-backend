#include "Service.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSaveFile>

#include <algorithm>

#include "websocket/server/Service.h"

Q_LOGGING_CATEGORY(ConfigurationService, "ConfigurationService")

namespace Services::Configuration
{

namespace
{
#ifdef PLATFORM_IS_TARGET
const QString DEFAULT_DATA_DIR = QStringLiteral(".");
#else
const QString DEFAULT_DATA_DIR = QStringLiteral("/workdir/data");
#endif

const QString PERSISTENCE_ERROR = QStringLiteral("Failed to persist configuration");

bool hasOperationError(const QJsonObject& result)
{
    return result.contains("__error");
}
} // namespace

Service::Service(Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent)
{
    if (!load()) {
        qCWarning(ConfigurationService) << "Failed to load configuration, using defaults.";
    }

    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::GetConfig, [this](const QJsonObject&) {
        return Result::success(asJson());
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::SetDeviceId, [this, &websocket](const QJsonObject& params) {
        const QJsonObject result = setDeviceId(params.value("device_id").toString());
        if (hasOperationError(result)) {
            return Result::error(-32000, result.value("__error").toString());
        }
        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::UpdateSystemConfig, [this, &websocket](const QJsonObject& params) {
        const QJsonObject result = updateSystemConfig(params);
        if (hasOperationError(result)) {
            return Result::error(-32000, result.value("__error").toString());
        }
        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::AddApp, [this, &websocket](const QJsonObject& params) {
        const QJsonObject result = addApp(params);
        if (hasOperationError(result)) {
            return Result::error(-32000, result.value("__error").toString());
        }
        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::UpdateApp, [this, &websocket](const QJsonObject& params) {
        const QJsonObject result = updateApp(params);
        if (hasOperationError(result)) {
            return Result::error(-32000, result.value("__error").toString());
        }
        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::RemoveApp, [this, &websocket](const QJsonObject& params) {
        const QJsonObject result = removeApp(params.value("id").toString());
        if (hasOperationError(result)) {
            return Result::error(-32000, result.value("__error").toString());
        }
        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, asJson());
        return Result::success(result);
    });
}

bool Service::load()
{
    const QString configurationPath = QDir(DEFAULT_DATA_DIR).filePath(QStringLiteral("configuration.json"));
    QFile file(configurationPath);

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

    m_configuration = Common::Communication::Configuration::DeviceConfiguration::fromJson(doc.object());
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

    if (!QDir().mkpath(DEFAULT_DATA_DIR)) {
        qCWarning(ConfigurationService) << "Failed to create data directory:" << DEFAULT_DATA_DIR;
        return false;
    }

    const QString configurationPath = QDir(DEFAULT_DATA_DIR).filePath(QStringLiteral("configuration.json"));

    const QFileInfo configurationInfo(configurationPath);
    if (configurationInfo.exists() && !configurationInfo.isWritable()) {
        qCWarning(ConfigurationService) << "Configuration file is not writable, attempting replacement:" << configurationPath;
        if (!QFile::remove(configurationPath)) {
            qCWarning(ConfigurationService) << "Failed to remove non-writable configuration:" << configurationPath;
            return false;
        }
    }

    QSaveFile file(configurationPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(ConfigurationService) << "Failed to open configuration for write:" << configurationPath
                                        << "error:" << file.errorString();
        return false;
    }

    const QJsonDocument doc(m_configuration.toJson());
    if (file.write(doc.toJson(QJsonDocument::Indented)) == -1) {
        qCWarning(ConfigurationService) << "Failed to write configuration:" << configurationPath
                                        << "error:" << file.errorString();
        return false;
    }

    if (!file.commit()) {
        qCWarning(ConfigurationService) << "Failed to commit configuration:" << configurationPath
                                        << "error:" << file.errorString();
        return false;
    }

    return true;
}

QJsonObject Service::asJson() const
{
    return m_configuration.toJson();
}

int Service::brightness() const
{
    const int configured = m_configuration.systemConfiguration.value("brightness").toInt(75);
    return std::clamp(configured, 0, 100);
}

int Service::volume() const
{
    const int configured = m_configuration.systemConfiguration.value("volume").toInt(75);
    return std::clamp(configured, 0, 100);
}

QJsonObject Service::setBrightness(quint8 value)
{
    if (value > 100) {
        return QJsonObject{{"__error", QStringLiteral("Brightness value must be between 0 and 100")}};
    }

    m_configuration.systemConfiguration["brightness"] = static_cast<int>(value);
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"brightness", static_cast<int>(value)}};
}

QJsonObject Service::setVolume(int value)
{
    const int clamped = std::clamp(value, 0, 100);
    m_configuration.systemConfiguration["volume"] = clamped;
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"volume", clamped}};
}

QJsonObject Service::setDeviceId(const QString& deviceId)
{
    m_configuration.deviceId = deviceId;
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
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
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"status", "updated"}};
}

QJsonObject Service::addApp(const QJsonObject& app)
{
    m_configuration.addApplication(app);
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"status", "added"}};
}

QJsonObject Service::updateApp(const QJsonObject& app)
{
    m_configuration.updateApplication(app);
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"status", "updated"}};
}

QJsonObject Service::removeApp(const QString& appId)
{
    m_configuration.removeApplication(appId);
    if (!save()) {
        qCWarning(ConfigurationService) << PERSISTENCE_ERROR;
        return QJsonObject{{"__error", PERSISTENCE_ERROR}};
    }
    return QJsonObject{{"status", "removed"}};
}

} // namespace Services::Configuration
