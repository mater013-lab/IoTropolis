#include "registration/IoTropolisUnitConnection.h"
#include "registration/IOComponent.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// ------------------------------------------------------------
// STATIC DISPATCH MAP INITIALIZATION
// To add a new command, simply add a line to this map.
// ------------------------------------------------------------
const QMap<QString, IoTropolisUnitConnection::HandlerFunc> IoTropolisUnitConnection::m_dispatchMap = {
    {"HELLO",    &IoTropolisUnitConnection::handleHello},
    {"DESCRIBE", &IoTropolisUnitConnection::handleDescribe}
};

IoTropolisUnitConnection::IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &IoTropolisUnitConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &IoTropolisUnitConnection::onDisconnected);
}

// ------------------------------------------------------------
// MAIN DISPATCHER
// ------------------------------------------------------------
void IoTropolisUnitConnection::onReadyRead()
{
    while (m_socket && m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        int spaceIdx = line.indexOf(' ');
        QString command = (spaceIdx == -1) ? QString(line) : QString(line.left(spaceIdx));
        QByteArray data = (spaceIdx == -1) ? QByteArray() : line.mid(spaceIdx + 1);

        // Lookup the handler in the map
        auto it = m_dispatchMap.find(command);
        if (it != m_dispatchMap.end()) {
            HandlerFunc handler = it.value();
            (this->*handler)(data); // Call the member function
        } else {
            handleUnknownCommand(command);
        }
    }
}

// ------------------------------------------------------------
// COMMAND HANDLERS
// ------------------------------------------------------------

void IoTropolisUnitConnection::handleHello(const QByteArray& data)
{
    if (m_helloDone) {
        failProtocol("Duplicate HELLO", "ERROR: Already greeted");
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(data).object();
    if (obj.value("version").toString() != "1.0") {
        failProtocol("Version mismatch", "ERROR: Supported version is 1.0");
        return;
    }

    m_helloDone = true;
    resetUnknownCommandCounter();
    sendReply("HELLO_ACK");
    emit helloCompleted();
}

void IoTropolisUnitConnection::handleDescribe(const QByteArray& data)
{
    if (!m_helloDone) {
        failProtocol("DESCRIBE before HELLO", "ERROR: Handshake first");
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(data).object();
    m_unitType = obj.value("type").toString();
    m_unitSubtype = obj.value("subtype").toString();

    if (!parseComponents(obj.value("sensors").toArray(), m_sensors, "sensor") ||
        !parseComponents(obj.value("actuators").toArray(), m_actuators, "actuator")) {
        return; 
    }

    m_describeDone = true;
    resetUnknownCommandCounter();
    sendReply("DESCRIBE_ACK");
    emit describeCompleted();
}

void IoTropolisUnitConnection::handleUnknownCommand(const QString& command)
{
    m_unknownCommandCount++;
    sendReply("UNKNOWN_COMMAND");
    qWarning() << "[IoTropolis] Unknown command:" << command;

    if (m_unknownCommandCount >= MAX_UNKNOWN_COMMANDS) {
        failProtocol("Too many unknown commands", "ERROR: Limit reached");
    }
}

// ------------------------------------------------------------
// PROTOCOL HELPERS (Behavior unchanged)
// ------------------------------------------------------------

bool IoTropolisUnitConnection::parseComponents(const QJsonArray& array, QList<IOComponent>& list, const QString& label)
{
    list.clear();
    for (const auto& val : array) {
        bool ok = false;
        IOComponent comp = IOComponent::fromJson(val.toObject(), &ok);
        if (!ok) {
            failProtocol("Invalid " + label, "ERROR: JSON Format");
            return false;
        }
        list.append(comp);
    }
    return true;
}

void IoTropolisUnitConnection::sendReply(const QString& msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(msg.toUtf8() + "\n");
    }
}

void IoTropolisUnitConnection::failProtocol(const QString& reason, const QString& clientMsg)
{
    qWarning() << "[IoTropolis] Protocol Error:" << reason;
    if (!clientMsg.isEmpty()) sendReply(clientMsg);
    m_socket->disconnectFromHost();
}

void IoTropolisUnitConnection::resetUnknownCommandCounter() { m_unknownCommandCount = 0; }
void IoTropolisUnitConnection::onDisconnected() { emit disconnected(); m_socket->deleteLater(); }

// -----------------------------------------------------------------------------
// IP address (normalized)
// -----------------------------------------------------------------------------
QString IoTropolisUnitConnection::ipAddress() const
{
    if (!m_socket)
        return {};

    QString ipStr = m_socket->peerAddress().toString();
    if (ipStr.startsWith("::ffff:"))
        ipStr = ipStr.mid(7);
    return ipStr;
}

// -----------------------------------------------------------------------------
// Convenience accessors for GUI and Logging
// -----------------------------------------------------------------------------
QStringList IoTropolisUnitConnection::sensorNames() const
{
    QStringList names;
    for (const auto& s : m_sensors)
        names << s.name();
    return names;
}

QStringList IoTropolisUnitConnection::actuatorNames() const
{
    QStringList names;
    for (const auto& a : m_actuators)
        names << a.name();
    return names;
}
