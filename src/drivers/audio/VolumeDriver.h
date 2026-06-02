#ifndef DRIVERS_HARDWARE_VOLUME_DRIVER_H
#define DRIVERS_HARDWARE_VOLUME_DRIVER_H

#include <QObject>
#include <QString>

namespace Drivers::Hardware
{

class VolumeDriver : public QObject
{
    Q_OBJECT

  public:
    explicit VolumeDriver(QObject* parent = nullptr);

    bool setVolume(int value, QString* error = nullptr) const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_VOLUME_DRIVER_H
