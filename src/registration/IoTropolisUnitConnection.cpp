#include "registration/IoTropolisUnitConnection.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

IoTropolisUnitConnection::IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket), m_helloDone(false), m_describeDone(false)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &IoTropolisUnitConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &IoTropolisUnitConnection::onDisconnected);
}

// Definition of ipAddress() only in .cpp
QString IoTropolisUnitConnection::ipAddress() const
{
    if (m_socket)
        return m_socket->peerAddress().toString();
    return QString();
}

void IoTropolisUnitConnection::onReadyRead()
{
    while (m_socket->canReadLine())
    {
        QByteArray line = m_socket->readLine().trimmed();

        // HELLO handling
        if (line == "HELLO") {
            if (!m_helloDone) {
                m_helloDone = true;
                emit helloCompleted();
                qDebug() << "[IoTropolis] HELLO received from" << ipAddress();
            } else {
                emit protocolError("HELLO already done");
            }
            continue;
        }

        // DESCRIBE handling
        if (line.startsWith("DESCRIBE ")) {
            if (!m_describeDone) {
                QByteArray jsonPart = line.mid(strlen("DESCRIBE "));
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
                if (err.error != QJsonParseError::NoError) {
                    emit protocolError("Invalid JSON message");
                    continue;
                }

                QJsonObject obj = doc.object();
                m_unitType = obj.value("type").toString();
                m_unitSubtype = obj.value("subtype").toString();

                QJsonArray sArray = obj.value("sensors").toArray();
                for (const auto& s : sArray)
                    m_sensors.append(s.toString());

                QJsonArray aArray = obj.value("actuators").toArray();
                for (const auto& a : aArray)
                    m_actuators.append(a.toString());

                m_describeDone = true;
                emit describeCompleted();

                qDebug() << "[IoTropolis] DESCRIBE received from" << ipAddress()
                         << ": type =" << m_unitType
                         << "subtype =" << m_unitSubtype
                         << "sensors =" << m_sensors
                         << "actuators =" << m_actuators;
            } else {
                emit protocolError("DESCRIBE already done");
            }
            continue;
        }

        // Any other message
        emit protocolError("Unexpected message");
    }
}

void IoTropolisUnitConnection::onDisconnected()
{
    emit disconnected();
    m_socket->deleteLater();
}
