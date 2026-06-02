#ifndef SERVICES_WEBSOCKET_TYPES_H
#define SERVICES_WEBSOCKET_TYPES_H

#include <QString>

namespace Services::WebSocket
{
Q_NAMESPACE

enum class MessageType
{
    Request,
    Response,
    Publish,
    UnknownMessageType
};
Q_ENUM_NS(MessageType)

enum class Method
{
    Subscribe,
    Unsubscribe,
    GetConfig,
    SetBrightness,
    SetVolume,
    SetDeviceId,
    UpdateSystemConfig,
    AddApp,
    UpdateApp,
    RemoveApp,
    GetStatus,
    GetProcessorTemperature,
    GetMedia,
    GetAllMedia,
    GetEnvironment,
    Shutdown,
    Reboot,
    PlaySound,
    StopSound,
    UnknownMethod
};
Q_ENUM_NS(Method)

enum class Topic
{
    Configuration,
    Media,
    ApplicationStatus,
    ProcessorTemperature,
    BackendStatus,
    Environment,
    UnknownTopic
};
Q_ENUM_NS(Topic)

enum class PlayMode
{
    Concurrent,
    Queue,
    Replace
};
Q_ENUM_NS(PlayMode)

inline MessageType messageTypeFromString(const QString& typeStr)
{
    if (typeStr == "request") {
        return MessageType::Request;
    }
    if (typeStr == "response") {
        return MessageType::Response;
    }
    if (typeStr == "publish") {
        return MessageType::Publish;
    }
    return MessageType::UnknownMessageType;
}

inline QString messageTypeToString(MessageType type)
{
    switch (type) {
    case MessageType::Request:
        return "request";
    case MessageType::Response:
        return "response";
    case MessageType::Publish:
        return "publish";
    default:
        return "unknown";
    }
}

inline Method methodFromString(const QString& methodStr)
{
    if (methodStr == "subscribe") {
        return Method::Subscribe;
    }
    if (methodStr == "unsubscribe") {
        return Method::Unsubscribe;
    }
    if (methodStr == "getConfig") {
        return Method::GetConfig;
    }
    if (methodStr == "setBrightness") {
        return Method::SetBrightness;
    }
    if (methodStr == "setVolume") {
        return Method::SetVolume;
    }
    if (methodStr == "setDeviceId") {
        return Method::SetDeviceId;
    }
    if (methodStr == "updateSystemConfig") {
        return Method::UpdateSystemConfig;
    }
    if (methodStr == "addApp") {
        return Method::AddApp;
    }
    if (methodStr == "updateApp") {
        return Method::UpdateApp;
    }
    if (methodStr == "removeApp") {
        return Method::RemoveApp;
    }
    if (methodStr == "getStatus") {
        return Method::GetStatus;
    }
    if (methodStr == "getProcessorTemperature") {
        return Method::GetProcessorTemperature;
    }
    if (methodStr == "getMedia") {
        return Method::GetMedia;
    }
    if (methodStr == "getAllMedia") {
        return Method::GetAllMedia;
    }
    if (methodStr == "getEnvironment") {
        return Method::GetEnvironment;
    }
    if (methodStr == "shutdown") {
        return Method::Shutdown;
    }
    if (methodStr == "reboot") {
        return Method::Reboot;
    }
    if (methodStr == "playSound") {
        return Method::PlaySound;
    }
    if (methodStr == "stopSound") {
        return Method::StopSound;
    }
    return Method::UnknownMethod;
}

inline QString methodToString(Method type)
{
    switch (type) {
    case Method::Subscribe:
        return "subscribe";
    case Method::Unsubscribe:
        return "unsubscribe";
    case Method::GetConfig:
        return "getConfig";
    case Method::SetBrightness:
        return "setBrightness";
    case Method::SetVolume:
        return "setVolume";
    case Method::SetDeviceId:
        return "setDeviceId";
    case Method::UpdateSystemConfig:
        return "updateSystemConfig";
    case Method::AddApp:
        return "addApp";
    case Method::UpdateApp:
        return "updateApp";
    case Method::RemoveApp:
        return "removeApp";
    case Method::GetStatus:
        return "getStatus";
    case Method::GetProcessorTemperature:
        return "getProcessorTemperature";
    case Method::GetMedia:
        return "getMedia";
    case Method::GetAllMedia:
        return "getAllMedia";
    case Method::GetEnvironment:
        return "getEnvironment";
    case Method::Shutdown:
        return "shutdown";
    case Method::Reboot:
        return "reboot";
    case Method::PlaySound:
        return "playSound";
    case Method::StopSound:
        return "stopSound";
    default:
        return "unknown";
    }
}

inline Topic topicFromString(const QString& topicStr)
{
    if (topicStr == "configuration") {
        return Topic::Configuration;
    }
    if (topicStr == "media") {
        return Topic::Media;
    }
    if (topicStr == "application-status") {
        return Topic::ApplicationStatus;
    }
    if (topicStr == "processor-temperature") {
        return Topic::ProcessorTemperature;
    }
    if (topicStr == "backend-status") {
        return Topic::BackendStatus;
    }
    if (topicStr == "environment") {
        return Topic::Environment;
    }
    return Topic::UnknownTopic;
}

inline QString topicToString(Topic topic)
{
    switch (topic) {
    case Topic::Configuration:
        return "configuration";
    case Topic::Media:
        return "media";
    case Topic::ApplicationStatus:
        return "application-status";
    case Topic::ProcessorTemperature:
        return "processor-temperature";
    case Topic::BackendStatus:
        return "backend-status";
    case Topic::Environment:
        return "environment";
    default:
        return "unknown";
    }
}

inline QString playModeToString(PlayMode mode)
{
    switch (mode) {
    case PlayMode::Concurrent:
        return "concurrent";
    case PlayMode::Queue:
        return "queue";
    case PlayMode::Replace:
        return "replace";
    default:
        return "unknown";
    }
}

} // namespace Services::WebSocket

#endif // SERVICES_WEBSOCKET_TYPES_H
