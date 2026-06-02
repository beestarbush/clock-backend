#ifndef SERVICES_PROTOCOL_METHODCATALOG_H
#define SERVICES_PROTOCOL_METHODCATALOG_H

#include <QList>

#include "services/websocket/Types.h"

namespace Services::Protocol
{

class MethodCatalog
{
  public:
    static QList<Services::WebSocket::Method> allMethods();
    static QList<Services::WebSocket::Topic> allTopics();
};

} // namespace Services::Protocol

#endif // SERVICES_PROTOCOL_METHODCATALOG_H
