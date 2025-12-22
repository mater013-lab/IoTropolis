#ifndef IOCOMPONENT_H
#define IOCOMPONENT_H

#include <QString>
#include <QJsonObject>

class IOComponent
{
public:
    IOComponent() = default;
    IOComponent(const QString& name, const QString& format)
        : m_name(name), m_format(format) {}

    const QString& name() const { return m_name; }
    const QString& format() const { return m_format; }

    bool isValid() const { return !m_name.isEmpty() && !m_format.isEmpty(); }

    // JSON helpers
    static IOComponent fromJson(const QJsonObject& obj, bool* ok = nullptr);
    QJsonObject toJson() const;

private:
    QString m_name;
    QString m_format;
};

#endif // IOCOMPONENT_H
