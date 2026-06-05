#ifndef DRIVERS_CONTAINER_H
#define DRIVERS_CONTAINER_H

#include <QObject>

#include "platform/Driver.h"

namespace Services
{
class Container;
}

namespace Drivers
{

class Container : public QObject
{
    Q_OBJECT

  public:
    explicit Container(QObject* parent = nullptr);

    friend class ::Services::Container;

  private:
    Platform::Driver m_power;
};

} // namespace Drivers

#endif // DRIVERS_CONTAINER_H
