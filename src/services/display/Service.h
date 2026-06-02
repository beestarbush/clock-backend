#ifndef SERVICES_DISPLAY_SERVICE_H
#define SERVICES_DISPLAY_SERVICE_H

#include <QObject>
#include <QString>

namespace Services::Configuration
{
class Service;
}

namespace Services::WebSocket
{
class Service;
}

namespace Services::Display
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Services::Configuration::Service& configuration,
                     Services::WebSocket::Service& websocket,
                     QObject* parent = nullptr);

  private:
    bool setBrightness(quint8 value) const;

    Services::Configuration::Service& m_configuration;
};

} // namespace Services::Display

#endif // SERVICES_DISPLAY_SERVICE_H
