#include "Service.h"

#include <QJsonObject>

#include "drivers/platform/PowerDriver.h"
#include "services/websocket/Service.h"

namespace Services::System
{

Service::Service(Drivers::Hardware::PowerDriver& power,
                 Services::WebSocket::Service* websocket,
                 QObject* parent)
    : QObject(parent),
      m_power(power)
{
    using Result = Services::WebSocket::Service::MethodResult;

    if (websocket == nullptr) {
        return;
    }

    websocket->registerMethodHandler(Services::WebSocket::Method::Shutdown, [this](const QJsonObject&) {
        QString error;
        if (!m_power.shutdown(&error)) {
            return Result::error(-32000, error);
        }
        return Result::success(QJsonObject{{"status", "shutdown initiated"}});
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::Reboot, [this](const QJsonObject&) {
        QString error;
        if (!m_power.reboot(&error)) {
            return Result::error(-32000, error);
        }
        return Result::success(QJsonObject{{"status", "reboot initiated"}});
    });
}

} // namespace Services::System
