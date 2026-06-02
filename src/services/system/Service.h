#ifndef SERVICES_SYSTEM_SERVICE_H
#define SERVICES_SYSTEM_SERVICE_H

#include <QObject>

namespace Drivers::Platform
{
class Driver;
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
    explicit Service(Drivers::Platform::Driver& power,
                     Services::WebSocket::Service& websocket,
                     QObject* parent = nullptr);

  private:
    Drivers::Platform::Driver& m_power;
};

} // namespace Services::System

#endif // SERVICES_SYSTEM_SERVICE_H
