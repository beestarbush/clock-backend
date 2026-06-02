#ifndef APPLICATIONS_CONTAINER_H
#define APPLICATIONS_CONTAINER_H

#include <QObject>

namespace Services
{
class Container;
namespace Ingress
{
class Service;
}
}

namespace Applications
{

class Container : public QObject
{
    Q_OBJECT

  public:
    explicit Container(Services::Container& services, QObject* parent = nullptr);

    bool start(quint16 port = 5000);

  private:
    Services::Ingress::Service& m_ingress;
};

} // namespace Applications

#endif // APPLICATIONS_CONTAINER_H
