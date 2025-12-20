#include <QApplication>
#include "registration/IoTropolisRegistrationServer.h"
#include "gui/IoTropolisGui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create server
    IoTropolisRegistrationServer server;

    // Start server on port 12345
    if (!server.start(12345)) {
        qCritical() << "Failed to start server";
        return 1;
    }

    // Create GUI
    IoTropolisGui gui;

    // Connect server signals to GUI slots
    QObject::connect(&server, &IoTropolisRegistrationServer::unitReady,
                     &gui, [&gui](IoTropolisUnitConnection* unit) {

        // Wait until the DESCRIBE handshake is completed
        QObject::connect(unit, &IoTropolisUnitConnection::describeCompleted, [&gui, unit]() {
            gui.addUnit(unit);

            qDebug() << "[IoTropolis] Unit ready:"
                     << unit->unitType()
                     << unit->unitSubtype()
                     << unit->ipAddress()
                     << "Sensors:" << unit->sensors()
                     << "Actuators:" << unit->actuators();
        });

        // Handle protocol errors (signal matches exactly)
        QObject::connect(unit, &IoTropolisUnitConnection::protocolError,
                         [unit](const QString& msg) {
            qWarning() << "[IoTropolis] Protocol error from unit" << unit->ipAddress() << ":" << msg;
        });
    });

    QObject::connect(&server, &IoTropolisRegistrationServer::unitDisconnected,
                     &gui, [&gui](IoTropolisUnitConnection* unit) {
        gui.removeUnit(unit);
        qDebug() << "[IoTropolis] Unit disconnected:" 
                 << unit->unitType()
                 << unit->unitSubtype()
                 << unit->ipAddress();
    });

    gui.show();
    return app.exec();
}
