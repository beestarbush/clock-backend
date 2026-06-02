#include "Container.h"

namespace Drivers
{

Container::Container(QObject* parent)
    : QObject(parent),
      m_power()
{
}

} // namespace Drivers
