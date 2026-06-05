#ifndef SERVICES_ENVIRONMENT_SERVICE_H
#define SERVICES_ENVIRONMENT_SERVICE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

namespace Common::Communication::WebSocket::Server
{
class Service;
}

namespace Services::Environment
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(Common::Communication::WebSocket::Server::Service& websocket,
                     QObject* parent = nullptr);
    ~Service() override;

    void start();
    QJsonObject environment() const;

  private:
    void refreshLoop();
    std::optional<double> readDoubleFile(const QString& path) const;
    QJsonObject processorTemperature() const;
    QJsonObject refreshEnvironment();

    mutable std::mutex m_environmentMutex;
    QJsonObject m_environment;
    std::atomic<bool> m_stopRequested{false};
    std::thread m_refreshThread;
};

} // namespace Services::Environment

#endif // SERVICES_ENVIRONMENT_SERVICE_H
