#ifndef DRIVERS_PLATFORM_DRIVER_H
#define DRIVERS_PLATFORM_DRIVER_H

#include <QObject>
#include <QString>

namespace Drivers::Platform
{

class Driver : public QObject
{
    Q_OBJECT

  public:
    explicit Driver(QObject* parent = nullptr);

    bool shutdown(QString* error = nullptr) const;
    bool reboot(QString* error = nullptr) const;
};

} // namespace Drivers::Platform

#endif // DRIVERS_PLATFORM_DRIVER_H
