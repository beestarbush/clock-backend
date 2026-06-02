#ifndef SERVICES_CONTAINER_H
#define SERVICES_CONTAINER_H

#include <QObject>

#include "audio/Service.h"
#include "configuration/Service.h"
#include "ingress/Service.h"
#include "media/Service.h"
#include "rest/Service.h"
#include "status/Service.h"
#include "system/Service.h"
#include "websocket/Service.h"

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
    explicit Container(Drivers::Container& drivers, const QString& dataDir, QObject* parent = nullptr);

    friend class ::Applications::Container;

  private:
    WebSocket::Service m_websocket;
    Rest::Service m_rest;
    Ingress::Service m_ingress;
    Configuration::Service m_configuration;
    Media::Service m_media;
    Status::Service m_status;
    Audio::Service m_audio;
    System::Service m_system;
};

} // namespace Services

#endif //SERVICES_CONTAINER_H
