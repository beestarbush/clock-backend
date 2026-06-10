#include "Service.h"

#include <QFile>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTextStream>

#include "services/configuration/Service.h"
#include "websocket/server/Service.h"

#include <algorithm>

Q_LOGGING_CATEGORY(DisplayService, "DisplayService")

namespace
{
constexpr quint8 PWM_MAX = 31;
#ifdef PLATFORM_IS_TARGET
const QString BRIGHTNESS_SYSFS = QStringLiteral("/sys/class/backlight/11-0045/brightness");
#else
const QString BRIGHTNESS_SYSFS = QStringLiteral("/workdir/data/brightness");
#endif
} // namespace

namespace Services::Display
{

Service::Service(Services::Configuration::Service& configuration,
                 Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent),
      m_configuration(configuration)
{
    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    if (!setBrightness(static_cast<quint8>(m_configuration.brightness()))) {
        qWarning(DisplayService) << QStringLiteral("Failed to apply configured brightness.");
    }

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::SetBrightness, [this, &websocket](const QJsonObject& params) {
        const QJsonValue valueParam = params.value("value");
        if (!valueParam.isDouble()) {
            return Result::error(-32000, QStringLiteral("Brightness value must be an integer between 0 and 100"));
        }

        const int requestedValue = valueParam.toInt();
        if (requestedValue < 0 || requestedValue > 100) {
            return Result::error(-32000, QStringLiteral("Brightness value must be between 0 and 100"));
        }

        const quint8 brightness = static_cast<quint8>(requestedValue);
        if (!setBrightness(brightness)) {
            return Result::error(-32000, QStringLiteral("Failed to set brightness"));
        }

        const QJsonObject configResult = m_configuration.setBrightness(brightness);
        if (configResult.contains("__error")) {
            return Result::error(-32000, configResult.value("__error").toString());
        }

        websocket.publish(Common::Communication::WebSocket::Topic::Configuration, m_configuration.asSystemConfigJson());

        return Result::success(QJsonObject{{"brightness", requestedValue}});
    });
}

bool Service::setBrightness(quint8 value) const
{
    const quint8 clamped = std::clamp(value, quint8{0}, quint8{100});
    const quint8 pwmVal = (clamped * PWM_MAX) / 100;

    QFile sysfs(BRIGHTNESS_SYSFS);
    if (!sysfs.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(DisplayService) << "Failed to open brightness sysfs for writing:" << BRIGHTNESS_SYSFS << sysfs.errorString();
        return false;
    }

    qCDebug(DisplayService) << "Setting brightness to" << static_cast<int>(clamped) << "% (PWM value:" << static_cast<int>(pwmVal) << ")";
    QTextStream out(&sysfs);
    out << pwmVal;
    return true;
}

} // namespace Services::Display
