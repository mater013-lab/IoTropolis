#include "registration/IoTropolisUnitConnection.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

IoTropolisUnitConnection::IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent),
      m_socket(socket),
      m_helloDone(false),
      m_describeDone(false),
      m_unknownCommandCount(0)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &IoTropolisUnitConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &IoTropolisUnitConnection::onDisconnected);
}


// IP getter (normalized for IPv4-mapped addresses)
QString IoTropolisUnitConnection::ipAddress() const
{
    if (!m_socket) return QString();

    QString ipStr = m_socket->peerAddress().toString();

    // Strip IPv4-mapped IPv6 prefix if present
    if (ipStr.startsWith("::ffff:"))
        ipStr = ipStr.mid(7);

    return ipStr;
}

// Send reply to client
void IoTropolisUnitConnection::sendReply(const QString& msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(msg.toUtf8() + "\n");
        m_socket->flush();
    }
}

// Disconnect unit with optional client message
void IoTropolisUnitConnection::failProtocol(const QString& msg, const QString& clientMsg)
{
    emit protocolError(msg);
    if (!clientMsg.isEmpty()) sendReply(clientMsg);
    m_socket->disconnectFromHost();
}

// Reset unknown command counter
void IoTropolisUnitConnection::resetUnknownCommandCounter()
{
    m_unknownCommandCount = 0;
}

// --- TCP read handler ---
void IoTropolisUnitConnection::onReadyRead()
{
    while (m_socket->canReadLine())
    {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        // Split command from JSON
        int spaceIdx = line.indexOf(' ');
        QString command;
        QByteArray jsonPart;
        if (spaceIdx > 0) {
            command = QString(line.left(spaceIdx));
            jsonPart = line.mid(spaceIdx + 1);
        } else {
            command = QString(line);
            jsonPart.clear();
        }

        // --- HELLO ---
        if (command == "HELLO") {
            if (!m_helloDone) {
                if (jsonPart.isEmpty()) {
                    failProtocol("HELLO missing JSON payload", "ERROR: HELLO requires JSON payload");
                    return;
                }

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
                if (err.error != QJsonParseError::NoError) {
                    failProtocol("HELLO invalid JSON", "ERROR: HELLO invalid JSON");
                    return;
                }

                QJsonObject obj = doc.object();
                QString protocol = obj.value("protocol").toString();
                QString version = obj.value("version").toString();

                if (protocol != "IoTropolis" || version != "1.0") {
                    failProtocol("HELLO protocol/version mismatch", "ERROR: HELLO protocol/version mismatch");
                    return;
                }

                m_helloDone = true;
                resetUnknownCommandCounter();
                sendReply("HELLO_ACK");
                emit helloCompleted();
                qDebug() << "[IoTropolis] HELLO received from" << ipAddress()
                         << ", protocol:" << protocol << "version:" << version;
            } else {
                sendReply("HELLO_DONE"); // repeated HELLO
            }
            continue;
        }

        // Any command before HELLO is invalid
        if (!m_helloDone) {
            failProtocol(QString("First command must be HELLO, got: %1").arg(command),
                         "ERROR: First command must be HELLO");
            return;
        }

        // --- DESCRIBE ---
        if (command == "DESCRIBE") {
            if (m_describeDone) {
                sendReply("DESCRIBE_DONE"); // repeated DESCRIBE
                continue;
            }

            if (jsonPart.isEmpty()) {
                failProtocol("DESCRIBE missing JSON payload", "ERROR: DESCRIBE requires JSON payload");
                return;
            }

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
            if (err.error != QJsonParseError::NoError) {
                failProtocol("DESCRIBE invalid JSON", "ERROR: DESCRIBE invalid JSON");
                return;
            }

            QJsonObject obj = doc.object();
            if (!obj.contains("type") || !obj.contains("subtype") ||
                !obj.contains("sensors") || !obj.contains("actuators")) {
                failProtocol("DESCRIBE missing required fields", "ERROR: DESCRIBE missing required fields");
                return;
            }

            m_unitType = obj.value("type").toString();
            m_unitSubtype = obj.value("subtype").toString();

            QJsonArray sArray = obj.value("sensors").toArray();
            m_sensors.clear();
            for (const auto& s : sArray) m_sensors.append(s.toString());

            QJsonArray aArray = obj.value("actuators").toArray();
            m_actuators.clear();
            for (const auto& a : aArray) m_actuators.append(a.toString());

            // --- Validation against persistent file ---
            QString filename = QString("./UnitType/%1_%2.json").arg(m_unitType, m_unitSubtype);
            QFile file(filename);
            if (file.exists()) {
                if (!file.open(QIODevice::ReadOnly)) {
                    failProtocol("Cannot open type file for validation", "ERROR: Cannot open type file for validation");
                    return;
                }

                QByteArray fileData = file.readAll();
                file.close();

                QJsonParseError fileErr;
                QJsonDocument fileDoc = QJsonDocument::fromJson(fileData, &fileErr);
                if (fileErr.error != QJsonParseError::NoError || !fileDoc.isObject()) {
                    failProtocol("Type file corrupted", "ERROR: Type file corrupted");
                    return;
                }

                QJsonObject fileObj = fileDoc.object();

                // Validate sensors
                QStringList requiredSensors;
                QJsonArray fSensors = fileObj.value("sensors").toArray();
                for (const auto& s : fSensors) requiredSensors.append(s.toString());

                QStringList missingSensors;
                for (const auto& s : requiredSensors)
                    if (!m_sensors.contains(s)) missingSensors.append(s);

                // Validate actuators
                QStringList requiredActuators;
                QJsonArray fActuators = fileObj.value("actuators").toArray();
                for (const auto& a : fActuators) requiredActuators.append(a.toString());

                QStringList missingActuators;
                for (const auto& a : requiredActuators)
                    if (!m_actuators.contains(a)) missingActuators.append(a);

                if (!missingSensors.isEmpty() || !missingActuators.isEmpty()) {
                    QString msg = "ERROR: Sensor/actuator mismatch. ";
                    if (!missingSensors.isEmpty())
                        msg += "Missing sensors: " + missingSensors.join(", ") + ". ";
                    if (!missingActuators.isEmpty())
                        msg += "Missing actuators: " + missingActuators.join(", ") + ". ";
                    failProtocol("DESCRIBE validation failed", msg);
                    return;
                }
            }

            m_describeDone = true;
            resetUnknownCommandCounter();
            sendReply("DESCRIBE_ACK");
            emit describeCompleted();

            qDebug() << "[IoTropolis] DESCRIBE received from UnitID" << m_unitID
                     << ": type =" << m_unitType
                     << "subtype =" << m_unitSubtype
                     << "sensors =" << m_sensors
                     << "actuators =" << m_actuators;
            continue;
        }

        // --- Unknown command ---
        m_unknownCommandCount++;
        sendReply("UNKNOWN_COMMAND");
        qWarning() << "[IoTropolis] Unknown command from UnitID" << m_unitID << ":" << command;

        if (m_unknownCommandCount >= MAX_UNKNOWN_COMMANDS) {
            failProtocol("Exceeded MAX_UNKNOWN_COMMANDS",
                         "ERROR: Maximum unknown commands reached");
            return;
        }
    }
}

// --- Disconnection handler ---
void IoTropolisUnitConnection::onDisconnected()
{
    emit disconnected();
    m_socket->deleteLater();
}
