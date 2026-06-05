#include <QtTest>

#include <QDir>
#include <QScopeGuard>
#include <QTemporaryDir>

#include <algorithm>

#include "services/configuration/Service.h"
#include "websocket/server/Service.h"

namespace
{

QString useIsolatedWorkingDir(const QTemporaryDir& dir)
{
    const QString previous = QDir::currentPath();

    QDir root(dir.path());
    root.mkpath(QStringLiteral("build"));
    root.mkpath(QStringLiteral("data"));
    QDir::setCurrent(root.filePath(QStringLiteral("build")));

    return previous;
}

void registerTestHandlers(Common::Communication::WebSocket::Server::Service& service, Services::Configuration::Service& configuration)
{
    using Result = Common::Communication::WebSocket::Server::Service::MethodResult;

    service.registerMethodHandler(Common::Communication::WebSocket::Method::GetConfig, [&configuration](const QJsonObject&) {
        return Result::success(configuration.asJson());
    });

    service.registerMethodHandler(Common::Communication::WebSocket::Method::SetDeviceId, [&configuration](const QJsonObject& params) {
        return Result::success(configuration.setDeviceId(params.value("device_id").toString()));
    });

    service.registerMethodHandler(Common::Communication::WebSocket::Method::SetBrightness, [&configuration](const QJsonObject& params) {
        const QJsonValue valueParam = params.value("value");
        if (!valueParam.isDouble()) {
            return Result::error(-32000, QStringLiteral("Brightness value must be an integer between 0 and 100"));
        }

        const int requestedValue = params.value("value").toInt();
        if (requestedValue < 0 || requestedValue > 100) {
            return Result::error(-32000, QStringLiteral("Brightness value must be between 0 and 100"));
        }

        const quint8 brightness = static_cast<quint8>(requestedValue);
        const QJsonObject result = configuration.setBrightness(brightness);
        if (result.contains("__error")) {
            return Result::error(-32000, result.value("__error").toString());
        }
        return Result::success(result);
    });
}

} // namespace

class BackendServiceTests : public QObject
{
    Q_OBJECT

  private slots:
    void testGetConfigReturnsFrame();
    void testSetDeviceIdUpdatesConfiguration();
    void testSetBrightnessOutOfBoundsReturnsError();
    void testUnknownMethodReturnsError();
};

void BackendServiceTests::testGetConfigReturnsFrame()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString previousDir = useIsolatedWorkingDir(dir);
    const auto restoreDir = qScopeGuard([&previousDir]() {
        QDir::setCurrent(previousDir);
    });

    Common::Communication::WebSocket::Server::Service service;
    Services::Configuration::Service configuration(service);
    QVERIFY(configuration.load());
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest("1", Common::Communication::WebSocket::Method::GetConfig);
    QCOMPARE(response.value("type").toString(), QString("response"));
    QCOMPARE(response.value("id").toString(), QString("1"));

    const QJsonObject result = response.value("result").toObject();
    QVERIFY(result.contains("system-configuration"));
    QVERIFY(result.contains("applications"));
    QVERIFY(result.contains("device_id"));
}

void BackendServiceTests::testSetDeviceIdUpdatesConfiguration()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString previousDir = useIsolatedWorkingDir(dir);
    const auto restoreDir = qScopeGuard([&previousDir]() {
        QDir::setCurrent(previousDir);
    });

    Common::Communication::WebSocket::Server::Service service;
    Services::Configuration::Service configuration(service);
    QVERIFY(configuration.load());
    registerTestHandlers(service, configuration);

    const QJsonObject setResponse = service.processRequestForTest(
        "2",
        Common::Communication::WebSocket::Method::SetDeviceId,
        QJsonObject{{"device_id", "SN-NEW-123"}});

    QCOMPARE(setResponse.value("result").toObject().value("device_id").toString(), QString("SN-NEW-123"));

    const QJsonObject getResponse = service.processRequestForTest("3", Common::Communication::WebSocket::Method::GetConfig);
    QCOMPARE(getResponse.value("result").toObject().value("device_id").toString(), QString("SN-NEW-123"));
}

void BackendServiceTests::testSetBrightnessOutOfBoundsReturnsError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString previousDir = useIsolatedWorkingDir(dir);
    const auto restoreDir = qScopeGuard([&previousDir]() {
        QDir::setCurrent(previousDir);
    });

    Common::Communication::WebSocket::Server::Service service;
    Services::Configuration::Service configuration(service);
    QVERIFY(configuration.load());
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest(
        "4",
        Common::Communication::WebSocket::Method::SetBrightness,
        QJsonObject{{"value", 999}});

    const QJsonObject error = response.value("error").toObject();
    QCOMPARE(error.value("code").toInt(), -32000);
    QCOMPARE(error.value("message").toString(), QString("Brightness value must be between 0 and 100"));
}

void BackendServiceTests::testUnknownMethodReturnsError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString previousDir = useIsolatedWorkingDir(dir);
    const auto restoreDir = qScopeGuard([&previousDir]() {
        QDir::setCurrent(previousDir);
    });

    Common::Communication::WebSocket::Server::Service service;
    Services::Configuration::Service configuration(service);
    QVERIFY(configuration.load());
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest(
        "5",
        Common::Communication::WebSocket::Method::UnknownMethod,
        QJsonObject());

    const QJsonObject error = response.value("error").toObject();
    QCOMPARE(error.value("code").toInt(), -32601);
    QCOMPARE(error.value("message").toString(), QString("Method not found"));
}

QTEST_MAIN(BackendServiceTests)
#include "BackendServiceTests.moc"
