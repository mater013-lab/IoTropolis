#ifndef IOTROPOLISREGISTRATIONSERVER_H
#define IOTROPOLISREGISTRATIONSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSet>

#include "registration/IoTropolisUnitConnection.h"

class IoTropolisRegistrationServer : public QObject
{
    Q_OBJECT
public:
//    explicit IoTropolisRegistrationServer(QObject* parent = nullptr);
    explicit IoTropolisRegistrationServer(const QString& unitTypeDir, QObject* parent = nullptr);

    bool start(quint16 port);

signals:
    // Unit passed HELLO; protocol compatibility confirmed
    void unitProtocolCompatible(IoTropolisUnitConnection* unit);

    // Unit passed DESCRIBE; fully registered and usable
    void unitFullyRegistered(IoTropolisUnitConnection* unit);

    // 🔒 NEW — emitted immediately before removal & deletion
    void unitAboutToBeRemoved(IoTropolisUnitConnection* unit);

    // Unit disconnected (before deletion)
    void unitDisconnected(IoTropolisUnitConnection* unit);

    // Forwarded protocol error from a unit
    void unitError(IoTropolisUnitConnection* unit, const QString& msg);

private slots:
    void onNewConnection();
    void onUnitHello(IoTropolisUnitConnection* unit);
    void onUnitDescribe(IoTropolisUnitConnection* unit);
    void onUnitProtocolError(IoTropolisUnitConnection* unit, const QString& msg);
    void onUnitDisconnectedInternal(IoTropolisUnitConnection* unit);

private:
    QSet<IoTropolisUnitConnection*> m_units;
    QTcpServer* m_server{nullptr};

    UnitID m_nextUnitID{1};
    QString m_unitTypeDir;
};

#endif // IOTROPOLISREGISTRATIONSERVER_H
