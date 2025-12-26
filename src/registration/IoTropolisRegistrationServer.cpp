#include "registration/IoTropolisRegistrationServer.h"
#include "registration/IoTropolisUnitConnection.h"
#include "registration/IOComponent.h"

#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace {

QList<IOComponent> componentsFromJson(const QJsonArray& array)
{
    QList<IOComponent> out;
    for (const auto& v : array) {
        if (v.isObject())
            out.append(IOComponent::fromJson(v.toObject()));
    }
    return out;
}

QStringList validateComponents(const QList<IOComponent>& expected,
                               const QList<IOComponent>& actual)
{
    QStringList missing;

    for (const auto& e : expected) {
        bool found = false;
        for (const auto& a : actual) {
            if (a.name() == e.name() &&
                a.format() == e.format()) {
                found = true;
                break;
            }
        }
        if (!found)
            missing << e.name();
    }
    return missing;
}

QJsonArray componentsToJson(const QList<IOComponent>& comps)
{
    QJsonArray arr;
    for (const auto& c : comps)
        arr.append(c.toJson());
    return arr;
}

} // namespace

IoTropolisRegistrationServer::IoTropolisRegistrationServer(const QString& unitTypeDir, QObject* parent)
    : QObject(parent)
    , m_unitTypeDir(unitTypeDir)
{
    QDir dir;
    if (!dir.exists(m_unitTypeDir))
        dir.mkpath(m_unitTypeDir);

    m_units.clear();
    m_nextUnitID = 1;
}

bool IoTropolisRegistrationServer::start(quint16 port)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &IoTropolisRegistrationServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server on port" << port;
        return false;
    }

    qDebug() << "[IoTropolis] Server started on port" << port;
    return true;
}

// ---------------------- Handle new connections ----------------------
void IoTropolisRegistrationServer::onNewConnection()
{
    while (m_server->hasPendingConnections())
    {
        QTcpSocket* socket = m_server->nextPendingConnection();
        auto* unit = new IoTropolisUnitConnection(socket, this);

        unit->setUnitID(m_nextUnitID++);

        qDebug() << "[IoTropolis] New connection assigned UnitID"
                 << unit->unitID()
                 << "from IP" << unit->ipAddress();

        connect(unit, &IoTropolisUnitConnection::helloCompleted,
                this, [this, unit]() { onUnitHello(unit); });

        connect(unit, &IoTropolisUnitConnection::describeCompleted,
                this, [this, unit]() { onUnitDescribe(unit); });

        connect(unit, &IoTropolisUnitConnection::protocolError,
                this, [this, unit](const QString& msg) {
                    onUnitProtocolError(unit, msg);
                });

        connect(unit, &IoTropolisUnitConnection::disconnected,
                this, [this, unit]() { onUnitDisconnectedInternal(unit); });

        m_units.insert(unit);
    }
}

// ---------------------- Unit events ----------------------
void IoTropolisRegistrationServer::onUnitHello(IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] HELLO completed from UnitID"
             << unit->unitID()
             << "IP:" << unit->ipAddress();

    emit unitProtocolCompatible(unit);
}

void IoTropolisRegistrationServer::onUnitDescribe(IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] DESCRIBE completed for UnitID"
             << unit->unitID()
             << ": type =" << unit->unitType()
             << "subtype =" << unit->unitSubtype()
             << "sensors =" << unit->sensorNames()
             << "actuators =" << unit->actuatorNames();

    QString filename = QString("%1/%2_%3.json")
                           .arg(m_unitTypeDir, unit->unitType(), unit->unitSubtype());
    QFile file(filename);

    bool persistentExists = file.exists();

    if (persistentExists) {
        if (!file.open(QIODevice::ReadOnly)) {
            emit unitError(unit, "Cannot open type file for validation");
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            emit unitError(unit, "Persistent type file corrupted");
            return;
        }

        QJsonObject obj = doc.object();

        QList<IOComponent> pSensors =
            componentsFromJson(obj.value("sensors").toArray());
        QList<IOComponent> pActuators =
            componentsFromJson(obj.value("actuators").toArray());

        QStringList missingSensors =
            validateComponents(pSensors, unit->sensors());
        QStringList missingActuators =
            validateComponents(pActuators, unit->actuators());

        if (!missingSensors.isEmpty() || !missingActuators.isEmpty()) {
            QString msg = "DESCRIBE validation failed. ";
            if (!missingSensors.isEmpty())
                msg += "Missing/mismatched sensors: " +
                       missingSensors.join(", ") + ". ";
            if (!missingActuators.isEmpty())
                msg += "Missing/mismatched actuators: " +
                       missingActuators.join(", ") + ". ";
            emit unitError(unit, msg);
            return;
        }

    } else {
        if (!file.open(QIODevice::WriteOnly)) {
            emit unitError(unit, "Cannot create type file");
            return;
        }

        QJsonObject obj;
        obj["sensors"]   = componentsToJson(unit->sensors());
        obj["actuators"] = componentsToJson(unit->actuators());

        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        qDebug() << "[IoTropolis] Created new type file:" << filename;
    }

    emit unitFullyRegistered(unit);
}

void IoTropolisRegistrationServer::onUnitProtocolError(
    IoTropolisUnitConnection* unit, const QString& msg)
{
    qWarning() << "[IoTropolis] Protocol error from UnitID"
               << unit->unitID()
               << "IP:" << unit->ipAddress()
               << ":" << msg;

    emit unitError(unit, msg);
}

void IoTropolisRegistrationServer::onUnitDisconnectedInternal(
    IoTropolisUnitConnection* unit)
{
    qDebug() << "[IoTropolis] Unit disconnected: UnitID"
             << unit->unitID()
             << "IP:" << unit->ipAddress();

    // 🔒 notify observers while the object is still valid
    emit unitAboutToBeRemoved(unit);

    m_units.remove(unit);
    emit unitDisconnected(unit);

    unit->deleteLater();
}
