#include "Service.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>

#include "services/websocket/Service.h"

namespace Services::Media
{
#ifdef PLATFORM_IS_TARGET
const QString MEDIA_DIR = QStringLiteral("/data/media");
#else
const QString MEDIA_DIR = QStringLiteral("/workdir/data/media");
#endif

Service::Service(Services::WebSocket::Service& websocket, QObject* parent)
    : QObject(parent),
      m_websocket(websocket)
{
    using Result = Services::WebSocket::Service::MethodResult;

    m_websocket.registerMethodHandler(Services::WebSocket::Method::GetMedia, [this](const QJsonObject&) {
        return Result::success(QJsonObject{{"files", QJsonArray::fromStringList(listMediaFiles(true))}});
    });

    m_websocket.registerMethodHandler(Services::WebSocket::Method::GetAllMedia, [this](const QJsonObject&) {
        return Result::success(QJsonObject{{"files", QJsonArray::fromStringList(listMediaFiles(false))}});
    });

    m_mediaScanTimer.setInterval(5000);
    connect(&m_mediaScanTimer, &QTimer::timeout, this, &Service::publishIfChanged);
}

void Service::start()
{
    m_lastPublishedImages = listMediaFiles(true);
    m_lastPublishedAll = listMediaFiles(false);
    if (!m_mediaScanTimer.isActive()) {
        m_mediaScanTimer.start();
    }
}

void Service::publishCurrentMedia()
{
    m_lastPublishedImages = listMediaFiles(true);
    m_lastPublishedAll = listMediaFiles(false);
    m_websocket.publish(Services::WebSocket::Topic::Media, buildMediaPayload());
}

QStringList Service::listMediaFiles(bool imageOnly) const
{
    QDir dir(MEDIA_DIR);
    if (!dir.exists()) {
        return {};
    }

    const QStringList files = dir.entryList(QDir::Files, QDir::Name);
    if (!imageOnly) {
        return files;
    }

    QStringList filtered;
    const QSet<QString> imageExt = {
        QStringLiteral("gif"),
        QStringLiteral("png"),
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
    };

    for (const QString& file : files) {
        if (imageExt.contains(QFileInfo(file).suffix().toLower())) {
            filtered.append(file);
        }
    }
    return filtered;
}

QJsonObject Service::buildMediaPayload() const
{
    const QJsonArray imageFiles = QJsonArray::fromStringList(listMediaFiles(true));
    const QJsonArray allFiles = QJsonArray::fromStringList(listMediaFiles(false));

    return QJsonObject{
        {"images", imageFiles},
        {"media", allFiles},
        {"files", imageFiles},
    };
}

void Service::publishIfChanged()
{
    const QStringList currentImages = listMediaFiles(true);
    const QStringList currentAll = listMediaFiles(false);

    if (currentImages == m_lastPublishedImages && currentAll == m_lastPublishedAll) {
        return;
    }

    m_lastPublishedImages = currentImages;
    m_lastPublishedAll = currentAll;
    m_websocket.publish(Services::WebSocket::Topic::Media, buildMediaPayload());
}

} // namespace Services::Media
