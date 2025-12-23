#include "registration/IoTropolisUnitConnection.h"
#include "registration/IOComponent.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
IoTropolisUnitConnection::IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent),
      m_socket(socket),
      m_helloDone(false),
      m_describeDone(false),
      m_unknownCommandCount(0)
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &IoTropolisUnitConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &IoTropolisUnitConnection::onDisconnected);
}

// ------------------------------------------------------------
// IP address (normalized)
// ------------------------------------------------------------
QString IoTropolisUnitConnection::ipAddress() const
{
    if (!m_socket)
        return {};

    QString ipStr = m_socket->peerAddress().toString();
    if (ipStr.startsWith("::ffff:"))
        ipStr = ipStr.mid(7);
    return ipStr;
}

// ------------------------------------------------------------
// Convenience accessors
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Send reply to client
// ------------------------------------------------------------
void IoTropolisUnitConnection::sendReply(const QString& msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(msg.toUtf8() + "\n");
        m_socket->flush();
    }
}

// ------------------------------------------------------------
// Protocol failure handler
// ------------------------------------------------------------
void IoTropolisUnitConnection::failProtocol(const QString& msg,
                                            const QString& clientMsg)
{
    emit protocolError(msg);
    if (!clientMsg.isEmpty())
        sendReply(clientMsg);

    m_socket->disconnectFromHost();
}

// ------------------------------------------------------------
// Reset unknown command counter
// ------------------------------------------------------------
void IoTropolisUnitConnection::resetUnknownCommandCounter()
{
    m_unknownCommandCount = 0;
}

// ------------------------------------------------------------
// TCP read handler
// ------------------------------------------------------------
void IoTropolisUnitConnection::onReadyRead()
{
    // Helper lambda: parse sensors or actuators
    auto parseComponents =
        [&](const QJsonArray& array,
            QList<IOComponent>& target,
            const QString& label) -> bool
    {
        target.clear();

        for (const auto& v : array) {
            if (!v.isObject()) {
                failProtocol(label + " entry not an object",
                             "ERROR: Each " + label + " must be an object");
                return false;
            }

            bool ok = false;
            IOComponent c = IOComponent::fromJson(v.toObject(), &ok);
            if (!ok) {
                failProtocol(label + " missing 'name' or 'format'",
                             "ERROR: Each " + label + " must have 'name' and 'format'");
                return false;
            }

            target.append(c);
        }
        return true;
    };

    while (m_socket->canReadLine()) {

        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty())
            continue;

        int spaceIdx = line.indexOf(' ');
        QString command;
        QByteArray jsonPart;

        if (spaceIdx > 0) {
            command = QString::fromUtf8(line.left(spaceIdx));
            jsonPart = line.mid(spaceIdx + 1);
        } else {
            command = QString::fromUtf8(line);
        }

        // ----------------------------------------------------
        // HELLO
        // ----------------------------------------------------
        if (command == "HELLO") {

            if (m_helloDone) {
                sendReply("HELLO_DONE");
                continue;
            }

            if (jsonPart.isEmpty()) {
                failProtocol("HELLO missing JSON payload",
                             "ERROR: HELLO requires JSON payload");
                return;
            }

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
            if (err.error != QJsonParseError::NoError) {
                failProtocol("HELLO invalid JSON",
                             "ERROR: HELLO invalid JSON");
                return;
            }

            QJsonObject obj = doc.object();
            QString protocol = obj.value("protocol").toString();
            QString version  = obj.value("version").toString();

            if (protocol != "IoTropolis" || version != "1.0") {
                failProtocol("HELLO protocol/version mismatch",
                             "ERROR: HELLO protocol/version mismatch");
                return;
            }

            m_helloDone = true;
            resetUnknownCommandCounter();
            sendReply("HELLO_ACK");
            emit helloCompleted();

            qDebug() << "[IoTropolis] HELLO received from" << ipAddress();
            continue;
        }

        // ----------------------------------------------------
        // Must have HELLO first
        // ----------------------------------------------------
        if (!m_helloDone) {
            failProtocol("First command must be HELLO",
                         "ERROR: First command must be HELLO");
            return;
        }

        // ----------------------------------------------------
        // DESCRIBE
        // ----------------------------------------------------
        if (command == "DESCRIBE") {

            if (m_describeDone) {
                sendReply("DESCRIBE_DONE");
                continue;
            }

            if (jsonPart.isEmpty()) {
                failProtocol("DESCRIBE missing JSON payload",
                             "ERROR: DESCRIBE requires JSON payload");
                return;
            }

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
            if (err.error != QJsonParseError::NoError) {
                failProtocol("DESCRIBE invalid JSON",
                             "ERROR: DESCRIBE invalid JSON");
                return;
            }

            QJsonObject obj = doc.object();
            if (!obj.contains("type") || !obj.contains("subtype") ||
                !obj.contains("sensors") || !obj.contains("actuators")) {
                failProtocol("DESCRIBE missing required fields",
                             "ERROR: DESCRIBE missing required fields");
                return;
            }

            m_unitType    = obj.value("type").toString();
            m_unitSubtype = obj.value("subtype").toString();

            if (!parseComponents(obj.value("sensors").toArray(),
                                 m_sensors, "sensor"))
                return;

            if (!parseComponents(obj.value("actuators").toArray(),
                                 m_actuators, "actuator"))
                return;

            m_describeDone = true;
            resetUnknownCommandCounter();
            sendReply("DESCRIBE_ACK");
            emit describeCompleted();

            qDebug() << "[IoTropolis] DESCRIBE received from UnitID"
                     << m_unitID
                     << "type =" << m_unitType
                     << "subtype =" << m_unitSubtype
                     << "sensors =" << sensorNames()
                     << "actuators =" << actuatorNames();
            continue;
        }

        // ----------------------------------------------------
        // Unknown command
        // ----------------------------------------------------
        m_unknownCommandCount++;
        sendReply("UNKNOWN_COMMAND");

        qWarning() << "[IoTropolis] Unknown command from UnitID"
                   << m_unitID << ":" << command;

        if (m_unknownCommandCount >= MAX_UNKNOWN_COMMANDS) {
            failProtocol("Exceeded MAX_UNKNOWN_COMMANDS",
                         "ERROR: Maximum unknown commands reached");
            return;
        }
    }
}

// ------------------------------------------------------------
// Disconnection handler
// ------------------------------------------------------------
void IoTropolisUnitConnection::onDisconnected()
{
    emit disconnected();
    m_socket->deleteLater();
}
