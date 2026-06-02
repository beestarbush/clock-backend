#ifndef SERVICES_WEBSOCKET_SERVICE_H
#define SERVICES_WEBSOCKET_SERVICE_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QWebSocket)
QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QTcpSocket)

#include "services/websocket/Types.h"

namespace Services::WebSocket
{

class Service : public QObject
{
    Q_OBJECT

  public:
  struct MethodResult
  {
    bool ok;
    QJsonObject payload;
    int errorCode;
    QString errorMessage;

    static MethodResult success(const QJsonObject& result = QJsonObject())
    {
      return MethodResult{true, result, 0, {}};
    }

    static MethodResult error(int code, const QString& message)
    {
      return MethodResult{false, {}, code, message};
    }
  };

  using MethodHandler = std::function<MethodResult(const QJsonObject& params)>;
    using PublishHandler = std::function<void(const QJsonObject& params)>;
    using TopicPublisher = std::function<QJsonObject()>;

    explicit Service(QObject* parent = nullptr);
    ~Service() override;

    bool start(quint16 port = 5000);
    bool start(const QList<quint16>& ports);
    void attachUpgradedSocket(QTcpSocket* socket);

    void registerMethodHandler(::Services::WebSocket::Method method, MethodHandler handler);
    void registerPublishHandler(::Services::WebSocket::Topic topic, PublishHandler handler);
    void registerPeriodicPublisher(::Services::WebSocket::Topic topic, TopicPublisher publisher);

    void publish(::Services::WebSocket::Topic topic, const QJsonObject& params);

    QJsonObject processRequestForTest(const QString& id,
                                      ::Services::WebSocket::Method method,
                                      const QJsonObject& params = QJsonObject());

  private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onSocketDisconnected();
    void onPeriodicPublishTick();

  private:
    QJsonObject processRequest(const QString& id,
                               ::Services::WebSocket::Method method,
                               const QJsonObject& params,
                               QSet<::Services::WebSocket::Topic>* subscriptions = nullptr);

    void handlePublish(const QJsonObject& message);
    void sendJson(QWebSocket* socket, const QJsonObject& message);
    void publishToSubscribed(::Services::WebSocket::Topic topic, const QJsonObject& params);
    void ensureActive();

    QWebSocketServer* m_handoffServer;
    QList<QWebSocketServer*> m_servers;
    QList<QWebSocket*> m_clients;
    QHash<QWebSocket*, QSet<::Services::WebSocket::Topic>> m_subscriptions;
    QHash<int, MethodHandler> m_methodHandlers;
    QHash<int, PublishHandler> m_publishHandlers;
    QHash<int, TopicPublisher> m_periodicPublishers;
    QTimer m_periodicPublishTimer;
};

} // namespace Services::WebSocket

#endif //SERVICES_WEBSOCKET_SERVICE_H
