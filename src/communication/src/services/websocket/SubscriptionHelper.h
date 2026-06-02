#ifndef SERVICES_WEBSOCKET_SUBSCRIPTIONHELPER_H
#define SERVICES_WEBSOCKET_SUBSCRIPTIONHELPER_H

#include <QMetaObject>
#include <QObject>
#include <QTimer>

#include <functional>

#include "Service.h"

namespace Services::WebSocket
{

class SubscriptionHelper : public QObject
{
    Q_OBJECT

  public:
    using DataHandler = std::function<void(const QJsonObject& data)>;

    explicit SubscriptionHelper(Service& webSocket, QObject* parent = nullptr);

    void setStartupTimeoutMs(int timeoutMs);
    void start(const Topic& topic, Method bootstrapMethod, DataHandler handler);
    void stop();

  signals:
    void startupCompleted();
    void startupTimedOut();

  private:
    void performBootstrap();
    void completeStartup();

    Service& m_webSocket;
    QTimer m_startupTimeoutTimer;
    QMetaObject::Connection m_connectionWatcher;

    bool m_startupInProgress;
    Topic m_topic;
    Method m_bootstrapMethod;
    DataHandler m_handler;
};

} // namespace Services::WebSocket

#endif // SERVICES_WEBSOCKET_SUBSCRIPTIONHELPER_H
