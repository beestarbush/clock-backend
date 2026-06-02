#ifndef DRIVERS_HARDWARE_TEMPERATURE_DRIVER_H
#define DRIVERS_HARDWARE_TEMPERATURE_DRIVER_H

#include <QObject>
#include <QString>

#include <optional>

namespace Drivers::Hardware
{

class TemperatureDriver : public QObject
{
    Q_OBJECT

  public:
    explicit TemperatureDriver(QObject* parent = nullptr);

    std::optional<double> getProcessorTemperature(const QString& dataDir) const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_TEMPERATURE_DRIVER_H
