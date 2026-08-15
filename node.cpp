#include "node.h"

Node::Node(const QString &id)
    : m_id(id)
    , m_state(LogicState::Undefined)
{
}

QString Node::id() const
{
    return m_id;
}

const QVector<QString> &Node::connectedPinRefs() const
{
    return m_connectedPinRefs;
}

const QVector<QString> &Node::wireIds() const
{
    return m_wireIds;
}

LogicState Node::state() const
{
    return m_state;
}

void Node::addConnectedPinRef(const QString &pinRef)
{
    if (!m_connectedPinRefs.contains(pinRef)) {
        m_connectedPinRefs.append(pinRef);
    }
}

void Node::addWireId(const QString &wireId)
{
    if (!m_wireIds.contains(wireId)) {
        m_wireIds.append(wireId);
    }
}

void Node::setState(LogicState state)
{
    m_state = state;
}
