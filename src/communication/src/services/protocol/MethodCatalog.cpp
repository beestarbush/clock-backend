#include "MethodCatalog.h"

namespace Services::Protocol
{

QList<Services::WebSocket::Method> MethodCatalog::allMethods()
{
    using Services::WebSocket::Method;

    return {
        Method::Subscribe,
        Method::Unsubscribe,
        Method::GetConfig,
        Method::SetBrightness,
        Method::SetVolume,
        Method::SetDeviceId,
        Method::UpdateSystemConfig,
        Method::AddApp,
        Method::UpdateApp,
        Method::RemoveApp,
        Method::GetStatus,
        Method::GetProcessorTemperature,
        Method::GetMedia,
        Method::GetAllMedia,
        Method::GetEnvironment,
        Method::Shutdown,
        Method::Reboot,
        Method::PlaySound,
        Method::StopSound,
    };
}

QList<Services::WebSocket::Topic> MethodCatalog::allTopics()
{
    using Services::WebSocket::Topic;

    return {
        Topic::Configuration,
        Topic::Media,
        Topic::ApplicationStatus,
        Topic::ProcessorTemperature,
        Topic::BackendStatus,
        Topic::Environment,
    };
}

} // namespace Services::Protocol
