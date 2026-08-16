#include "component.h"

Component::Component(const QString &id, const QString &name, const QPoint &position)
    : m_id(id)
    , m_name(name)
    , m_position(position)
{
}

QString Component::id() const
{
    return m_id;
}

QString Component::name() const
{
    return m_name;
}

QPoint Component::position() const
{
    return m_position;
}

void Component::setId(const QString &id)
{
    m_id = id;
}

void Component::setPosition(const QPoint &position)
{
    m_position = position;
}

void Component::addPin(const Pin &pin)
{
    m_pins.append(pin);
}

const QVector<Pin> &Component::pins() const
{
    return m_pins;
}

Pin *Component::findPin(const QString &name)
{
    for (Pin &pin : m_pins) {
        if (pin.name() == name) {
            return &pin;
        }
    }

    return nullptr;
}

const Pin *Component::findPin(const QString &name) const
{
    for (const Pin &pin : m_pins) {
        if (pin.name() == name) {
            return &pin;
        }
    }

    return nullptr;
}
