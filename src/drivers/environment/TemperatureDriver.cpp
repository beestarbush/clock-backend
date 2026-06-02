#include "TemperatureDriver.h"

#include <QDir>
#include <QFile>

namespace Drivers::Hardware
{

namespace
{
const QString TEMP_SYSFS = QStringLiteral("/sys/class/thermal/thermal_zone0/hwmon0/temp1_input");
}

TemperatureDriver::TemperatureDriver(QObject* parent)
    : QObject(parent)
{
}

std::optional<double> TemperatureDriver::getProcessorTemperature(const QString& dataDir) const
{
    auto readTemperature = [](const QString& path) -> std::optional<double> {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return std::nullopt;
        }

        const QByteArray content = file.readAll().trimmed();
        bool ok = false;
        const double value = QString::fromUtf8(content).toDouble(&ok);
        if (!ok) {
            return std::nullopt;
        }
        return value;
    };

    if (auto value = readTemperature(TEMP_SYSFS)) {
        return value;
    }

    return readTemperature(QDir(dataDir).filePath("processor_temperature"));
}

} // namespace Drivers::Hardware
