#include "Container.h"

#include "drivers/Container.h"

namespace Services
{

Container::Container(Drivers::Container& drivers, const QString& dataDir, QObject* parent)
    : QObject(parent),
      m_websocket(),
      m_rest(dataDir),
      m_ingress(m_rest, m_websocket),
    m_configuration(drivers.m_brightness, drivers.m_volume, dataDir, &m_websocket),
      m_media(dataDir, &m_websocket),
    m_status(drivers.m_environment, drivers.m_temperature, dataDir, &m_websocket),
    m_audio(drivers.m_audio, m_configuration, dataDir, &m_websocket),
    m_system(drivers.m_power, &m_websocket)
{
    m_configuration.load();
    m_status.start();
}

} // namespace Services
