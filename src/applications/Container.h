#ifndef APPLICATIONS_CONTAINER_H
#define APPLICATIONS_CONTAINER_H

#include <QObject>

namespace Services
{
class Container;
}

namespace Applications
{
class Container : public QObject
{
    Q_OBJECT

  public:
    explicit Container(Services::Container& services, QObject* parent = nullptr);
};

} // namespace Applications

#endif // APPLICATIONS_CONTAINER_H
