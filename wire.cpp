#include "wire.h"

Wire::Wire(const QString &id,
           const QString &startComponentId,
           const QString &startPinName,
           const QString &endComponentId,
           const QString &endPinName)
    : m_id(id)
    , m_startComponentId(startComponentId)
    , m_startPinName(startPinName)
    , m_endComponentId(endComponentId)
    , m_endPinName(endPinName)
{
}

QString Wire::id() const
{
    return m_id;
}

QString Wire::startComponentId() const
{
    return m_startComponentId;
}

QString Wire::startPinName() const
{
    return m_startPinName;
}

QString Wire::endComponentId() const
{
    return m_endComponentId;
}

QString Wire::endPinName() const
{
    return m_endPinName;
}

const QVector<QPoint> &Wire::pathPoints() const
{
    return m_pathPoints;
}

void Wire::setPathPoints(const QVector<QPoint> &pathPoints)
{
    m_pathPoints = pathPoints;
}

void Wire::addPathPoint(const QPoint &point)
{
    m_pathPoints.append(point);
}
