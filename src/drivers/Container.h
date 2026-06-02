#ifndef DRIVERS_CONTAINER_H
#define DRIVERS_CONTAINER_H

#include <QObject>

#include "audio/AudioDriver.h"
#include "audio/VolumeDriver.h"
#include "display/BrightnessDriver.h"
#include "environment/EnvironmentDriver.h"
#include "environment/TemperatureDriver.h"
#include "platform/PowerDriver.h"

namespace Services
{
class Container;
}

namespace Drivers
{

class Container : public QObject
{
    Q_OBJECT

  public:
    explicit Container(QObject* parent = nullptr);

    friend class ::Services::Container;

  private:
    Hardware::BrightnessDriver m_brightness;
    Hardware::VolumeDriver m_volume;
    Hardware::TemperatureDriver m_temperature;
    Hardware::EnvironmentDriver m_environment;
    Hardware::PowerDriver m_power;
    Hardware::AudioDriver m_audio;
};

} // namespace Drivers

#endif //DRIVERS_CONTAINER_H
