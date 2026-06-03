#ifndef SERVICES_MEDIA_SERVICE_H
#define SERVICES_MEDIA_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Media
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);

    void start();
    void publishCurrentMedia();
    QStringList listMediaFiles(bool imageOnly) const;

  private:
    QJsonObject buildMediaPayload() const;
    void publishIfChanged();

    Common::Communication::WebSocket::Server::Service& m_websocket;
    QTimer m_mediaScanTimer;
    QStringList m_lastPublishedImages;
    QStringList m_lastPublishedAll;
};

} // namespace Services::Media

#endif // SERVICES_MEDIA_SERVICE_H
