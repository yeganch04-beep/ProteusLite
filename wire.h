#ifndef WIRE_H
#define WIRE_H

#include "pin.h"

#include <QPoint>
#include <QString>
#include <QVector>

class Wire
{
public:
    Wire(const QString &id,
         const QString &startComponentId,
         const QString &startPinName,
         const QString &endComponentId,
         const QString &endPinName);

    QString id() const;
    QString startComponentId() const;
    QString startPinName() const;
    QString endComponentId() const;
    QString endPinName() const;
    LogicState state() const;
    void setState(LogicState state);

    const QVector<QPoint> &pathPoints() const;
    void setPathPoints(const QVector<QPoint> &pathPoints);
    void addPathPoint(const QPoint &point);

private:
    QString m_id;
    QString m_startComponentId;
    QString m_startPinName;
    QString m_endComponentId;
    QString m_endPinName;
    LogicState m_state;
    QVector<QPoint> m_pathPoints;
};

#endif // WIRE_H
