#include <QCoreApplication>
#include <QLoggingCategory>

#include "applications/Container.h"
#include "drivers/Container.h"
#include "services/Container.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QString dataDir = QStringLiteral(".");
    constexpr quint16 port = 5000;

    QLoggingCategory::setFilterRules(QStringLiteral("BackendQtWebSocketService.info=true\nBackendQtWebSocketService.debug=true"));

    Drivers::Container drivers;
    Services::Container services(drivers, dataDir);
    Applications::Container applications(services);

    if (!applications.start(port)) {
        return 1;
    }

    qInfo().noquote()
        << QStringLiteral("clock-backend running on:\n"
                          "\t\tws://127.0.0.1:%1/ws\n"
                          "\t\thttp://127.0.0.1:%1/media\n"
                          "\t\thttp://127.0.0.1:%1/api/media\n"
                          "\t\tdata directory: %2")
              .arg(port)
              .arg(dataDir);

    return app.exec();
}
