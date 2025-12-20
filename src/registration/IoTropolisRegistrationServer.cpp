#include "registration/IoTropolisRegistrationServer.h"
#include "registration/IoTropolisUnitConnection.h"

#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

IoTropolisRegistrationServer::IoTropolisRegistrationServer(QObject* parent)
    : QObject(parent)
{
    QDir dir;
    if (!dir.exists("./UnitType"))
        dir.mkpath("./UnitType");

    m_units.clear();
    m_nextUnitID = 1;
}

bool IoTropolisRegistrationServer::start(quint16 port)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &IoTropolisRegistrationServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port))
    {
        qCritical() << "Failed to start server on port" << port;
        return false;
    }

    qDebug() << "[IoTropolis] Server started on port" << port;
    return true;
}

void IoTropolisRegistrationServer::onNewConnection()
{
    while (m_server->hasPendingConnections())
    {
        QTcpSocket* socket = m_server->nextPendingConnection();
        auto* unit = new IoTropolisUnitConnection(socket, this);

        // Assign a unique UnitID
        unit->setUnitID(m_nextUnitID++);

        qDebug() << "[IoTropolis] New connection assigned UnitID" << unit->unitID()
                 << "from IP" << unit->ipAddress();

        // Connect signals using lambdas to pass unit pointer
        connect(unit, &IoTropolisUnitConnection::helloCompleted, this,
                [this, unit]() { this->onUnitHello(unit); });

        connect(unit, &IoTropolisUnitConnection::describeCompleted, this,
                [this, unit]() { this->onUnitDescribe(unit); });

        connect(unit, &IoTropolisUnitConnection::protocolError, this,
                [this, unit](const QString &msg) { this->onUnitProtocolError(unit, msg); });

        connect(unit, &IoTropolisUnitConnection::disconnected, this,
                [this, unit]() { this->onUnitDisconnectedInternal(unit); });

        m_units.insert(unit);
    }
}

void IoTropolisRegistrationServer::onUnitHello(IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] HELLO completed from UnitID" << unit->unitID()
             << "IP:" << unit->ipAddress();
    emit unitReady(unit);
}

void IoTropolisRegistrationServer::onUnitDescribe(IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] DESCRIBE completed for UnitID" << unit->unitID()
             << ": type =" << unit->unitType()
             << "subtype =" << unit->unitSubtype()
             << "sensors =" << unit->sensors()
             << "actuators =" << unit->actuators();

    // Persist type/subtype if new
    validateOrCreateTypeFile(unit);

    emit unitReady(unit);  // signal after DESCRIBE as well
}

void IoTropolisRegistrationServer::onUnitProtocolError(IoTropolisUnitConnection* unit, const QString &msg)
{
    qWarning() << "[IoTropolis] Protocol error from UnitID" << unit->unitID()
               << "IP:" << unit->ipAddress() << ":" << msg;
    emit unitError(unit, msg);
}

void IoTropolisRegistrationServer::onUnitDisconnectedInternal(IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] Unit disconnected: UnitID" << unit->unitID()
             << "IP:" << unit->ipAddress();

    m_units.remove(unit);
    emit unitDisconnected(unit);

    unit->deleteLater();
}

bool IoTropolisRegistrationServer::validateOrCreateTypeFile(IoTropolisUnitConnection* unit)
{
    QString filename = QString("./UnitType/%1_%2.json").arg(unit->unitType(), unit->unitSubtype());
    QFile file(filename);

    if (file.exists())
        return true;

    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot create type file:" << filename;
        return false;
    }

    QJsonObject obj;
    QJsonArray sArray;
    for (const auto& s : unit->sensors())
        sArray.append(s);
    obj["sensors"] = sArray;

    QJsonArray aArray;
    for (const auto& a : unit->actuators())
        aArray.append(a);
    obj["actuators"] = aArray;

    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[IoTropolis] Created new type file:" << filename;
    return true;
}
