#ifndef SERVICES_AUDIO_SERVICE_H
#define SERVICES_AUDIO_SERVICE_H

#include <QObject>
#include <QString>

namespace Drivers::Hardware
{
class AudioDriver;
}

namespace Services::Configuration
{
class Service;
}

namespace Services::WebSocket
{
class Service;
}

namespace Services::Audio
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Drivers::Hardware::AudioDriver& audio,
                     Services::Configuration::Service& configuration,
                     const QString& dataDir,
                     Services::WebSocket::Service* websocket = nullptr,
                     QObject* parent = nullptr);

  private:
    QString mediaPath(const QString& filename) const;

    Drivers::Hardware::AudioDriver& m_audio;
    Services::Configuration::Service& m_configuration;
    QString m_dataDir;
};

} // namespace Services::Audio

#endif //SERVICES_AUDIO_SERVICE_H
