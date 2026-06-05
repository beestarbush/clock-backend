#include "Service.h"

#include <QAudioOutput>
#include <QFileInfo>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMediaPlayer>
#include <QUrl>

#include "services/configuration/Service.h"
#include "websocket/server/Service.h"

#include <algorithm>

Q_LOGGING_CATEGORY(AudioService, "AudioService")

namespace Services::Audio
{
#ifdef PLATFORM_IS_TARGET
const QString MEDIA_DIR = QStringLiteral("./media");
#else
const QString MEDIA_DIR = QStringLiteral("/workdir/data/media");
#endif

Service::Service(Services::Configuration::Service& configuration,
                 Common::Communication::WebSocket::Server::Service& websocket,
                 QObject* parent)
    : QObject(parent),
      m_configuration(configuration),
      m_queuePlayer(new QMediaPlayer(this)),
      m_queueOutput(new QAudioOutput(this))
{
    if (!setVolume(m_configuration.volume())) {
        qWarning(AudioService) << QStringLiteral("Failed to apply configured volume.");
    }

    connect(m_queuePlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::NoMedia) {
            startNextQueuedPlayback();
        }
    });

    connect(m_queuePlayer,
            &QMediaPlayer::errorOccurred,
            this,
            [this](QMediaPlayer::Error, const QString&) {
                startNextQueuedPlayback();
            });

    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;
    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::SetVolume, [this](const QJsonObject& params) {
        const QJsonValue valueParam = params.value("value");
        if (!valueParam.isDouble()) {
            return Result::error(-32000, QStringLiteral("Volume value must be an integer between 0 and 100"));
        }

        const int requestedValue = valueParam.toInt();
        if (requestedValue < 0 || requestedValue > 100) {
            return Result::error(-32000, QStringLiteral("Volume value must be between 0 and 100"));
        }

        if (!setVolume(requestedValue)) {
            return Result::error(-32000, QStringLiteral("Failed to set volume"));
        }

        const QJsonObject configResult = m_configuration.setVolume(requestedValue);
        if (configResult.contains("__error")) {
            return Result::error(-32000, configResult.value("__error").toString());
        }

        return Result::success(QJsonObject{{"volume", requestedValue}});
    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::PlaySound,
                                    [this](const QJsonObject& params) {
                                        const QString filename = params.value("filename").toString();
                                        const QString mode = params.value("mode").toString(QStringLiteral("concurrent"));

                                        if (filename.isEmpty()) {
                                            return Result::error(-32001, QStringLiteral("Missing filename"));
                                        }

                                        const QString normalizedMode = mode.toLower();
                                        if (normalizedMode != QStringLiteral("concurrent") &&
                                            normalizedMode != QStringLiteral("queue") &&
                                            normalizedMode != QStringLiteral("replace")) {
                                            return Result::error(-32001, QStringLiteral("Invalid play mode"));
                                        }

                                        if (!playAudioFile(MEDIA_DIR + QStringLiteral("/%1").arg(filename), normalizedMode)) {
                                            return Result::error(-32001, QStringLiteral("Failed to play audio file"));
                                        }
                                        return Result::success(QJsonObject{{"status", "played"}});
                                    });

    websocket.registerMethodHandler(Common::Communication::WebSocket::Method::StopSound, [this](const QJsonObject&) {
        if (!stopAudio()) {
            return Result::error(-32002, QStringLiteral("Failed to stop audio"));
        }
        return Result::success(QJsonObject{{"status", "stopped"}});
    });
}

bool Service::setVolume(quint8 value)
{
    quint8 clamped = std::clamp(value, static_cast<quint8>(0), static_cast<quint8>(100));
    qCDebug(AudioService) << "Setting volume to" << clamped;

    const float normalized = static_cast<float>(clamped) / 100.0f;
    if (m_queueOutput) {
        m_queueOutput->setVolume(normalized);
    }

    for (const ActivePlayback& playback : std::as_const(m_concurrentPlaybacks)) {
        if (playback.output) {
            playback.output->setVolume(normalized);
        }
    }
    return true;
}

void Service::startNextQueuedPlayback()
{
    if (m_queuePlayer->playbackState() == QMediaPlayer::PlayingState) {
        return;
    }

    while (!m_queue.isEmpty()) {
        const QString nextPath = m_queue.dequeue();
        if (!QFileInfo::exists(nextPath)) {
            qCWarning(AudioService) << "Queued audio file does not exist:" << nextPath;
            continue;
        }

        m_queuePlayer->setSource(QUrl::fromLocalFile(nextPath));
        m_queuePlayer->play();
        return;
    }
}

bool Service::startConcurrentPlayback(const QString& mediaPath)
{
    auto* output = new QAudioOutput(this);
    output->setVolume(static_cast<float>(m_configuration.volume()) / 100.0f);

    auto* player = new QMediaPlayer(this);
    player->setAudioOutput(output);

    const auto cleanup = [this, player, output]() {
        for (qsizetype i = 0; i < m_concurrentPlaybacks.size(); ++i) {
            if (m_concurrentPlaybacks[i].player == player) {
                m_concurrentPlaybacks.removeAt(i);
                break;
            }
        }

        player->deleteLater();
        output->deleteLater();
    };

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [cleanup](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::NoMedia) {
            cleanup();
        }
    });

    connect(player,
            &QMediaPlayer::errorOccurred,
            this,
            [cleanup](QMediaPlayer::Error, const QString&) {
                cleanup();
            });

    m_concurrentPlaybacks.append({player, output});
    player->setSource(QUrl::fromLocalFile(mediaPath));
    player->play();

    return true;
}

void Service::clearQueue()
{
    m_queue.clear();
    m_queuePlayer->stop();
}

bool Service::playAudioFile(const QString& mediaPath, const QString& mode)
{
    const QFileInfo info(mediaPath);
    if (!info.exists() || !info.isFile()) {
        qCWarning(AudioService) << "Audio file does not exist:" << mediaPath;
        return false;
    }

    const QString normalizedMode = mode.toLower();

    if (normalizedMode == QStringLiteral("replace")) {
        stopAudio();
        m_queue.enqueue(mediaPath);
        startNextQueuedPlayback();
        return true;
    }

    if (normalizedMode == QStringLiteral("queue")) {
        m_queue.enqueue(mediaPath);
        startNextQueuedPlayback();
        return true;
    }

    if (normalizedMode == QStringLiteral("concurrent")) {
        return startConcurrentPlayback(mediaPath);
    }

    qCWarning(AudioService) << "Unsupported play mode" << mode;
    return false;
}

bool Service::stopAudio()
{
    clearQueue();

    for (const ActivePlayback& playback : std::as_const(m_concurrentPlaybacks)) {
        if (playback.player) {
            playback.player->stop();
            playback.player->deleteLater();
        }
        if (playback.output) {
            playback.output->deleteLater();
        }
    }
    m_concurrentPlaybacks.clear();

    return true;
}
} // namespace Services::Audio
