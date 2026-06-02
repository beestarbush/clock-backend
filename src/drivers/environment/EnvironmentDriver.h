#ifndef DRIVERS_HARDWARE_ENVIRONMENT_DRIVER_H
#define DRIVERS_HARDWARE_ENVIRONMENT_DRIVER_H

#include <QJsonObject>
#include <QObject>

#include <optional>

namespace Drivers::Hardware
{

class EnvironmentDriver : public QObject
{
    Q_OBJECT

  public:
    explicit EnvironmentDriver(QObject* parent = nullptr);

    std::optional<QJsonObject> getEnvironmentData() const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_ENVIRONMENT_DRIVER_H
