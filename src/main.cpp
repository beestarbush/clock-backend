#include <QCoreApplication>

#include "applications/Container.h"
#include "drivers/Container.h"
#include "services/Container.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Drivers::Container drivers;
    Services::Container services(drivers);
    Applications::Container applications(services);

    return app.exec();
}
