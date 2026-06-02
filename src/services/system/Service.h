#ifndef SERVICES_SYSTEM_SERVICE_H
#define SERVICES_SYSTEM_SERVICE_H

#include <QObject>

namespace Drivers::Hardware
{
class PowerDriver;
}

namespace Services::WebSocket
{
class Service;
}

namespace Services::System
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Drivers::Hardware::PowerDriver& power,
                     Services::WebSocket::Service* websocket = nullptr,
                     QObject* parent = nullptr);

  private:
    Drivers::Hardware::PowerDriver& m_power;
};

} // namespace Services::System

#endif //SERVICES_SYSTEM_SERVICE_H
