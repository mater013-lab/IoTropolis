#ifndef IOTROPOLISUNITCONNECTION_H
#define IOTROPOLISUNITCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QStringList>

constexpr int MAX_UNKNOWN_COMMANDS = 5;

// Type alias for unit ID (can be extended later)
using UnitID = quint32;

class IoTropolisUnitConnection : public QObject
{
    Q_OBJECT
public:
    explicit IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent = nullptr);

    QString ipAddress() const;  // readable IP
    QString unitType() const { return m_unitType; }
    QString unitSubtype() const { return m_unitSubtype; }
    QStringList sensors() const { return m_sensors; }
    QStringList actuators() const { return m_actuators; }

    UnitID unitID() const { return m_unitID; }
    void setUnitID(UnitID id) { m_unitID = id; }

signals:
    void helloCompleted();
    void describeCompleted();
    void protocolError(const QString& msg);
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void failProtocol(const QString& msg, const QString& clientMsg = QString());
    void sendReply(const QString& msg);
    void resetUnknownCommandCounter();

    QTcpSocket* m_socket;

    bool m_helloDone;
    bool m_describeDone;

    QString m_unitType;
    QString m_unitSubtype;
    QStringList m_sensors;
    QStringList m_actuators;

    int m_unknownCommandCount;

    UnitID m_unitID{0}; // Assigned by server
};

#endif // IOTROPOLISUNITCONNECTION_H
