#ifndef SERVICES_MEDIA_SERVICE_H
#define SERVICES_MEDIA_SERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

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
    explicit Service(const QString& dataDir,
                     Services::WebSocket::Service* websocket = nullptr,
                     QObject* parent = nullptr);

    QStringList listMediaFiles(bool imageOnly) const;

  private:
    QString mediaDir() const;

    QString m_dataDir;
};

} // namespace Services::Media

#endif //SERVICES_MEDIA_SERVICE_H
