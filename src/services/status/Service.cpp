#include "Service.h"

#include "services/websocket/Service.h"

namespace Services::Status
{

Service::Service(Services::WebSocket::Service& websocket,
                 QObject* parent)
    : QObject(parent)
{
    using Result = Services::WebSocket::Service::MethodResult;

    websocket.registerPeriodicPublisher(Services::WebSocket::Topic::BackendStatus, 1000, [this]() {
        return backendStatus();
    });

    websocket.registerPublishHandler(Services::WebSocket::Topic::ApplicationStatus, [this](const QJsonObject& params) {
        setApplicationStatus(params);
    });

    websocket.registerMethodHandler(Services::WebSocket::Method::GetStatus, [this](const QJsonObject&) {
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
