#include "config/IoTropolisConfig.h"
#include <QSettings>
#include <QFile>
#include <QDebug>

IoTropolisConfig::IoTropolisConfig(const QString& configFile)
{
    loadDefaults();

    QString path = configFile.isEmpty() ? defaultConfigPath() : configFile;
    if (QFile::exists(path))
        loadFromFile(path);
    else
        qDebug() << "[IoTropolisConfig] Config file not found, using defaults:" << path;
}

void IoTropolisConfig::loadDefaults()
{
    m_tcpPort = 12345;
    m_bindAddress = "0.0.0.0";
    m_unitTypeDir = "./UnitType";
    m_guiEnabled = true;
}

void IoTropolisConfig::loadFromFile(const QString& path)
{
    QSettings settings(path, QSettings::IniFormat);

    m_tcpPort      = settings.value("server/tcp_port", m_tcpPort).toUInt();
    m_bindAddress  = settings.value("server/bind_address", m_bindAddress).toString();
    m_unitTypeDir  = settings.value("paths/unit_type_dir", m_unitTypeDir).toString();
    m_guiEnabled   = settings.value("gui/enable", m_guiEnabled).toBool();
}

quint16 IoTropolisConfig::tcpPort() const { return m_tcpPort; }
QString IoTropolisConfig::bindAddress() const { return m_bindAddress; }
QString IoTropolisConfig::unitTypeDir() const { return m_unitTypeDir; }
bool IoTropolisConfig::guiEnabled() const { return m_guiEnabled; }

QString IoTropolisConfig::defaultConfigPath()
{
    return "./iotropolis.ini";
}
