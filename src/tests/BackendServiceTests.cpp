#include <QtTest>

#include <QTemporaryDir>

#include "drivers/audio/VolumeDriver.h"
#include "drivers/display/BrightnessDriver.h"
#include "services/configuration/Service.h"
#include "services/websocket/Service.h"

namespace
{

void registerTestHandlers(Services::WebSocket::Service& service, Services::Configuration::Service& configuration)
{
    using Result = Services::WebSocket::Service::MethodResult;

    service.registerMethodHandler(Services::WebSocket::Method::GetConfig, [&configuration](const QJsonObject&) {
        return Result::success(configuration.asJson());
    });

    service.registerMethodHandler(Services::WebSocket::Method::SetDeviceId, [&configuration](const QJsonObject& params) {
        return Result::success(configuration.setDeviceId(params.value("device_id").toString()));
    });

    service.registerMethodHandler(Services::WebSocket::Method::SetBrightness, [&configuration](const QJsonObject& params) {
        const QJsonObject result = configuration.setBrightness(params.value("value").toInt());
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
    void testSetBrightnessClampsAndReturnsValue();
    void testUnknownMethodReturnsError();
};

void BackendServiceTests::testGetConfigReturnsFrame()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Drivers::Hardware::BrightnessDriver brightness;
    Drivers::Hardware::VolumeDriver volume;
    Services::Configuration::Service configuration(brightness, volume, dir.path());
    QVERIFY(configuration.load());
    Services::WebSocket::Service service;
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest("1", Services::WebSocket::Method::GetConfig);
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

    Drivers::Hardware::BrightnessDriver brightness;
    Drivers::Hardware::VolumeDriver volume;
    Services::Configuration::Service configuration(brightness, volume, dir.path());
    QVERIFY(configuration.load());
    Services::WebSocket::Service service;
    registerTestHandlers(service, configuration);

    const QJsonObject setResponse = service.processRequestForTest(
        "2",
        Services::WebSocket::Method::SetDeviceId,
        QJsonObject{{"device_id", "SN-NEW-123"}});

    QCOMPARE(setResponse.value("result").toObject().value("device_id").toString(), QString("SN-NEW-123"));

    const QJsonObject getResponse = service.processRequestForTest("3", Services::WebSocket::Method::GetConfig);
    QCOMPARE(getResponse.value("result").toObject().value("device_id").toString(), QString("SN-NEW-123"));
}

void BackendServiceTests::testSetBrightnessClampsAndReturnsValue()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Drivers::Hardware::BrightnessDriver brightness;
    Drivers::Hardware::VolumeDriver volume;
    Services::Configuration::Service configuration(brightness, volume, dir.path());
    QVERIFY(configuration.load());
    Services::WebSocket::Service service;
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest(
        "4",
        Services::WebSocket::Method::SetBrightness,
        QJsonObject{{"value", 999}});

    QCOMPARE(response.value("result").toObject().value("brightness").toInt(), 100);
}

void BackendServiceTests::testUnknownMethodReturnsError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Drivers::Hardware::BrightnessDriver brightness;
    Drivers::Hardware::VolumeDriver volume;
    Services::Configuration::Service configuration(brightness, volume, dir.path());
    QVERIFY(configuration.load());
    Services::WebSocket::Service service;
    registerTestHandlers(service, configuration);

    const QJsonObject response = service.processRequestForTest(
        "5",
        Services::WebSocket::Method::UnknownMethod,
        QJsonObject());

    const QJsonObject error = response.value("error").toObject();
    QCOMPARE(error.value("code").toInt(), -32601);
    QCOMPARE(error.value("message").toString(), QString("Method not found"));
}

QTEST_MAIN(BackendServiceTests)
#include "BackendServiceTests.moc"
