#include "Frame.h"

namespace Services::WebSocket
{

QJsonObject Frame::buildRequest(const QJsonValue& id, Method method, const QJsonObject& params)
{
    QJsonObject request;
    request["jsonrpc"] = QStringLiteral("2.0");
    request["type"] = messageTypeToString(MessageType::Request);
    request["method"] = methodToString(method);
    request["params"] = params;
    request["id"] = id;
    return request;
}

QJsonObject Frame::buildResponse(const QJsonValue& id, const QJsonObject& result)
{
    QJsonObject response;
    response["jsonrpc"] = QStringLiteral("2.0");
    response["type"] = messageTypeToString(MessageType::Response);
    response["result"] = result;
    response["id"] = id;
    return response;
}

bool Frame::isRequest(const QJsonObject& message)
{
    return messageTypeFromString(message.value("type").toString()) == MessageType::Request;
}

bool Frame::isResponse(const QJsonObject& message)
{
    return messageTypeFromString(message.value("type").toString()) == MessageType::Response;
}

bool Frame::isPublish(const QJsonObject& message)
{
    return messageTypeFromString(message.value("type").toString()) == MessageType::Publish;
}

QJsonObject Frame::buildErrorResponse(const QJsonValue& id, int code, const QString& message)
{
    QJsonObject response;
    response["jsonrpc"] = QStringLiteral("2.0");
    response["type"] = messageTypeToString(MessageType::Response);

    QJsonObject error;
    error["code"] = code;
    error["message"] = message;

    response["error"] = error;
    response["id"] = id;
    return response;
}

QJsonObject Frame::buildPublish(Topic topic, const QJsonObject& params)
{
    QJsonObject publish;
    publish["jsonrpc"] = QStringLiteral("2.0");
    publish["type"] = messageTypeToString(MessageType::Publish);
    publish["topic"] = topicToString(topic);
    publish["params"] = params;
    return publish;
}

QJsonValue Frame::parseId(const QJsonObject& message)
{
    return message.value("id");
}

bool Frame::isValidId(const QJsonValue& id)
{
    return id.isString() || id.isDouble();
}

Method Frame::parseMethod(const QJsonObject& message)
{
    return methodFromString(message.value("method").toString());
}

Topic Frame::parseTopic(const QJsonObject& message)
{
    return topicFromString(message.value("topic").toString());
}

QJsonObject Frame::parseParams(const QJsonObject& message)
{
    return message.value("params").toObject();
}

QJsonObject Frame::parseResult(const QJsonObject& message)
{
    return message.value("result").toObject();
}

QString Frame::parseErrorMessage(const QJsonObject& message)
{
    return message.value("error").toObject().value("message").toString();
}

} // namespace Services::WebSocket
