#include "Container.h"

#include "services/Container.h"

namespace Applications
{

Container::Container(Services::Container& services, QObject* parent)
    : QObject(parent)
{
}

} // namespace Applications
