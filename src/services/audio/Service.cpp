#include "Service.h"

#include <QDir>
#include <QJsonObject>

#include "drivers/audio/AudioDriver.h"
#include "services/configuration/Service.h"
#include "services/websocket/Service.h"

namespace Services::Audio
{

Service::Service(Drivers::Hardware::AudioDriver& audio,
                 Services::Configuration::Service& configuration,
                 const QString& dataDir,
                 Services::WebSocket::Service* websocket,
                 QObject* parent)
    : QObject(parent),
      m_audio(audio),
      m_configuration(configuration),
      m_dataDir(dataDir)
{
    using Result = Services::WebSocket::Service::MethodResult;

    if (websocket == nullptr) {
        return;
    }

    websocket->registerMethodHandler(Services::WebSocket::Method::PlaySound,
                                     [this](const QJsonObject& params) {
                                         QString error;
                                         const QString filename = params.value("filename").toString();
                                         const QString mode = params.value("mode").toString(QStringLiteral("concurrent"));

                                         if (!m_audio.playAudioFile(mediaPath(filename), m_configuration.volume(), mode, &error)) {
                                             return Result::error(-32001, error);
                                         }
                                         return Result::success(QJsonObject{{"status", "played"}});
                                     });

    websocket->registerMethodHandler(Services::WebSocket::Method::StopSound, [this](const QJsonObject&) {
        QString error;
        if (!m_audio.stopAudio(&error)) {
            return Result::error(-32002, error);
        }
        return Result::success(QJsonObject{{"status", "stopped"}});
    });
}

QString Service::mediaPath(const QString& filename) const
{
    return QDir(m_dataDir).filePath(QStringLiteral("media/%1").arg(filename));
}

} // namespace Services::Audio
