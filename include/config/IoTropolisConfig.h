#pragma once
#include <QString>

class IoTropolisConfig
{
public:
    explicit IoTropolisConfig(const QString& configFile = QString());

    // Accessors
    quint16 tcpPort() const;
    QString bindAddress() const;
    QString unitTypeDir() const;
    bool guiEnabled() const;

    // Default location for INI file
    static QString defaultConfigPath();

private:
    void loadDefaults();
    void loadFromFile(const QString& path);

    quint16 m_tcpPort;
    QString m_bindAddress;
    QString m_unitTypeDir;
    bool m_guiEnabled;
};
