#ifndef IOTROPOLISUNITCONNECTION_H
#define IOTROPOLISUNITCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QStringList>
#include <QList>
#include <QMap>

#include "registration/IOComponent.h"

constexpr int MAX_UNKNOWN_COMMANDS = 5;

// Type alias for unit ID
using UnitID = quint32;

class IoTropolisUnitConnection : public QObject
{
    Q_OBJECT
public:
    explicit IoTropolisUnitConnection(QTcpSocket* socket,
                                      QObject* parent = nullptr);

    // --------------------------------------------------------
    // Connection info
    // --------------------------------------------------------
    QString ipAddress() const;

    // --------------------------------------------------------
    // IO Components
    // --------------------------------------------------------
    QList<IOComponent> sensors() const   { return m_sensors; }
    QList<IOComponent> actuators() const { return m_actuators; }

    QStringList sensorNames() const;
    QStringList actuatorNames() const;

    // --------------------------------------------------------
    // Unit metadata
    // --------------------------------------------------------
    QString unitType() const    { return m_unitType; }
    QString unitSubtype() const { return m_unitSubtype; }

    UnitID unitID() const       { return m_unitID; }
    void setUnitID(UnitID id)   { m_unitID = id; }

signals:
    void helloCompleted();
    void describeCompleted();
    void protocolError(const QString& msg);
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    // --------------------------------------------------------
    // NEW: Command Dispatch System
    // --------------------------------------------------------
    
    // Typedef for a pointer to a member function handler
    typedef void (IoTropolisUnitConnection::*HandlerFunc)(const QByteArray&);

    // Static map that associates strings ("HELLO") with their functions
    static const QMap<QString, HandlerFunc> m_dispatchMap;

    // Command Handlers
    void handleHello(const QByteArray& data);
    void handleDescribe(const QByteArray& data);
    void handleUnknownCommand(const QString& command);

    // --------------------------------------------------------
    // Protocol helpers
    // --------------------------------------------------------
    void failProtocol(const QString& msg,
                      const QString& clientMsg = QString());
    void sendReply(const QString& msg);
    void resetUnknownCommandCounter();

    bool parseComponents(const QJsonArray& array, 
                         QList<IOComponent>& list, 
                         const QString& label);

    // --------------------------------------------------------
    // Data members
    // --------------------------------------------------------
    QTcpSocket* m_socket{nullptr};

    bool m_helloDone{false};
    bool m_describeDone{false};

    QString m_unitType;
    QString m_unitSubtype;

    QList<IOComponent> m_sensors;
    QList<IOComponent> m_actuators;

    int m_unknownCommandCount{0};

    UnitID m_unitID{0}; 
};

#endif // IOTROPOLISUNITCONNECTION_H
