#ifndef SERVICES_WEBSOCKET_FRAME_H
#define SERVICES_WEBSOCKET_FRAME_H

#include <QJsonObject>
#include <QString>

#include "Types.h"

namespace Services::WebSocket
{

class Frame
{
  public:
    static QJsonObject buildRequest(const QString& id, Method method, const QJsonObject& params = QJsonObject());
    static QJsonObject buildResponse(const QString& id, const QJsonObject& result = QJsonObject());
    static QJsonObject buildErrorResponse(const QString& id, int code, const QString& message);
    static QJsonObject buildPublish(Topic topic, const QJsonObject& params = QJsonObject());

    static bool isRequest(const QJsonObject& message);
    static bool isResponse(const QJsonObject& message);
    static bool isPublish(const QJsonObject& message);

    static QString parseId(const QJsonObject& message);
    static Method parseMethod(const QJsonObject& message);
    static Topic parseTopic(const QJsonObject& message);
    static QJsonObject parseParams(const QJsonObject& message);
    static QJsonObject parseResult(const QJsonObject& message);
    static QString parseErrorMessage(const QJsonObject& message);
};

} // namespace Services::WebSocket

#endif // SERVICES_WEBSOCKET_FRAME_H
