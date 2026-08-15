#include "pin.h"

Pin::Pin(const QString &name, PinType type, const QPoint &offset)
    : m_name(name)
    , m_type(type)
    , m_offset(offset)
    , m_state(LogicState::Undefined)
{
}

QString Pin::name() const
{
    return m_name;
}

PinType Pin::type() const
{
    return m_type;
}

QPoint Pin::offset() const
{
    return m_offset;
}

LogicState Pin::state() const
{
    return m_state;
}

QString Pin::connectedNodeId() const
{
    return m_connectedNodeId;
}

void Pin::setState(LogicState state)
{
    m_state = state;
}

void Pin::setConnectedNodeId(const QString &nodeId)
{
    m_connectedNodeId = nodeId;
}
