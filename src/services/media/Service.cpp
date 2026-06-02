#include "Service.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

#include "services/websocket/Service.h"

namespace Services::Media
{

Service::Service(const QString& dataDir, Services::WebSocket::Service* websocket, QObject* parent)
    : QObject(parent),
      m_dataDir(dataDir)
{
    using Result = Services::WebSocket::Service::MethodResult;

    if (websocket == nullptr) {
        return;
    }

    websocket->registerMethodHandler(Services::WebSocket::Method::GetMedia, [this](const QJsonObject&) {
        return Result::success(QJsonObject{{"files", QJsonArray::fromStringList(listMediaFiles(true))}});
    });

    websocket->registerMethodHandler(Services::WebSocket::Method::GetAllMedia, [this](const QJsonObject&) {
        return Result::success(QJsonObject{{"files", QJsonArray::fromStringList(listMediaFiles(false))}});
    });
}

QStringList Service::listMediaFiles(bool imageOnly) const
{
    QDir dir(mediaDir());
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

QString Service::mediaDir() const
{
    return QDir(m_dataDir).filePath(QStringLiteral("media"));
}

} // namespace Services::Media
