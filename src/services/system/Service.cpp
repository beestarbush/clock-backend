#include "Service.h"

#include <QJsonObject>

#include "drivers/platform/Driver.h"
#include "websocket/server/Service.h"

namespace Services::System
{

Service::Service(Drivers::Platform::Driver& power,
                 Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent),
      m_power(power)
{
    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::Shutdown, [this](const QJsonObject&) {
        QString error;
        if (!m_power.shutdown(&error)) {
            return Result::error(-32000, error);
        }
        return Result::success(QJsonObject{{"status", "shutdown initiated"}});
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::Reboot, [this](const QJsonObject&) {
        QString error;
        if (!m_power.reboot(&error)) {
            return Result::error(-32000, error);
        }
        return Result::success(QJsonObject{{"status", "reboot initiated"}});
    });
}

} // namespace Services::System
