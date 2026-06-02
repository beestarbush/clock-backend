#include "SubscriptionHelper.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(SubscriptionHelperService, "CommunicationSubscriptionHelper")

namespace Services::WebSocket
{

constexpr int DEFAULT_STARTUP_TIMEOUT_MS = 10000;

SubscriptionHelper::SubscriptionHelper(Service& webSocket, QObject* parent)
    : QObject(parent),
      m_webSocket(webSocket),
      m_startupInProgress(false),
      m_topic(Topic::UnknownTopic),
      m_bootstrapMethod(Method::UnknownMethod)
{
    m_startupTimeoutTimer.setSingleShot(true);
    m_startupTimeoutTimer.setInterval(DEFAULT_STARTUP_TIMEOUT_MS);

    connect(&m_startupTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_startupInProgress) {
            return;
        }

        qCWarning(SubscriptionHelperService) << "Startup bootstrap timed out for topic" << topicToString(m_topic);
        m_startupInProgress = false;
        emit startupTimedOut();
    });

    connect(&m_webSocket, &Service::publishReceived, this, [this](const Topic& topic, const QJsonObject& data) {
        if (topic != m_topic || !m_handler) {
            return;
        }

        m_handler(data);
    });
}

void SubscriptionHelper::setStartupTimeoutMs(int timeoutMs)
{
    m_startupTimeoutTimer.setInterval(timeoutMs);
}

void SubscriptionHelper::start(const Topic& topic, Method bootstrapMethod, DataHandler handler)
{
    m_topic = topic;
    m_bootstrapMethod = bootstrapMethod;
    m_handler = std::move(handler);

    m_webSocket.subscribe(m_topic);

    m_startupInProgress = true;
    m_startupTimeoutTimer.start();

    if (m_webSocket.connected()) {
        performBootstrap();
        return;
    }

    m_connectionWatcher = connect(&m_webSocket, &Service::connectedChanged, this, [this]() {
        if (m_webSocket.connected() && m_startupInProgress) {
            disconnect(m_connectionWatcher);
            performBootstrap();
        }
    });
}

void SubscriptionHelper::stop()
{
    m_startupTimeoutTimer.stop();
    disconnect(m_connectionWatcher);
    m_startupInProgress = false;

    if (m_topic != Topic::UnknownTopic) {
        m_webSocket.unsubscribe(m_topic);
    }

    m_topic = Topic::UnknownTopic;
    m_bootstrapMethod = Method::UnknownMethod;
    m_handler = nullptr;
}

void SubscriptionHelper::performBootstrap()
{
    if (m_bootstrapMethod == Method::UnknownMethod) {
        completeStartup();
        return;
    }

    m_webSocket.request(m_bootstrapMethod, QJsonObject(), [this](bool success, const QJsonObject& result, const QString& error) {
        if (!success) {
            qCWarning(SubscriptionHelperService) << "Bootstrap request failed:" << error;
            return;
        }

        if (m_handler) {
            m_handler(result);
        }
        completeStartup();
    });
}

void SubscriptionHelper::completeStartup()
{
    if (!m_startupInProgress) {
        return;
    }

    m_startupTimeoutTimer.stop();
    disconnect(m_connectionWatcher);
    m_startupInProgress = false;
    emit startupCompleted();
}

} // namespace Services::WebSocket
