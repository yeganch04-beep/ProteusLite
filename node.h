#ifndef NODE_H
#define NODE_H

#include "pin.h"

#include <QString>
#include <QVector>

class Node
{
public:
    explicit Node(const QString &id);

    QString id() const;
    const QVector<QString> &connectedPinRefs() const;
    const QVector<QString> &wireIds() const;
    LogicState state() const;

    void addConnectedPinRef(const QString &pinRef);
    void addWireId(const QString &wireId);
    void setState(LogicState state);

private:
    QString m_id;
    QVector<QString> m_connectedPinRefs;
    QVector<QString> m_wireIds;
    LogicState m_state;
};

#endif // NODE_H
