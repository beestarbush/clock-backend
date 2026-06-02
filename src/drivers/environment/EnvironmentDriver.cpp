#include "EnvironmentDriver.h"

#include <QFile>

namespace Drivers::Hardware
{

namespace
{
const QString ENVIRONMENT_IIO_BASE =
    QStringLiteral("/sys/devices/platform/axi/1000120000.pcie/1f00074000.i2c/i2c-1/1-0062/iio:device0");

std::optional<double> readDoubleFile(const QString& path)
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

} // namespace

EnvironmentDriver::EnvironmentDriver(QObject* parent)
    : QObject(parent)
{
}

std::optional<QJsonObject> EnvironmentDriver::getEnvironmentData() const
{
    const auto co2Raw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_concentration_co2_raw"));
    const auto co2Scale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_concentration_co2_scale"));
    const auto tempRaw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_temp_raw"));
    const auto tempScale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_temp_scale"));
    const auto humRaw = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_humidityrelative_raw"));
    const auto humScale = readDoubleFile(ENVIRONMENT_IIO_BASE + QStringLiteral("/in_humidityrelative_scale"));

    if (co2Raw && co2Scale && tempRaw && tempScale && humRaw && humScale) {
        return QJsonObject{
            {"co2_parts_per_million", *co2Raw},
            {"temperature_celsius", ((*tempRaw * *tempScale) / 1000.0) - 45.0},
            {"humidity_percentage", (*humRaw * *humScale) / 1000.0},
        };
    }

    return QJsonObject{
        {"co2_parts_per_million", 750.0},
        {"temperature_celsius", 21.5},
        {"humidity_percentage", 45.0},
    };
}

} // namespace Drivers::Hardware
