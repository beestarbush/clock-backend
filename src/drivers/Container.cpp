#include "Container.h"

namespace Drivers
{

Container::Container(QObject* parent)
    : QObject(parent),
      m_brightness(),
      m_volume(),
      m_temperature(),
      m_environment(),
      m_power(),
      m_audio()
{
}

} // namespace Drivers
