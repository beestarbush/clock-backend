#include "Container.h"

#include "services/Container.h"

namespace Applications
{

Container::Container(Services::Container& services, QObject* parent)
    : QObject(parent),
            m_ingress(services.m_ingress)
{
}

bool Container::start(quint16 port)
{
        return m_ingress.start(port);
}

} // namespace Applications
