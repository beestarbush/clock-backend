#include "Service.h"

#include "websocket/server/Service.h"

#include <QFile>
#include <QLoggingCategory>

#include <chrono>

#ifdef PLATFORM_IS_TARGET
const QString ENVIRONMENT_IIO_BASE = QStringLiteral("/sys/devices/platform/axi/1000120000.pcie/1f00074000.i2c/i2c-1/1-0062/iio:device0");
const QString PROCESSOR_TEMP_SYSFS = QStringLiteral("/sys/class/thermal/thermal_zone0/hwmon0/temp1_input");
#else
const QString ENVIRONMENT_IIO_BASE = QStringLiteral("/workdir/data/environment");
const QString PROCESSOR_TEMP_SYSFS = QStringLiteral("/workdir/data/environment/processor_temperature");
#endif

Q_LOGGING_CATEGORY(EnvironmentService, "EnvironmentService")

namespace Services::Environment
{

namespace
{
constexpr int ENVIRONMENT_REFRESH_INTERVAL_MS = 5000;
constexpr int REFRESH_STOP_GRANULARITY_MS = 200;
} // namespace

Service::Service(Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent)
{
    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::GetEnvironment, [this](const QJsonObject&) {
        return Result::success(environment());
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::GetProcessorTemperature, [this](const QJsonObject&) {
        return Result::success(processorTemperature());
    });

    websocket.registerPeriodicPublisher(Common::Communication::WebSocket::Topic::Environment, ENVIRONMENT_REFRESH_INTERVAL_MS, [this]() {
        return environment();
    });

    websocket.registerPeriodicPublisher(Common::Communication::WebSocket::Topic::ProcessorTemperature, 60000, [this]() {
        return processorTemperature();
    });
}

Service::~Service()
{
    m_stopRequested = true;
    if (m_refreshThread.joinable()) {
        m_refreshThread.join();
    }
}

void Service::start()
{
    if (m_refreshThread.joinable()) {
        return;
    }

    m_stopRequested = false;
    m_refreshThread = std::thread([this]() {
        refreshLoop();
    });
}

QJsonObject Service::environment() const
{
    std::scoped_lock lock(m_environmentMutex);
    return m_environment;
}

std::optional<double> Service::readDoubleFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    bool ok = false;
    const double value = QString::fromUtf8(file.readAll().trimmed()).toDouble(&ok);
    if (!ok) {
        return std::nullopt;
    }

    return value;
}

QJsonObject Service::processorTemperature() const
{
    const QJsonObject snapshot = environment();
    QJsonObject result;
    result["processor_temperature"] = snapshot.value("processor_temperature").toVariant().toDouble();
    return result;
}

QJsonObject Service::refreshEnvironment()
{
    const auto co2Raw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_concentration_co2_raw"));
    const auto co2Scale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_concentration_co2_scale"));
    const auto tempRaw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_temp_raw"));
    const auto tempScale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_temp_scale"));
    const auto humRaw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_humidityrelative_raw"));
    const auto humScale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_humidityrelative_scale"));
    const auto processorTemp = readDoubleFile(PROCESSOR_TEMP_SYSFS);

    QJsonObject snapshot;
    if (!co2Raw || !co2Scale || !tempRaw || !tempScale || !humRaw || !humScale || !processorTemp) {
        qCWarning(EnvironmentService) << "Failed to read environment sensor values.";
        snapshot = QJsonObject{
            {"co2_parts_per_million", QJsonValue::Null},
            {"temperature_celsius", QJsonValue::Null},
            {"humidity_percentage", QJsonValue::Null},
            {"processor_temperature", QJsonValue::Null},
        };
    }

    if (snapshot.isEmpty()) {
        qCDebug(EnvironmentService) << "Read environment sensor values: co2_raw:" << *co2Raw << "co2_scale:" << *co2Scale
                                    << "temp_raw:" << *tempRaw << "temp_scale:" << *tempScale
                                    << "hum_raw:" << *humRaw << "hum_scale:" << *humScale
                                    << "processor_temp:" << *processorTemp;

        snapshot = QJsonObject{
            {"co2_parts_per_million", *co2Raw},
            {"temperature_celsius", ((*tempRaw * *tempScale) / 1000.0) - 45.0},
            {"humidity_percentage", (*humRaw * *humScale) / 1000.0},
            {"processor_temperature", *processorTemp},
        };
    }

    {
        std::scoped_lock lock(m_environmentMutex);
        m_environment = snapshot;
    }

    return snapshot;
}

void Service::refreshLoop()
{
    while (!m_stopRequested.load()) {
        refreshEnvironment();

        int sleptMs = 0;
        while (!m_stopRequested.load() && sleptMs < ENVIRONMENT_REFRESH_INTERVAL_MS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_STOP_GRANULARITY_MS));
            sleptMs += REFRESH_STOP_GRANULARITY_MS;
        }
    }
}

} // namespace Services::Environment
