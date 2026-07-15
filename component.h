#ifndef COMPONENT_H
#define COMPONENT_H

#include "pin.h"

#include <QPoint>
#include <QString>
#include <QVector>

class Component
{
public:
    Component(const QString &id, const QString &name, const QPoint &position);

    QString id() const;
    QString name() const;
    QPoint position() const;

    void setPosition(const QPoint &position);
    void addPin(const Pin &pin);

    const QVector<Pin> &pins() const;
    Pin *findPin(const QString &name);
    const Pin *findPin(const QString &name) const;

private:
    QString m_id;
    QString m_name;
    QPoint m_position;
    QVector<Pin> m_pins;
};

#endif // COMPONENT_H
