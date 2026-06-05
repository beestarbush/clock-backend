#include "Service.h"

#include "websocket/server/Service.h"

namespace Services::Status
{

Service::Service(Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent)
{
    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    websocket.registerPeriodicPublisher(Common::Communication::WebSocket::Topic::BackendStatus, 1000, [this]() {
        return backendStatus();
    });

    websocket.registerPublishHandler(Common::Communication::WebSocket::Topic::ApplicationStatus, [this](const QJsonObject& params) {
        setApplicationStatus(params);
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::GetStatus, [this](const QJsonObject&) {
        return Result::success(appStatus());
    });
}

void Service::start()
{
    m_uptime.start();
}

QJsonObject Service::appStatus() const
{
    return m_appStatus;
}

QJsonObject Service::backendStatus() const
{
    return QJsonObject{{"uptime", static_cast<double>(m_uptime.elapsed()) / 1000.0}};
}

void Service::setApplicationStatus(const QJsonObject& params)
{
    if (params.contains("version")) {
        m_appStatus["version"] = params.value("version");
    }
}

} // namespace Services::Status
