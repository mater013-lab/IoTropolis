#include <QApplication>
#include <QDebug>

#include "registration/IoTropolisRegistrationServer.h"
#include "gui/IoTropolisGui.h"
#include "config/IoTropolisConfig.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // --- Load configuration ---
    QString configPath = (argc > 1) ? QString(argv[1]) 
                                    : IoTropolisConfig::defaultConfigPath();

    IoTropolisConfig config(configPath);

    // --- Create server using unit type directory from config ---
    IoTropolisRegistrationServer server(config.unitTypeDir());

    // --- Start server with port from config ---
    if (!server.start(config.tcpPort())) {
        qCritical() << "Failed to start IoTropolis server";
        return 1;
    }

    IoTropolisGui gui;

    // ---- Unit fully registered (safe: unit fully alive) ----
    QObject::connect(&server,
                     &IoTropolisRegistrationServer::unitFullyRegistered,
                     &gui,
                     [&](IoTropolisUnitConnection* unit) {

        gui.addUnit(unit);

        qDebug() << "[IoTropolis] Unit fully registered:"
                 << unit->unitType()
                 << unit->unitSubtype()
                 << unit->ipAddress()
                 << "Sensors:" << unit->sensorNames()
                 << "Actuators:" << unit->actuatorNames();
    });

    // ---- Unit about to be removed (safe: last chance to read state) ----
    QObject::connect(&server,
                     &IoTropolisRegistrationServer::unitAboutToBeRemoved,
                     &gui,
                     [&](IoTropolisUnitConnection* unit) {

        gui.removeUnit(unit);

        qDebug() << "[IoTropolis] Unit removed from GUI:"
                 << unit->unitID()
                 << unit->ipAddress();
    });

    // ---- Transport-level disconnection (do NOT dereference unit) ----
    QObject::connect(&server,
                     &IoTropolisRegistrationServer::unitDisconnected,
                     &server,
                     [] {
                         qDebug() << "[IoTropolis] Unit disconnected (transport-level)";
                     });

    gui.show();
    return app.exec();
}
