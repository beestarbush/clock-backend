#include "Container.h"

#include "drivers/Container.h"

namespace Services
{
const quint16 INGRESS_PORT = 5000;

Container::Container(Drivers::Container& drivers, QObject* parent)
    : QObject(parent),
      m_websocket(),
      m_rest(),
      m_ingress(m_rest, m_websocket),
      m_configuration(m_websocket),
      m_display(m_configuration, m_websocket),
      m_media(m_websocket),
      m_environment(m_websocket),
      m_status(m_websocket),
      m_audio(m_configuration, m_websocket),
      m_system(drivers.m_power, m_websocket)
{
    connect(&m_rest, &Common::Communication::Rest::Server::Service::mediaUploaded, &m_media, [this](const QString&) {
        m_media.publishCurrentMedia();
    });

    m_ingress.start(INGRESS_PORT);
    m_media.start();
    m_environment.start();
    m_status.start();
}

} // namespace Services
