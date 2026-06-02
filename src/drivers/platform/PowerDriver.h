#ifndef DRIVERS_HARDWARE_POWER_DRIVER_H
#define DRIVERS_HARDWARE_POWER_DRIVER_H

#include <QObject>
#include <QString>

namespace Drivers::Hardware
{

class PowerDriver : public QObject
{
    Q_OBJECT

  public:
    explicit PowerDriver(QObject* parent = nullptr);

    bool shutdown(QString* error = nullptr) const;
    bool reboot(QString* error = nullptr) const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_POWER_DRIVER_H
