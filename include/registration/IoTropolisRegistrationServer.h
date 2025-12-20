#ifndef IOTROPOLISREGISTRATIONSERVER_H
#define IOTROPOLISREGISTRATIONSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSet>

class IoTropolisUnitConnection;
class IoTropolisGui;

class IoTropolisRegistrationServer : public QObject
{
    Q_OBJECT
public:
    explicit IoTropolisRegistrationServer(QObject* parent = nullptr);
    bool start(quint16 port);

    void setGui(IoTropolisGui* gui);

signals:
    void unitReady(IoTropolisUnitConnection* unit);
    void unitDisconnected(IoTropolisUnitConnection* unit);
    void unitError(IoTropolisUnitConnection* unit, const QString &msg);

private slots:
    void onNewConnection();
    void onUnitHello(IoTropolisUnitConnection* unit);
    void onUnitDescribe(IoTropolisUnitConnection* unit);
    void onUnitProtocolError(IoTropolisUnitConnection* unit, const QString &msg);
    void onUnitDisconnectedInternal(IoTropolisUnitConnection* unit);

private:
    QSet<IoTropolisUnitConnection*> m_units;
    QTcpServer* m_server = nullptr;
    IoTropolisGui* m_gui = nullptr;

    bool validateOrCreateTypeFile(IoTropolisUnitConnection* unit);
};

#endif // IOTROPOLISREGISTRATIONSERVER_H
