
#include "circuitcomponent.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>

        CircuitComponent::CircuitComponent(Type type, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_type(type)
{
    setFlags(QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setTransformOriginPoint(0, 0);
}

QRectF CircuitComponent::boundingRect() const
{
    return QRectF(-70, -55, 140, 110);
}

void CircuitComponent::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (isSelected()) {
        painter->setPen(QPen(QColor(0, 220, 255), 2, Qt::DashLine));
        painter->drawRect(boundingRect().adjusted(2, 2, -2, -2));
    }

    const QPen shapePen(QColor(60, 140, 255), 2);
    const QPen textPen(QColor(255, 70, 70), 1);

    painter->setBrush(Qt::NoBrush);

    QFont font = painter->font();
    font.setPointSize(9);
    font.setBold(true);
    painter->setFont(font);

    switch (m_type) {
    case Type::Resistor: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -20, 0);
        painter->drawLine(20, 0, 45, 0);
        painter->drawRect(-20, -10, 40, 20);

        painter->setPen(textPen);
        painter->drawText(QRectF(-15, -38, 30, 16), Qt::AlignCenter, "R");
        break;
    }

    case Type::Capacitor: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -10, 0);
        painter->drawLine(10, 0, 45, 0);
        painter->drawLine(-10, -16, -10, 16);
        painter->drawLine(10, -16, 10, 16);

        painter->setPen(textPen);
        painter->drawText(QRectF(-15, -38, 30, 16), Qt::AlignCenter, "C");
        break;
    }

    case Type::Inductor: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -30, 0);
        painter->drawLine(28, 0, 45, 0);

        QPainterPath path;
        path.arcMoveTo(QRectF(-30, -10, 16, 20), 180);
        path.arcTo(QRectF(-30, -10, 16, 20), 180, -180);
        path.arcTo(QRectF(-16, -10, 16, 20), 180, -180);
        path.arcTo(QRectF(-2, -10, 16, 20), 180, -180);
        path.arcTo(QRectF(12, -10, 16, 20), 180, -180);
        painter->drawPath(path);

        painter->setPen(textPen);
        painter->drawText(QRectF(-15, -38, 30, 16), Qt::AlignCenter, "L");
        break;
    }

    case Type::AndGate: {
        painter->setPen(shapePen);
        painter->drawLine(-45, -12, -20, -12);
        painter->drawLine(-45, 12, -20, 12);
        painter->drawLine(20, 0, 45, 0);

        QPainterPath path;
        path.moveTo(-20, -20);
        path.lineTo(0, -20);
        path.arcTo(QRectF(-20, -20, 40, 40), 90, -180);
        path.lineTo(-20, 20);
        path.closeSubpath();
        painter->drawPath(path);

        painter->setPen(textPen);
        painter->drawText(QRectF(-20, -12, 40, 24), Qt::AlignCenter, "AND");
        break;
    }

    case Type::OrGate: {
        painter->setPen(shapePen);
        painter->drawLine(-45, -12, -15, -12);
        painter->drawLine(-45, 12, -15, 12);
        painter->drawLine(20, 0, 45, 0);

        QPainterPath path;
        path.moveTo(-20, -20);
        path.quadTo(-5, 0, -20, 20);
        path.quadTo(8, 18, 20, 0);
        path.quadTo(8, -18, -20, -20);
        path.closeSubpath();
        painter->drawPath(path);

        painter->setPen(textPen);
        painter->drawText(QRectF(-20, -12, 40, 24), Qt::AlignCenter, "OR");
        break;
    }

    case Type::NotGate: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -20, 0);
        painter->drawLine(30, 0, 45, 0);

        QPolygonF triangle;
        triangle << QPointF(-20, -18) << QPointF(-20, 18) << QPointF(18, 0);
        painter->drawPolygon(triangle);

        painter->drawEllipse(QPointF(23, 0), 4, 4);

        painter->setPen(textPen);
        painter->drawText(QRectF(-24, -12, 40, 24), Qt::AlignCenter, "NOT");
        break;
    }

    case Type::Ground: {
        painter->setPen(shapePen);
        painter->drawLine(0, -24, 0, -6);
        painter->drawLine(-14, -6, 14, -6);
        painter->drawLine(-9, 0, 9, 0);
        painter->drawLine(-4, 6, 4, 6);

        painter->setPen(textPen);
        painter->drawText(QRectF(-25, 12, 50, 16), Qt::AlignCenter, "GND");
        break;
    }

    case Type::VoltageSource: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -18, 0);
        painter->drawLine(18, 0, 45, 0);
        painter->drawEllipse(QPointF(0, 0), 18, 18);

        painter->setPen(textPen);
        painter->drawText(QRectF(-14, -7, 10, 14), Qt::AlignCenter, "-");
        painter->drawText(QRectF(4, -7, 10, 14), Qt::AlignCenter, "+");
        painter->drawText(QRectF(-35, -38, 70, 16), Qt::AlignCenter, "VDC");
        break;
    }

    case Type::Switch: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -12, 0);
        painter->drawEllipse(QPointF(-12, 0), 2, 2);
        painter->drawEllipse(QPointF(18, 0), 2, 2);

        if (m_switchClosed) {
            painter->drawLine(-12, 0, 18, 0);
        } else {
            painter->drawLine(-12, 0, 10, -15);
        }

        painter->drawLine(18, 0, 45, 0);

        painter->setPen(textPen);
        painter->drawText(QRectF(-28, -38, 56, 16), Qt::AlignCenter,
                          m_switchClosed ? "SW ON" : "SW OFF");
        break;
    }

    case Type::Led: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -20, 0);
        painter->drawLine(10, 0, 45, 0);

        QPolygonF triangle;
        triangle << QPointF(-20, -16) << QPointF(-20, 16) << QPointF(8, 0);
        painter->drawPolygon(triangle);
        painter->drawLine(10, -18, 10, 18);

        painter->drawLine(QPointF(-4, -24), QPointF(4, -34));
        painter->drawLine(QPointF(4, -34), QPointF(4, -29));
        painter->drawLine(QPointF(4, -34), QPointF(-1, -34));

        painter->drawLine(QPointF(8, -20), QPointF(16, -30));
        painter->drawLine(QPointF(16, -30), QPointF(16, -25));
        painter->drawLine(QPointF(16, -30), QPointF(11, -30));

        painter->setPen(textPen);
        painter->drawText(QRectF(-34, -44, 40, 16), Qt::AlignCenter, "LED");
        break;
    }

    case Type::Diode: {
        painter->setPen(shapePen);
        painter->drawLine(-45, 0, -20, 0);
        painter->drawLine(10, 0, 45, 0);

        QPolygonF triangle;
        triangle << QPointF(-20, -16) << QPointF(-20, 16) << QPointF(8, 0);
        painter->drawPolygon(triangle);
        painter->drawLine(10, -18, 10, 18);

        painter->setPen(textPen);
        painter->drawText(QRectF(-20, -38, 40, 16), Qt::AlignCenter, "D");
        break;
    }
    }
}

QVariant CircuitComponent::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        update();
    }
    return QGraphicsItem::itemChange(change, value);
}

void CircuitComponent::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_type == Type::Switch) {
        m_switchClosed = !m_switchClosed;
        update();
        event->accept();
    }

    QGraphicsItem::mousePressEvent(event);
}

void CircuitComponent::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setRotation(rotation() + 90.0);
        update();
        event->accept();
        return;
    }

    QGraphicsItem::mouseDoubleClickEvent(event);
}