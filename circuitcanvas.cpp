#include "circuitcanvas.h"

#include <QMouseEvent>
#include <QPainter>

CircuitCanvas::CircuitCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(400, 300);
}

QPoint CircuitCanvas::snapToGrid(const QPoint &point) const
{
    const int snappedX = ((point.x() + GridSpacing / 2) / GridSpacing) * GridSpacing;
    const int snappedY = ((point.y() + GridSpacing / 2) / GridSpacing) * GridSpacing;

    return QPoint(snappedX, snappedY);
}

void CircuitCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    painter.setPen(QPen(QColor(225, 225, 225), 1));

    for (int x = 0; x <= width(); x += GridSpacing) {
        painter.drawLine(x, 0, x, height());
    }

    for (int y = 0; y <= height(); y += GridSpacing) {
        painter.drawLine(0, y, width(), y);
    }
}

void CircuitCanvas::mouseMoveEvent(QMouseEvent *event)
{
    emit mousePositionChanged(snapToGrid(event->pos()));
}
