#include <QtTest>

#include "services/protocol/MethodCatalog.h"
#include "services/websocket/Frame.h"
#include "services/websocket/Types.h"

class CommunicationUnitTests : public QObject
{
    Q_OBJECT

  private slots:
    void testMessageTypeMapping();
    void testMethodMappingRoundTrip();
    void testTopicMappingRoundTrip();
    void testPlayModeToString();

    void testBuildRequestFrame();
    void testBuildResponseFrame();
    void testBuildErrorResponseFrame();
    void testBuildPublishFrame();
    void testFrameParsers();

    void testMethodCatalogCoverage();
    void testTopicCatalogCoverage();
};

void CommunicationUnitTests::testMessageTypeMapping()
{
    using namespace Services::WebSocket;

    QCOMPARE(messageTypeFromString("request"), MessageType::Request);
    QCOMPARE(messageTypeFromString("response"), MessageType::Response);
    QCOMPARE(messageTypeFromString("publish"), MessageType::Publish);
    QCOMPARE(messageTypeFromString("invalid"), MessageType::UnknownMessageType);

    QCOMPARE(messageTypeToString(MessageType::Request), QString("request"));
    QCOMPARE(messageTypeToString(MessageType::Response), QString("response"));
    QCOMPARE(messageTypeToString(MessageType::Publish), QString("publish"));
    QCOMPARE(messageTypeToString(MessageType::UnknownMessageType), QString("unknown"));
}

void CommunicationUnitTests::testMethodMappingRoundTrip()
{
    using namespace Services::WebSocket;

    const QList<Method> methods = {
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

    for (const Method method : methods) {
        const QString wireName = methodToString(method);
        QVERIFY2(wireName != "unknown", "Method produced unknown wire name");
        QCOMPARE(methodFromString(wireName), method);
    }

    QCOMPARE(methodFromString("invalidMethod"), Method::UnknownMethod);
    QCOMPARE(methodToString(Method::UnknownMethod), QString("unknown"));
}

void CommunicationUnitTests::testTopicMappingRoundTrip()
{
    using namespace Services::WebSocket;

    const QList<Topic> topics = {
        Topic::Configuration,
        Topic::Media,
        Topic::ApplicationStatus,
        Topic::ProcessorTemperature,
        Topic::BackendStatus,
        Topic::Environment,
    };

    for (const Topic topic : topics) {
        const QString wireName = topicToString(topic);
        QVERIFY2(wireName != "unknown", "Topic produced unknown wire name");
        QCOMPARE(topicFromString(wireName), topic);
    }

    QCOMPARE(topicFromString("invalid-topic"), Topic::UnknownTopic);
    QCOMPARE(topicToString(Topic::UnknownTopic), QString("unknown"));
}

void CommunicationUnitTests::testPlayModeToString()
{
    using namespace Services::WebSocket;

    QCOMPARE(playModeToString(PlayMode::Concurrent), QString("concurrent"));
    QCOMPARE(playModeToString(PlayMode::Queue), QString("queue"));
    QCOMPARE(playModeToString(PlayMode::Replace), QString("replace"));
}

void CommunicationUnitTests::testBuildRequestFrame()
{
    using namespace Services::WebSocket;

    QJsonObject params;
    params["value"] = 42;

    const QJsonObject request = Frame::buildRequest("7", Method::SetBrightness, params);

    QCOMPARE(request.value("jsonrpc").toString(), QString("2.0"));
    QCOMPARE(request.value("type").toString(), QString("request"));
    QCOMPARE(request.value("method").toString(), QString("setBrightness"));
    QCOMPARE(request.value("id"), QJsonValue(QString("7")));
    QCOMPARE(request.value("params").toObject().value("value").toInt(), 42);
    QVERIFY(Frame::isRequest(request));
    QVERIFY(!Frame::isResponse(request));
    QVERIFY(!Frame::isPublish(request));
}

void CommunicationUnitTests::testBuildResponseFrame()
{
    using namespace Services::WebSocket;

    QJsonObject result;
    result["status"] = "ok";

    const QJsonObject response = Frame::buildResponse("11", result);

    QCOMPARE(response.value("jsonrpc").toString(), QString("2.0"));
    QCOMPARE(response.value("type").toString(), QString("response"));
    QCOMPARE(response.value("id"), QJsonValue(QString("11")));
    QCOMPARE(response.value("result").toObject().value("status").toString(), QString("ok"));
    QVERIFY(Frame::isResponse(response));
    QVERIFY(!Frame::isRequest(response));
    QVERIFY(!Frame::isPublish(response));
}

void CommunicationUnitTests::testBuildErrorResponseFrame()
{
    using namespace Services::WebSocket;

    const QJsonObject response = Frame::buildErrorResponse("99", -32601, "Method not found");

    QCOMPARE(response.value("jsonrpc").toString(), QString("2.0"));
    QCOMPARE(response.value("type").toString(), QString("response"));
    QCOMPARE(response.value("id"), QJsonValue(QString("99")));
    QCOMPARE(response.value("error").toObject().value("code").toInt(), -32601);
    QCOMPARE(response.value("error").toObject().value("message").toString(), QString("Method not found"));
    QCOMPARE(Frame::parseErrorMessage(response), QString("Method not found"));
}

void CommunicationUnitTests::testBuildPublishFrame()
{
    using namespace Services::WebSocket;

    QJsonObject params;
    params["processor_temperature"] = 45000;

    const QJsonObject publish = Frame::buildPublish(Topic::ProcessorTemperature, params);

    QCOMPARE(publish.value("jsonrpc").toString(), QString("2.0"));
    QCOMPARE(publish.value("type").toString(), QString("publish"));
    QCOMPARE(publish.value("topic").toString(), QString("processor-temperature"));
    QCOMPARE(publish.value("params").toObject().value("processor_temperature").toInt(), 45000);
    QVERIFY(Frame::isPublish(publish));
    QVERIFY(!Frame::isRequest(publish));
    QVERIFY(!Frame::isResponse(publish));
}

void CommunicationUnitTests::testFrameParsers()
{
    using namespace Services::WebSocket;

    QJsonObject params;
    params["topic"] = "configuration";
    const QJsonObject request = Frame::buildRequest("123", Method::Subscribe, params);

    QCOMPARE(Frame::parseId(request), QJsonValue(QString("123")));
    QCOMPARE(Frame::parseMethod(request), Method::Subscribe);
    QCOMPARE(Frame::parseParams(request).value("topic").toString(), QString("configuration"));

    const QJsonObject numericIdRequest = Frame::buildRequest(QJsonValue(456), Method::GetConfig, QJsonObject());
    QCOMPARE(Frame::parseId(numericIdRequest), QJsonValue(456));
    QVERIFY(Frame::isValidId(Frame::parseId(numericIdRequest)));

    QJsonObject responseResult;
    responseResult["files"] = QJsonArray{QJsonValue("a.jpg"), QJsonValue("b.png")};
    const QJsonObject response = Frame::buildResponse("124", responseResult);

    QCOMPARE(Frame::parseResult(response).value("files").toArray().size(), 2);

    const QJsonObject publish = Frame::buildPublish(Topic::Environment,
                                                    QJsonObject{{"temperature_celsius", 21.5}});
    QCOMPARE(Frame::parseTopic(publish), Topic::Environment);
    QCOMPARE(Frame::parseParams(publish).value("temperature_celsius").toDouble(), 21.5);
}

void CommunicationUnitTests::testMethodCatalogCoverage()
{
    using namespace Services::Protocol;
    using namespace Services::WebSocket;

    const QList<Method> methods = MethodCatalog::allMethods();

    QVERIFY(methods.contains(Method::Subscribe));
    QVERIFY(methods.contains(Method::Unsubscribe));
    QVERIFY(methods.contains(Method::GetConfig));
    QVERIFY(methods.contains(Method::SetBrightness));
    QVERIFY(methods.contains(Method::SetVolume));
    QVERIFY(methods.contains(Method::SetDeviceId));
    QVERIFY(methods.contains(Method::UpdateSystemConfig));
    QVERIFY(methods.contains(Method::AddApp));
    QVERIFY(methods.contains(Method::UpdateApp));
    QVERIFY(methods.contains(Method::RemoveApp));
    QVERIFY(methods.contains(Method::GetStatus));
    QVERIFY(methods.contains(Method::GetProcessorTemperature));
    QVERIFY(methods.contains(Method::GetMedia));
    QVERIFY(methods.contains(Method::GetAllMedia));
    QVERIFY(methods.contains(Method::GetEnvironment));
    QVERIFY(methods.contains(Method::Shutdown));
    QVERIFY(methods.contains(Method::Reboot));
    QVERIFY(methods.contains(Method::PlaySound));
    QVERIFY(methods.contains(Method::StopSound));

    QVERIFY(!methods.contains(Method::UnknownMethod));

    for (const Method method : methods) {
        QCOMPARE(methodFromString(methodToString(method)), method);
    }
}

void CommunicationUnitTests::testTopicCatalogCoverage()
{
    using namespace Services::Protocol;
    using namespace Services::WebSocket;

    const QList<Topic> topics = MethodCatalog::allTopics();

    QVERIFY(topics.contains(Topic::Configuration));
    QVERIFY(topics.contains(Topic::Media));
    QVERIFY(topics.contains(Topic::ApplicationStatus));
    QVERIFY(topics.contains(Topic::ProcessorTemperature));
    QVERIFY(topics.contains(Topic::BackendStatus));
    QVERIFY(topics.contains(Topic::Environment));

    QVERIFY(!topics.contains(Topic::UnknownTopic));

    for (const Topic topic : topics) {
        QCOMPARE(topicFromString(topicToString(topic)), topic);
    }
}

QTEST_MAIN(CommunicationUnitTests)
#include "CommunicationUnitTests.moc"
