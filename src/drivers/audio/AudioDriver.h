#ifndef DRIVERS_HARDWARE_AUDIO_DRIVER_H
#define DRIVERS_HARDWARE_AUDIO_DRIVER_H

#include <QObject>
#include <QString>

namespace Drivers::Hardware
{

class AudioDriver : public QObject
{
    Q_OBJECT

  public:
    explicit AudioDriver(QObject* parent = nullptr);

    bool playAudioFile(const QString& mediaPath, int volume, const QString& mode, QString* error = nullptr) const;
    bool stopAudio(QString* error = nullptr) const;
};

} // namespace Drivers::Hardware

#endif //DRIVERS_HARDWARE_AUDIO_DRIVER_H
