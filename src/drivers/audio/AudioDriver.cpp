#include "AudioDriver.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace Drivers::Hardware
{

AudioDriver::AudioDriver(QObject* parent)
    : QObject(parent)
{
}

bool AudioDriver::playAudioFile(const QString& mediaPath, int volume, const QString& mode, QString* error) const
{
    const QFileInfo info(mediaPath);
    if (!info.exists() || !info.isFile()) {
        if (error) {
            *error = QStringLiteral("Audio file not found: %1").arg(mediaPath);
        }
        return false;
    }

    const QString normalizedMode = mode.toLower();
    if (normalizedMode == QStringLiteral("replace")) {
        stopAudio(nullptr);
    }

    const QString ffplay = QStandardPaths::findExecutable(QStringLiteral("ffplay"));
    if (!ffplay.isEmpty()) {
        const bool ok = QProcess::startDetached(ffplay,
                                                {QStringLiteral("-nodisp"),
                                                 QStringLiteral("-autoexit"),
                                                 QStringLiteral("-loglevel"),
                                                 QStringLiteral("error"),
                                                 QStringLiteral("-volume"),
                                                 QString::number(qBound(0, volume, 100)),
                                                 mediaPath});
        if (ok) {
            return true;
        }
    }

    const QString paplay = QStandardPaths::findExecutable(QStringLiteral("paplay"));
    if (!paplay.isEmpty()) {
        if (QProcess::startDetached(paplay, {mediaPath})) {
            return true;
        }
    }

    const QString aplay = QStandardPaths::findExecutable(QStringLiteral("aplay"));
    if (!aplay.isEmpty()) {
        if (QProcess::startDetached(aplay, {mediaPath})) {
            return true;
        }
    }

    // Keep desktop/dev behavior non-fatal when no playback backend is installed.
    return true;
}

bool AudioDriver::stopAudio(QString* error) const
{
    Q_UNUSED(error);

    const QString pkill = QStandardPaths::findExecutable(QStringLiteral("pkill"));
    if (pkill.isEmpty()) {
        return true;
    }

    QProcess::execute(pkill, {QStringLiteral("-f"), QStringLiteral("ffplay")});
    QProcess::execute(pkill, {QStringLiteral("-f"), QStringLiteral("paplay")});
    QProcess::execute(pkill, {QStringLiteral("-f"), QStringLiteral("aplay")});

    return true;
}

} // namespace Drivers::Hardware
