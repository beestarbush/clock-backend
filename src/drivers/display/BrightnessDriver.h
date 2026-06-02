#ifndef DRIVERS_HARDWARE_BRIGHTNESS_DRIVER_H
#define DRIVERS_HARDWARE_BRIGHTNESS_DRIVER_H

#include <QObject>
#include <QString>

namespace Drivers::Hardware
{

class BrightnessDriver : public QObject
{
    Q_OBJECT

  public:
    explicit BrightnessDriver(QObject* parent = nullptr);

    bool setBrightness(int value, const QString& dataDir, QString* error = nullptr) const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_BRIGHTNESS_DRIVER_H
