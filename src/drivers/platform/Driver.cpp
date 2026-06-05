#include "Driver.h"

#include <QProcess>
#include <QStandardPaths>

namespace Drivers::Platform
{

Driver::Driver(QObject* parent)
    : QObject(parent)
{
}

bool Driver::shutdown(QString* error) const
{
    const QString shutdownCmd = QStandardPaths::findExecutable(QStringLiteral("shutdown"));
    if (shutdownCmd.isEmpty()) {
        return true;
    }

    if (QProcess::startDetached(shutdownCmd, {QStringLiteral("-h"), QStringLiteral("now")})) {
        return true;
    }

    if (error) {
        *error = QStringLiteral("Failed to execute shutdown command");
    }
    return false;
}

bool Driver::reboot(QString* error) const
{
    const QString rebootCmd = QStandardPaths::findExecutable(QStringLiteral("reboot"));
    if (rebootCmd.isEmpty()) {
        return true;
    }

    if (QProcess::startDetached(rebootCmd, {})) {
        return true;
    }

    if (error) {
        *error = QStringLiteral("Failed to execute reboot command");
    }
    return false;
}

} // namespace Drivers::Platform
