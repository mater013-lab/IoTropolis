#ifndef IOTROPOLISUNITCONNECTION_H
#define IOTROPOLISUNITCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QStringList>

class IoTropolisUnitConnection : public QObject
{
    Q_OBJECT
public:
    explicit IoTropolisUnitConnection(QTcpSocket* socket, QObject* parent = nullptr);

    QString ipAddress() const;  // just declaration
    QString unitType() const { return m_unitType; }
    QString unitSubtype() const { return m_unitSubtype; }
    QStringList sensors() const { return m_sensors; }
    QStringList actuators() const { return m_actuators; }

signals:
    void helloCompleted();
    void describeCompleted();
    void protocolError(const QString& msg);
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket;
    bool m_helloDone;
    bool m_describeDone;

    QString m_unitType;
    QString m_unitSubtype;
    QStringList m_sensors;
    QStringList m_actuators;
};

#endif // IOTROPOLISUNITCONNECTION_H
