#include <QApplication>
#include <QDebug>

#include "registration/IoTropolisRegistrationServer.h"
#include "gui/IoTropolisGui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    IoTropolisRegistrationServer server;

    constexpr quint16 DEFAULT_PORT = 12345;
    if (!server.start(DEFAULT_PORT)) {
        qCritical() << "Failed to start IoTropolis server";
        return 1;
    }

    IoTropolisGui gui;

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

    QObject::connect(&server,
                     &IoTropolisRegistrationServer::unitDisconnected,
                     &gui,
                     [&](IoTropolisUnitConnection* unit) {

        gui.removeUnit(unit);

        qDebug() << "[IoTropolis] Unit removed from GUI:"
                 << unit->unitID()
                 << unit->ipAddress();
    });

    gui.show();
    return app.exec();
}
