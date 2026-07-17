#ifndef CIRCUITCOMPONENT_H
#define CIRCUITCOMPONENT_H

#include <QGraphicsItem>

class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class CircuitComponent : public QGraphicsItem
{
public:
    enum class Type {
        Resistor,
        Capacitor,
        Inductor,
        AndGate,
        OrGate,
        NotGate,
        Ground,
        VoltageSource,
        Switch,
        Led,
        Diode
    };

    explicit CircuitComponent(Type type, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Type m_type;
    bool m_switchClosed = false;
};

#endif
