#include "Service.h"

#include "drivers/environment/EnvironmentDriver.h"
#include "drivers/environment/TemperatureDriver.h"
#include "services/websocket/Service.h"

namespace Services::Status
{

Service::Service(Drivers::Hardware::EnvironmentDriver& environment,
                 Drivers::Hardware::TemperatureDriver& temperature,
                 const QString& dataDir,
                 Services::WebSocket::Service* websocket,
                 QObject* parent)
    : QObject(parent),
      m_environmentDriver(environment),
      m_temperatureDriver(temperature),
      m_dataDir(dataDir)
{
    using Result = Services::WebSocket::Service::MethodResult;

    if (websocket == nullptr) {
        return;
    }

    websocket->registerPeriodicPublisher(Services::WebSocket::Topic::BackendStatus, [this]() {
        return backendStatus();
    });

    websocket->registerPublishHandler(Services::WebSocket::Topic::ApplicationStatus, [this](const QJsonObject& params) {
        setApplicationStatus(params);
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::GetStatus, [this](const QJsonObject&) {
        return Result::success(appStatus());
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::GetProcessorTemperature, [this](const QJsonObject&) {
        return Result::success(processorTemperature());
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::GetEnvironment, [this](const QJsonObject&) {
        return Result::success(this->environment());
    });
}

void Service::start()
{
    if (auto env = m_environmentDriver.getEnvironmentData()) {
        m_environment = *env;
    }
    m_uptime.start();
}

QJsonObject Service::appStatus() const
{
    return m_appStatus;
}

QJsonObject Service::environment() const
{
    return m_environment;
}

QJsonObject Service::backendStatus() const
{
    return QJsonObject{{"uptime", static_cast<double>(m_uptime.elapsed()) / 1000.0}};
}

QJsonObject Service::processorTemperature() const
{
    QJsonObject result;
    if (auto temperature = m_temperatureDriver.getProcessorTemperature(m_dataDir)) {
        result["processor_temperature"] = *temperature;
    } else {
        result["processor_temperature"] = QJsonValue::Null;
    }
    return result;
}

void Service::setApplicationStatus(const QJsonObject& params)
{
    if (params.contains("version")) {
        m_appStatus["version"] = params.value("version");
    }
}

} // namespace Services::Status
