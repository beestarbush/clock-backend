#include <QtTest>
#include <QJsonArray>

#include "services/configuration/DeviceConfiguration.h"

class DeviceConfigurationUnitTests : public QObject
{
    Q_OBJECT

  private slots:
    void testDefaultState();
    void testToJsonWithAllFields();
        void testFromJsonWithFrameUnwrap();
    void testFromJsonWithMissingOptionalFields();
    void testFromJsonFiltersInvalidApplications();
    void testApplicationMutations();
};

void DeviceConfigurationUnitTests::testDefaultState()
{
    using namespace Services::Configuration;

    DeviceConfiguration config;

    QCOMPARE(config.version, QString("1.0"));
    QVERIFY(config.lastModified.isValid());
    QVERIFY(config.isValid());
    QVERIFY(!config.hasApplications());
    QCOMPARE(config.applicationCount(), 0);

    const QJsonObject json = config.toJson();
    QCOMPARE(json.value("version").toString(), QString("1.0"));
    QVERIFY(json.contains("last_modified"));
    QVERIFY(json.value("applications").isArray());
    QCOMPARE(json.value("applications").toArray().size(), 0);
}

void DeviceConfigurationUnitTests::testToJsonWithAllFields()
{
    using namespace Services::Configuration;

    DeviceConfiguration config;
    config.version = "2.3";
    config.deviceId = "SN-TEST-001";
    config.activeAppId = "clock";
    config.lastModified = QDateTime::fromString("2026-05-29T10:30:00Z", Qt::ISODate);

    QJsonObject systemConfig;
    systemConfig["brightness"] = 75;
    systemConfig["volume"] = 60;
    systemConfig["base-color"] = "#000000";
    config.systemConfiguration = systemConfig;

    QJsonObject app1;
    app1["id"] = "clock";
    app1["type"] = "clock";
    app1["order"] = 1;

    QJsonObject app2;
    app2["id"] = "environment";
    app2["type"] = "environment";
    app2["order"] = 2;

    config.applications = {app1, app2};

    const QJsonObject json = config.toJson();

    QCOMPARE(json.value("version").toString(), QString("2.3"));
    QCOMPARE(json.value("device_id").toString(), QString("SN-TEST-001"));
    QCOMPARE(json.value("active_app_id").toString(), QString("clock"));
    QCOMPARE(json.value("last_modified").toString(), QString("2026-05-29T10:30:00Z"));

    QVERIFY(json.contains("system-configuration"));
    QCOMPARE(json.value("system-configuration").toObject().value("brightness").toInt(), 75);
    QCOMPARE(json.value("system-configuration").toObject().value("volume").toInt(), 60);

    const QJsonArray apps = json.value("applications").toArray();
    QCOMPARE(apps.size(), 2);
    QCOMPARE(apps.at(0).toObject().value("id").toString(), QString("clock"));
    QCOMPARE(apps.at(1).toObject().value("id").toString(), QString("environment"));
}

void DeviceConfigurationUnitTests::testFromJsonWithFrameUnwrap()
{
    using namespace Services::Configuration;

    QJsonObject configPayload;
    configPayload["version"] = "3.0";
    configPayload["device_id"] = "SN-FRAME";
    configPayload["active_app_id"] = "env";
    configPayload["last_modified"] = "2026-05-29T11:00:00Z";

    QJsonObject systemConfig;
    systemConfig["brightness"] = 10;
    configPayload["system-configuration"] = systemConfig;

    QJsonArray apps;
    apps.append(QJsonObject{{"id", "env"}, {"type", "environment"}});
    configPayload["applications"] = apps;

    QJsonObject wrapped;
    wrapped["status"] = "success";
    wrapped["configuration"] = configPayload;

    const DeviceConfiguration parsed = DeviceConfiguration::fromJson(wrapped);

    QCOMPARE(parsed.version, QString("3.0"));
    QCOMPARE(parsed.deviceId, QString("SN-FRAME"));
    QCOMPARE(parsed.activeAppId, QString("env"));
    QCOMPARE(parsed.systemConfiguration.value("brightness").toInt(), 10);
    QCOMPARE(parsed.applicationCount(), 1);
    QCOMPARE(parsed.applications.at(0).value("id").toString(), QString("env"));
}

void DeviceConfigurationUnitTests::testFromJsonWithMissingOptionalFields()
{
    using namespace Services::Configuration;

    QJsonObject raw;
    raw["applications"] = QJsonArray{};

    const DeviceConfiguration parsed = DeviceConfiguration::fromJson(raw);

    QCOMPARE(parsed.version, QString("1.0"));
    QCOMPARE(parsed.deviceId, QString());
    QCOMPARE(parsed.activeAppId, QString());
    QVERIFY(parsed.systemConfiguration.isEmpty());
    QVERIFY(parsed.lastModified.isValid() || !parsed.lastModified.isValid());
    QCOMPARE(parsed.applicationCount(), 0);
    QVERIFY(parsed.isValid());
}

void DeviceConfigurationUnitTests::testFromJsonFiltersInvalidApplications()
{
    using namespace Services::Configuration;

    QJsonArray apps;
    apps.append(QJsonObject{{"id", "valid-1"}, {"type", "clock"}});
    apps.append(QJsonValue(123));
    apps.append(QJsonValue("bad"));
    apps.append(QJsonObject{{"id", "valid-2"}, {"type", "environment"}});

    QJsonObject raw;
    raw["applications"] = apps;

    const DeviceConfiguration parsed = DeviceConfiguration::fromJson(raw);

    QCOMPARE(parsed.applicationCount(), 2);
    QCOMPARE(parsed.applications.at(0).value("id").toString(), QString("valid-1"));
    QCOMPARE(parsed.applications.at(1).value("id").toString(), QString("valid-2"));
}

void DeviceConfigurationUnitTests::testApplicationMutations()
{
    using namespace Services::Configuration;

    DeviceConfiguration config;

    QJsonObject appA;
    appA["id"] = "a";
    appA["type"] = "clock";
    appA["order"] = 1;

    QJsonObject appB;
    appB["id"] = "b";
    appB["type"] = "environment";
    appB["order"] = 2;

    config.addApplication(appA);
    config.addApplication(appB);

    QCOMPARE(config.applicationCount(), 2);
    QVERIFY(config.hasApplications());

    QJsonObject appAUpdated;
    appAUpdated["id"] = "a";
    appAUpdated["type"] = "clock";
    appAUpdated["order"] = 9;

    config.updateApplication(appAUpdated);
    QCOMPARE(config.applicationCount(), 2);

    bool foundUpdated = false;
    for (const QJsonObject& app : config.applications) {
        if (app.value("id").toString() == "a") {
            foundUpdated = true;
            QCOMPARE(app.value("order").toInt(), 9);
        }
    }
    QVERIFY(foundUpdated);

    QJsonObject appC;
    appC["id"] = "c";
    appC["type"] = "no-operation";
    config.updateApplication(appC);
    QCOMPARE(config.applicationCount(), 3);

    config.removeApplication("b");
    QCOMPARE(config.applicationCount(), 2);
    for (const QJsonObject& app : config.applications) {
        QVERIFY(app.value("id").toString() != "b");
    }
}

QTEST_MAIN(DeviceConfigurationUnitTests)
#include "DeviceConfigurationUnitTests.moc"
