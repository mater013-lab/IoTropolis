#include "registration/IOComponent.h"

IOComponent IOComponent::fromJson(const QJsonObject& obj, bool* ok)
{
    if (!obj.contains("name") || !obj.contains("format")) {
        if (ok) *ok = false;
        return {};
    }

    IOComponent c(obj.value("name").toString(),
                  obj.value("format").toString());

    if (ok) *ok = c.isValid();
    return c;
}

QJsonObject IOComponent::toJson() const
{
    QJsonObject obj;
    obj["name"] = m_name;
    obj["format"] = m_format;
    return obj;
}
