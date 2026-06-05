#ifndef SERVICES_CONTAINER_H
#define SERVICES_CONTAINER_H

#include <QObject>
#include <QString>

#include "audio/Service.h"
#include "configuration/Service.h"
#include "display/Service.h"
#include "environment/Service.h"
#include "ingress/Service.h"
#include "media/Service.h"
#include "rest/server/Service.h"
#include "status/Service.h"
#include "system/Service.h"
#include "websocket/server/Service.h"

namespace Drivers
{
class Container;
}

namespace Applications
{
class Container;
}

namespace Services
{

class Container : public QObject
{
    Q_OBJECT

  public:
    explicit Container(Drivers::Container& drivers, QObject* parent = nullptr);

    friend class ::Applications::Container;

  private:
    Common::Communication::WebSocket::Server::Service m_websocket;
    Common::Communication::Rest::Server::Service m_rest;
    Ingress::Service m_ingress;
    Configuration::Service m_configuration;
    Display::Service m_display;
    Media::Service m_media;
    Environment::Service m_environment;
    Status::Service m_status;
    Audio::Service m_audio;
    System::Service m_system;
};

} // namespace Services

#endif // SERVICES_CONTAINER_H
