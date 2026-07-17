#include "circuitcanvas.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

CircuitCanvas::CircuitCanvas(QWidget *parent)
    : QWidget(parent)
    , zoomFactor(1.0)
    , panOffset(0.0, 0.0)
    , isPanning(false)
    , isDraggingComponent(false)
    , selectedComponentIndex(-1)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
}

QPoint CircuitCanvas::snapToGrid(const QPoint &point) const
{
    const int snappedX = static_cast<int>(std::round(point.x() / static_cast<double>(GridSpacing))) * GridSpacing;
    const int snappedY = static_cast<int>(std::round(point.y() / static_cast<double>(GridSpacing))) * GridSpacing;

    return QPoint(snappedX, snappedY);
}

void CircuitCanvas::setActiveComponentType(const QString &typeName)
{
    activeComponentType = typeName;
    emit actionOccurred(QString("Selected component: %1").arg(componentDisplayName(typeName)));
}

void CircuitCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_0) {
        resetView();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete && selectedComponentIndex >= 0) {
        const QString name = componentDisplayName(placedComponents[selectedComponentIndex].typeName);
        placedComponents.removeAt(selectedComponentIndex);
        selectedComponentIndex = -1;
        emit actionOccurred(QString("Deleted component: %1").arg(name));
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_R && selectedComponentIndex >= 0) {
        PlacedComponent &component = placedComponents[selectedComponentIndex];
        component.rotationDegrees = (component.rotationDegrees + 90) % 360;
        emit actionOccurred(QString("Rotated component: %1").arg(componentDisplayName(component.typeName)));
        update();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void CircuitCanvas::mousePressEvent(QMouseEvent *event)
{
    setFocus();

    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const QPoint worldPoint = screenToWorld(event->pos()).toPoint();
        const int clickedIndex = componentAt(worldPoint);

        if (clickedIndex >= 0) {
            selectedComponentIndex = clickedIndex;
            isDraggingComponent = true;
            emit actionOccurred(QString("Selected component: %1")
                                    .arg(componentDisplayName(placedComponents[clickedIndex].typeName)));
            update();
            event->accept();
            return;
        }

        if (!activeComponentType.isEmpty()) {
            PlacedComponent component;
            component.typeName = activeComponentType;
            component.label = createComponentLabel(component.typeName);
            component.position = snapToGrid(worldPoint);
            placedComponents.append(component);
            selectedComponentIndex = placedComponents.size() - 1;
            emit actionOccurred(QString("Placed %1 at X: %2, Y: %3")
                                    .arg(componentDisplayName(component.typeName))
                                    .arg(component.position.x())
                                    .arg(component.position.y()));
            update();
            event->accept();
            return;
        }

        selectedComponentIndex = -1;
        update();
    }

    QWidget::mousePressEvent(event);
}

void CircuitCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && isDraggingComponent) {
        isDraggingComponent = false;
        if (selectedComponentIndex >= 0) {
            const PlacedComponent &component = placedComponents[selectedComponentIndex];
            emit actionOccurred(QString("Moved %1 to X: %2, Y: %3")
                                    .arg(componentDisplayName(component.typeName))
                                    .arg(component.position.x())
                                    .arg(component.position.y()));
        }
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void CircuitCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    painter.translate(panOffset);
    painter.scale(zoomFactor, zoomFactor);

    QPen gridPen(QColor(225, 225, 225));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    const QPointF topLeft = screenToWorld(QPoint(0, 0));
    const QPointF bottomRight = screenToWorld(QPoint(width(), height()));

    const int firstX = static_cast<int>(std::floor(topLeft.x() / GridSpacing)) * GridSpacing;
    const int lastX = static_cast<int>(std::ceil(bottomRight.x() / GridSpacing)) * GridSpacing;
    const int firstY = static_cast<int>(std::floor(topLeft.y() / GridSpacing)) * GridSpacing;
    const int lastY = static_cast<int>(std::ceil(bottomRight.y() / GridSpacing)) * GridSpacing;

    for (int x = firstX; x <= lastX; x += GridSpacing) {
        painter.drawLine(QPointF(x, firstY), QPointF(x, lastY));
    }

    for (int y = firstY; y <= lastY; y += GridSpacing) {
        painter.drawLine(QPointF(firstX, y), QPointF(lastX, y));
    }

    for (int i = 0; i < placedComponents.size(); ++i) {
        const PlacedComponent &component = placedComponents[i];

        painter.save();
        painter.translate(component.position);
        painter.rotate(component.rotationDegrees);
        drawComponent(painter, component.typeName);
        painter.restore();

        drawComponentLabel(painter, component);

        if (i == selectedComponentIndex) {
            drawSelectedComponentBounds(painter, componentBounds(component.position));
        }
    }
}

void CircuitCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (isPanning) {
        const QPoint movement = event->pos() - lastPanPoint;
        panOffset += movement;
        lastPanPoint = event->pos();
        update();
    }

    if (isDraggingComponent && selectedComponentIndex >= 0) {
        const QPoint worldPoint = screenToWorld(event->pos()).toPoint();
        placedComponents[selectedComponentIndex].position = snapToGrid(worldPoint);
        update();
    }

    emitMousePosition(event->pos());
}

void CircuitCanvas::wheelEvent(QWheelEvent *event)
{
    if (!(event->modifiers() & Qt::ControlModifier)) {
        QWidget::wheelEvent(event);
        return;
    }

    const double zoomStep = event->angleDelta().y() > 0 ? 1.1 : 1.0 / 1.1;
    setZoom(zoomFactor * zoomStep, event->position().toPoint());
    event->accept();
}

QPointF CircuitCanvas::screenToWorld(const QPoint &screenPoint) const
{
    return (QPointF(screenPoint) - panOffset) / zoomFactor;
}

void CircuitCanvas::emitMousePosition(const QPoint &screenPoint)
{
    const QPointF worldPosition = screenToWorld(screenPoint);
    emit mousePositionChanged(snapToGrid(worldPosition.toPoint()));
}

int CircuitCanvas::componentAt(const QPoint &worldPoint) const
{
    for (int i = placedComponents.size() - 1; i >= 0; --i) {
        if (componentBounds(placedComponents[i].position).contains(worldPoint)) {
            return i;
        }
    }

    return -1;
}

QRectF CircuitCanvas::componentBounds(const QPoint &position) const
{
    return QRectF(position.x() - 70, position.y() - 55, 140, 110);
}

QString CircuitCanvas::componentDisplayName(const QString &typeName) const
{
    if (typeName == "Led") {
        return "LED";
    }
    if (typeName == "VoltageSource") {
        return "Digital Voltage Source";
    }
    if (typeName == "AndGate") {
        return "AND Gate";
    }
    if (typeName == "OrGate") {
        return "OR Gate";
    }
    if (typeName == "NotGate") {
        return "NOT Gate";
    }

    return typeName;
}

QString CircuitCanvas::createComponentLabel(const QString &typeName)
{
    const QString prefix = labelPrefix(typeName);
    const int nextNumber = labelCounters.value(prefix, 0) + 1;
    labelCounters.insert(prefix, nextNumber);

    return QString("%1%2").arg(prefix).arg(nextNumber);
}

QString CircuitCanvas::labelPrefix(const QString &typeName) const
{
    if (typeName == "Resistor") {
        return "R";
    }
    if (typeName == "Capacitor") {
        return "C";
    }
    if (typeName == "Inductor") {
        return "L";
    }
    if (typeName == "Diode") {
        return "D";
    }
    if (typeName == "Led") {
        return "LED";
    }
    if (typeName == "Switch") {
        return "SW";
    }
    if (typeName == "Ground") {
        return "GND";
    }
    if (typeName == "VoltageSource") {
        return "VDC";
    }
    if (typeName == "AndGate") {
        return "AND";
    }
    if (typeName == "OrGate") {
        return "OR";
    }
    if (typeName == "NotGate") {
        return "NOT";
    }

    return "U";
}

void CircuitCanvas::drawComponent(QPainter &painter, const QString &typeName) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    QPen symbolPen(QColor(45, 95, 190), 2);
    symbolPen.setCosmetic(true);
    painter.setPen(symbolPen);

    if (typeName == "Resistor") {
        drawResistor(painter);
    } else if (typeName == "Capacitor") {
        drawCapacitor(painter);
    } else if (typeName == "Inductor") {
        drawInductor(painter);
    } else if (typeName == "Diode") {
        drawDiode(painter, false);
    } else if (typeName == "Led") {
        drawDiode(painter, true);
    } else if (typeName == "Switch") {
        drawSwitch(painter);
    } else if (typeName == "Ground") {
        drawGround(painter);
    } else if (typeName == "VoltageSource") {
        drawVoltageSource(painter);
    } else if (typeName == "AndGate") {
        drawAndGate(painter);
    } else if (typeName == "OrGate") {
        drawOrGate(painter);
    } else if (typeName == "NotGate") {
        drawNotGate(painter);
    }
}

void CircuitCanvas::drawComponentLabel(QPainter &painter, const PlacedComponent &component) const
{
    painter.save();

    QPen labelPen(Qt::red);
    labelPen.setCosmetic(true);
    painter.setPen(labelPen);

    QFont labelFont = painter.font();
    labelFont.setBold(true);
    labelFont.setPointSize(9);
    painter.setFont(labelFont);

    const QRectF labelRect(component.position.x() - 45, component.position.y() - 70, 90, 18);
    painter.drawText(labelRect, Qt::AlignCenter, component.label);

    painter.restore();
}

void CircuitCanvas::drawSelectedComponentBounds(QPainter &painter, const QRectF &bounds) const
{
    QPen selectedPen(QColor(0, 160, 220), 1, Qt::DashLine);
    selectedPen.setCosmetic(true);
    painter.setPen(selectedPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(bounds.adjusted(2, 2, -2, -2));
}

void CircuitCanvas::drawResistor(QPainter &painter) const
{
    painter.drawLine(-58, 0, -25, 0);
    painter.drawRect(QRectF(-25, -12, 50, 24));
    painter.drawLine(25, 0, 58, 0);
}

void CircuitCanvas::drawCapacitor(QPainter &painter) const
{
    painter.drawLine(-58, 0, -12, 0);
    painter.drawLine(-12, -24, -12, 24);
    painter.drawLine(12, -24, 12, 24);
    painter.drawLine(12, 0, 58, 0);
}

void CircuitCanvas::drawInductor(QPainter &painter) const
{
    painter.drawLine(-58, 0, -36, 0);
    painter.drawLine(36, 0, 58, 0);

    QPainterPath path;
    path.moveTo(-36, 0);
    path.arcTo(QRectF(-36, -16, 18, 32), 180, -180);
    path.arcTo(QRectF(-18, -16, 18, 32), 180, -180);
    path.arcTo(QRectF(0, -16, 18, 32), 180, -180);
    path.arcTo(QRectF(18, -16, 18, 32), 180, -180);
    painter.drawPath(path);
}

void CircuitCanvas::drawDiode(QPainter &painter, bool led) const
{
    painter.drawLine(-58, 0, -24, 0);
    painter.drawLine(16, 0, 58, 0);

    QPolygonF triangle;
    triangle << QPointF(-24, -22) << QPointF(-24, 22) << QPointF(14, 0);
    painter.drawPolygon(triangle);
    painter.drawLine(16, -24, 16, 24);

    if (led) {
        painter.drawLine(QPointF(-2, -30), QPointF(12, -44));
        painter.drawLine(QPointF(12, -44), QPointF(10, -35));
        painter.drawLine(QPointF(12, -44), QPointF(3, -42));
        painter.drawLine(QPointF(14, -26), QPointF(28, -40));
        painter.drawLine(QPointF(28, -40), QPointF(26, -31));
        painter.drawLine(QPointF(28, -40), QPointF(19, -38));
    }
}

void CircuitCanvas::drawSwitch(QPainter &painter) const
{
    painter.drawLine(-58, 0, -18, 0);
    painter.drawEllipse(QPointF(-18, 0), 3, 3);
    painter.drawLine(-18, 0, 18, -22);
    painter.drawEllipse(QPointF(22, 0), 3, 3);
    painter.drawLine(22, 0, 58, 0);
}

void CircuitCanvas::drawGround(QPainter &painter) const
{
    painter.drawLine(0, -38, 0, -12);
    painter.drawLine(-26, -12, 26, -12);
    painter.drawLine(-18, -2, 18, -2);
    painter.drawLine(-10, 8, 10, 8);
}

void CircuitCanvas::drawVoltageSource(QPainter &painter) const
{
    painter.drawLine(-58, 0, -24, 0);
    painter.drawEllipse(QPointF(0, 0), 24, 24);
    painter.drawLine(24, 0, 58, 0);
    painter.drawText(QRectF(-18, -8, 12, 16), Qt::AlignCenter, "-");
    painter.drawText(QRectF(6, -8, 12, 16), Qt::AlignCenter, "+");
}

void CircuitCanvas::drawAndGate(QPainter &painter) const
{
    painter.drawLine(-58, -16, -28, -16);
    painter.drawLine(-58, 16, -28, 16);
    painter.drawLine(24, 0, 58, 0);

    QPainterPath path;
    path.moveTo(-28, -30);
    path.lineTo(0, -30);
    path.arcTo(QRectF(-30, -30, 60, 60), 90, -180);
    path.lineTo(-28, 30);
    path.closeSubpath();
    painter.drawPath(path);
}

void CircuitCanvas::drawOrGate(QPainter &painter) const
{
    painter.drawLine(-58, -16, -28, -16);
    painter.drawLine(-58, 16, -28, 16);
    painter.drawLine(26, 0, 58, 0);

    QPainterPath path;
    path.moveTo(-34, -30);
    path.quadTo(-12, 0, -34, 30);
    path.quadTo(2, 26, 28, 0);
    path.quadTo(2, -26, -34, -30);
    path.closeSubpath();
    painter.drawPath(path);
}

void CircuitCanvas::drawNotGate(QPainter &painter) const
{
    painter.drawLine(-58, 0, -26, 0);
    painter.drawLine(32, 0, 58, 0);

    QPolygonF triangle;
    triangle << QPointF(-26, -26) << QPointF(-26, 26) << QPointF(22, 0);
    painter.drawPolygon(triangle);
    painter.drawEllipse(QPointF(28, 0), 5, 5);
}

void CircuitCanvas::resetView()
{
    zoomFactor = 1.0;
    panOffset = QPointF(0.0, 0.0);
    emit zoomChanged(100);
    update();
}

void CircuitCanvas::setZoom(double newZoom, const QPoint &anchorPoint)
{
    newZoom = std::clamp(newZoom, MinimumZoom, MaximumZoom);

    const QPointF worldBeforeZoom = screenToWorld(anchorPoint);
    zoomFactor = newZoom;
    panOffset = QPointF(anchorPoint) - worldBeforeZoom * zoomFactor;

    emit zoomChanged(static_cast<int>(std::round(zoomFactor * 100.0)));
    update();
}
