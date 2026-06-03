#ifndef SERVICES_AUDIO_SERVICE_H
#define SERVICES_AUDIO_SERVICE_H

#include <QList>
#include <QObject>
#include <QQueue>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QAudioOutput)
QT_FORWARD_DECLARE_CLASS(QMediaPlayer)

namespace Services::Configuration
{
class Service;
}

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Audio
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Services::Configuration::Service& configuration,
                     Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

  private:
    struct ActivePlayback
    {
        QMediaPlayer* player;
        QAudioOutput* output;
    };

    bool setVolume(quint8 value);
    bool playAudioFile(const QString& mediaPath, const QString& mode);
    bool stopAudio();

    void startNextQueuedPlayback();
    bool startConcurrentPlayback(const QString& mediaPath);
    void clearQueue();

    Services::Configuration::Service& m_configuration;
    QMediaPlayer* m_queuePlayer;
    QAudioOutput* m_queueOutput;
    QQueue<QString> m_queue;
    QList<ActivePlayback> m_concurrentPlaybacks;
};

} // namespace Services::Audio

#endif // SERVICES_AUDIO_SERVICE_H
