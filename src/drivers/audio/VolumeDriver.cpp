#include "VolumeDriver.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

namespace Drivers::Hardware
{

VolumeDriver::VolumeDriver(QObject* parent)
    : QObject(parent)
{
}

bool VolumeDriver::setVolume(int value, QString* error) const
{
    const int clamped = std::clamp(value, 0, 100);

    const QString amixer = QStandardPaths::findExecutable(QStringLiteral("amixer"));
    if (!amixer.isEmpty()) {
        QProcess process;
        process.start(amixer, {QStringLiteral("-q"), QStringLiteral("sset"), QStringLiteral("Master"), QStringLiteral("%1%").arg(clamped)});
        if (process.waitForFinished(3000) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            return true;
        }
    }

    QFile mock(QDir::current().filePath(QStringLiteral("volume")));
    if (mock.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&mock);
        out << clamped;
        return true;
    }

    if (error) {
        *error = QStringLiteral("Failed to set volume");
    }
    return false;
}

} // namespace Drivers::Hardware
