#ifndef SERVICES_MEDIA_SERVICE_H
#define SERVICES_MEDIA_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace Services::WebSocket
{
class Service;
}

namespace Services::Media
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Services::WebSocket::Service& websocket,
                     QObject* parent = nullptr);

    void start();
    void publishCurrentMedia();
    QStringList listMediaFiles(bool imageOnly) const;

  private:
    QJsonObject buildMediaPayload() const;
    void publishIfChanged();

    Services::WebSocket::Service& m_websocket;
    QTimer m_mediaScanTimer;
    QStringList m_lastPublishedImages;
    QStringList m_lastPublishedAll;
};

} // namespace Services::Media

#endif // SERVICES_MEDIA_SERVICE_H
