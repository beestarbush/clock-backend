#ifndef SERVICES_WEBSOCKET_SERVICE_H
#define SERVICES_WEBSOCKET_SERVICE_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QWebSocket>

#include <functional>

#include "Types.h"

namespace Services::WebSocket
{

class Service : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

  public:
    using ResponseCallback = std::function<void(bool success, const QJsonObject& result, const QString& error)>;

    explicit Service(QObject* parent = nullptr);

    QString serverUrl() const;
    bool connected() const;

    void setServerUrl(const QString& url);

    void connectToSocket();
    void disconnectFromSocket();

    void request(const Method& method, const QJsonObject& params, ResponseCallback callback);
    void publish(const Topic& topic, const QJsonObject& params = QJsonObject());
    void subscribe(const Topic& topic);
    void unsubscribe(const Topic& topic);

  signals:
    void serverUrlChanged();
    void connectedChanged();
    void publishReceived(const Topic& topic, const QJsonObject& data);

  private:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void dispatchMessage(const QJsonObject& message);
    void resubscribeAll();

    QWebSocket m_webSocket;
    QString m_serverUrl;
    bool m_connected;
    int m_nextRequestId;
    QHash<QString, ResponseCallback> m_pendingRequests;
    QList<Topic> m_subscribedTopics;
};

} // namespace Services::WebSocket

#endif // SERVICES_WEBSOCKET_SERVICE_H
