#include "BrightnessDriver.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <algorithm>

namespace Drivers::Hardware
{

namespace
{
constexpr int PWM_MAX = 31;
const QString BRIGHTNESS_SYSFS = QStringLiteral("/sys/class/backlight/11-0045/brightness");
}

BrightnessDriver::BrightnessDriver(QObject* parent)
    : QObject(parent)
{
}

bool BrightnessDriver::setBrightness(int value, const QString& dataDir, QString* error) const
{
    const int clamped = std::clamp(value, 0, 100);
    const int pwmVal = (clamped * PWM_MAX) / 100;

    QFile sysfs(BRIGHTNESS_SYSFS);
    if (sysfs.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&sysfs);
        out << pwmVal;
        return true;
    }

    QFile mock(QDir(dataDir).filePath("brightness"));
    if (mock.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&mock);
        out << pwmVal;
        return true;
    }

    if (error) {
        *error = QStringLiteral("Failed to set brightness");
    }
    return false;
}

} // namespace Drivers::Hardware
