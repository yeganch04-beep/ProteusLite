#include "circuitcanvas.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>

CircuitCanvas::CircuitCanvas(QWidget *parent)
    : QWidget(parent)
    , zoomFactor(1.0)
    , panOffset(0.0, 0.0)
    , isPanning(false)
    , isDraggingComponent(false)
    , isWiringMode(false)
    , hasWireStartPoint(false)
    , nextComponentId(1)
    , selectedComponentIndex(-1)
    , selectedWireIndex(-1)
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
        const QString name = componentDisplayName(placedComponents[selectedComponentIndex].component.name());
        placedComponents.removeAt(selectedComponentIndex);
        selectedComponentIndex = -1;
        emit actionOccurred(QString("Deleted component: %1").arg(name));
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete && selectedWireIndex >= 0) {
        placedWires.removeAt(selectedWireIndex);
        selectedWireIndex = -1;
        emit actionOccurred("Wire deleted");
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_W) {
        isWiringMode = !isWiringMode;
        hasWireStartPoint = false;
        isDraggingComponent = false;
        selectedComponentIndex = -1;
        selectedWireIndex = -1;
        emit actionOccurred(isWiringMode ? "Wire mode enabled" : "Wire mode disabled");
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E) {
        evaluateLogicGates();
        emit actionOccurred("Logic gates evaluated");
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_R && selectedComponentIndex >= 0) {
        PlacedComponent &component = placedComponents[selectedComponentIndex];
        component.rotationDegrees = (component.rotationDegrees + 90) % 360;
        emit actionOccurred(QString("Rotated component: %1").arg(componentDisplayName(component.component.name())));
        update();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void CircuitCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || isWiringMode) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QPoint worldPoint = screenToWorld(event->pos()).toPoint();
    const int clickedIndex = componentAt(worldPoint);

    if (clickedIndex < 0) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    PlacedComponent &component = placedComponents[clickedIndex];
    selectedComponentIndex = clickedIndex;
    selectedWireIndex = -1;
    isDraggingComponent = false;

    QString actionMessage;

    const QString typeName = component.component.name();
    if (typeName == "Switch") {
        component.stateOn = !component.stateOn;
        component.component.findPin("OUT")->setState(component.stateOn ? LogicState::High : LogicState::Low);
        actionMessage = QString("Switch toggled %1").arg(component.stateOn ? "ON" : "OFF");
    } else if (typeName == "VoltageSource") {
        component.stateOn = !component.stateOn;
        component.component.findPin("OUT")->setState(component.stateOn ? LogicState::High : LogicState::Low);
        actionMessage = QString("Digital Voltage Source set to %1").arg(component.stateOn ? 1 : 0);
    } else if (typeName == "Led") {
        component.stateOn = !component.stateOn;
        component.component.findPin("IN")->setState(component.stateOn ? LogicState::High : LogicState::Low);
        actionMessage = QString("LED toggled %1").arg(component.stateOn ? "ON" : "OFF");
    } else if (typeName == "AndGate" || typeName == "OrGate") {
        const int currentInputs = (component.inputA ? 2 : 0) + (component.inputB ? 1 : 0);
        const int nextInputs = (currentInputs + 1) % 4;
        component.inputA = (nextInputs & 2) != 0;
        component.inputB = (nextInputs & 1) != 0;
        component.component.findPin("A")->setState(component.inputA ? LogicState::High : LogicState::Low);
        component.component.findPin("B")->setState(component.inputB ? LogicState::High : LogicState::Low);
        actionMessage = QString("%1 inputs set to A=%2 B=%3")
                            .arg(componentDisplayName(typeName))
                            .arg(component.inputA ? 1 : 0)
                            .arg(component.inputB ? 1 : 0);
    } else if (typeName == "NotGate") {
        component.inputA = !component.inputA;
        component.component.findPin("IN")->setState(component.inputA ? LogicState::High : LogicState::Low);
        actionMessage = QString("NOT Gate input set to IN=%1").arg(component.inputA ? 1 : 0);
    } else {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    emit actionOccurred(QString("%1 - Component state changed: %2").arg(actionMessage, component.label));
    update();
    event->accept();
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
        const QPoint snappedPoint = snapToGrid(worldPoint);

        if (isWiringMode) {
            selectedComponentIndex = -1;
            selectedWireIndex = -1;

            if (!hasWireStartPoint) {
                wireStartPoint = snappedPoint;
                previewWireEndPoint = snappedPoint;
                hasWireStartPoint = true;
                emit actionOccurred(QString("Wire started at X: %1, Y: %2")
                                        .arg(wireStartPoint.x())
                                        .arg(wireStartPoint.y()));
            } else {
                PlacedWire wire;
                wire.startPoint = wireStartPoint;
                wire.endPoint = snappedPoint;
                placedWires.append(wire);
                selectedWireIndex = placedWires.size() - 1;
                hasWireStartPoint = false;
                emit actionOccurred(QString("Wire placed from X: %1, Y: %2 to X: %3, Y: %4")
                                        .arg(wire.startPoint.x())
                                        .arg(wire.startPoint.y())
                                        .arg(wire.endPoint.x())
                                        .arg(wire.endPoint.y()));
            }

            update();
            event->accept();
            return;
        }

        const int clickedWireIndex = wireAt(worldPoint);
        if (clickedWireIndex >= 0) {
            selectedComponentIndex = -1;
            selectedWireIndex = clickedWireIndex;
            emit actionOccurred("Wire selected");
            update();
            event->accept();
            return;
        }

        const int clickedIndex = componentAt(worldPoint);

        if (clickedIndex >= 0) {
            selectedComponentIndex = clickedIndex;
            selectedWireIndex = -1;
            isDraggingComponent = true;
            emit actionOccurred(QString("Selected component: %1")
                                    .arg(componentDisplayName(placedComponents[clickedIndex].component.name())));
            update();
            event->accept();
            return;
        }

        if (!activeComponentType.isEmpty()) {
            const QPoint position = snapToGrid(worldPoint);
            Component model(createComponentId(), activeComponentType, position);
            const QVector<Pin> pins = createPinsForComponent(activeComponentType);
            for (const Pin &pin : pins) {
                model.addPin(pin);
            }
            PlacedComponent component(model, createComponentLabel(activeComponentType));
            placedComponents.append(component);
            selectedComponentIndex = placedComponents.size() - 1;
            selectedWireIndex = -1;
            emit actionOccurred(QString("Placed %1 at X: %2, Y: %3")
                                    .arg(componentDisplayName(component.component.name()))
                                    .arg(component.component.position().x())
                                    .arg(component.component.position().y()));
            update();
            event->accept();
            return;
        }

        selectedComponentIndex = -1;
        selectedWireIndex = -1;
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
                                    .arg(componentDisplayName(component.component.name()))
                                    .arg(component.component.position().x())
                                    .arg(component.component.position().y()));
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

    for (int i = 0; i < placedWires.size(); ++i) {
        drawWire(painter, placedWires[i], i == selectedWireIndex);
    }

    if (isWiringMode && hasWireStartPoint) {
        drawWirePath(painter, orthogonalWirePath(wireStartPoint, previewWireEndPoint), true);
    }

    for (int i = 0; i < placedComponents.size(); ++i) {
        const PlacedComponent &component = placedComponents[i];

        painter.save();
        painter.translate(component.component.position());
        painter.rotate(component.rotationDegrees);
        drawComponent(painter, component);
        drawComponentPins(painter, component);
        painter.restore();

        drawComponentLabel(painter, component);
        drawComponentStateText(painter, component);

        if (i == selectedComponentIndex) {
            drawSelectedComponentBounds(painter, componentBounds(component.component.position()));
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
        placedComponents[selectedComponentIndex].component.setPosition(snapToGrid(worldPoint));
        update();
    }

    if (isWiringMode && hasWireStartPoint) {
        const QPoint worldPoint = screenToWorld(event->pos()).toPoint();
        previewWireEndPoint = snapToGrid(worldPoint);
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
        if (componentBounds(placedComponents[i].component.position()).contains(worldPoint)) {
            return i;
        }
    }

    return -1;
}

int CircuitCanvas::wireAt(const QPoint &worldPoint) const
{
    constexpr double HitTolerance = 8.0;

    for (int i = placedWires.size() - 1; i >= 0; --i) {
        const QVector<QPoint> path = orthogonalWirePath(placedWires[i].startPoint, placedWires[i].endPoint);

        for (int pointIndex = 0; pointIndex + 1 < path.size(); ++pointIndex) {
            if (distanceToSegment(worldPoint, path[pointIndex], path[pointIndex + 1]) <= HitTolerance) {
                return i;
            }
        }
    }

    return -1;
}

QVector<QPoint> CircuitCanvas::orthogonalWirePath(const QPoint &startPoint, const QPoint &endPoint) const
{
    const int middleX = (startPoint.x() + endPoint.x()) / 2;

    return {
        startPoint,
        QPoint(middleX, startPoint.y()),
        QPoint(middleX, endPoint.y()),
        endPoint
    };
}

double CircuitCanvas::distanceToSegment(const QPoint &point, const QPoint &startPoint, const QPoint &endPoint) const
{
    const double dx = endPoint.x() - startPoint.x();
    const double dy = endPoint.y() - startPoint.y();

    if (dx == 0.0 && dy == 0.0) {
        return std::hypot(point.x() - startPoint.x(), point.y() - startPoint.y());
    }

    const double t = std::clamp(((point.x() - startPoint.x()) * dx + (point.y() - startPoint.y()) * dy)
                                    / (dx * dx + dy * dy),
                                0.0,
                                1.0);
    const double closestX = startPoint.x() + t * dx;
    const double closestY = startPoint.y() + t * dy;

    return std::hypot(point.x() - closestX, point.y() - closestY);
}

QRectF CircuitCanvas::componentBounds(const QPoint &position) const
{
    return QRectF(position.x() - 70, position.y() - 55, 140, 110);
}

QVector<Pin> CircuitCanvas::createPinsForComponent(const QString &typeName) const
{
    auto pinWithState = [](const QString &name, PinType type, const QPoint &offset, LogicState state) {
        Pin pin(name, type, offset);
        pin.setState(state);
        return pin;
    };

    if (typeName == "AndGate" || typeName == "OrGate") {
        return {
            pinWithState("A", PinType::Input, QPoint(-58, -16), LogicState::Low),
            pinWithState("B", PinType::Input, QPoint(-58, 16), LogicState::Low),
            pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::Low)
        };
    }
    if (typeName == "NotGate") {
        return {
            pinWithState("IN", PinType::Input, QPoint(-58, 0), LogicState::Low),
            pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::Low)
        };
    }
    if (typeName == "VoltageSource") {
        return {pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::Low)};
    }
    if (typeName == "Switch") {
        return {
            pinWithState("IN", PinType::Input, QPoint(-58, 0), LogicState::Low),
            pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::Low)
        };
    }
    if (typeName == "Led") {
        return {pinWithState("IN", PinType::Input, QPoint(-58, 0), LogicState::Low)};
    }
    if (typeName == "Ground") {
        return {Pin("GND", PinType::Bidirectional, QPoint(0, -38))};
    }
    if (typeName == "Resistor" || typeName == "Capacitor"
        || typeName == "Inductor" || typeName == "Diode") {
        return {
            Pin("P1", PinType::Bidirectional, QPoint(-58, 0)),
            Pin("P2", PinType::Bidirectional, QPoint(58, 0))
        };
    }

    return {};
}

QPoint CircuitCanvas::pinWorldPosition(const PlacedComponent &component, const Pin &pin) const
{
    const double radians = component.rotationDegrees * 3.14159265358979323846 / 180.0;
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const QPoint offset = pin.offset();
    const QPoint rotatedOffset(static_cast<int>(std::round(offset.x() * cosine - offset.y() * sine)),
                               static_cast<int>(std::round(offset.x() * sine + offset.y() * cosine)));
    return component.component.position() + rotatedOffset;
}

bool CircuitCanvas::findNearestPin(const QPoint &worldPoint,
                                   QString *componentId,
                                   QString *pinName,
                                   QPoint *pinPosition) const
{
    constexpr double HitTolerance = 10.0;
    double nearestDistance = std::numeric_limits<double>::max();
    const PlacedComponent *nearestComponent = nullptr;
    const Pin *nearestPin = nullptr;
    QPoint nearestPosition;

    for (const PlacedComponent &component : placedComponents) {
        for (const Pin &pin : component.component.pins()) {
            const QPoint position = pinWorldPosition(component, pin);
            const double distance = std::hypot(worldPoint.x() - position.x(),
                                               worldPoint.y() - position.y());
            if (distance <= HitTolerance && distance < nearestDistance) {
                nearestDistance = distance;
                nearestComponent = &component;
                nearestPin = &pin;
                nearestPosition = position;
            }
        }
    }

    if (nearestComponent == nullptr || nearestPin == nullptr) {
        return false;
    }
    if (componentId != nullptr) {
        *componentId = nearestComponent->component.id();
    }
    if (pinName != nullptr) {
        *pinName = nearestPin->name();
    }
    if (pinPosition != nullptr) {
        *pinPosition = nearestPosition;
    }
    return true;
}

QString CircuitCanvas::createComponentId()
{
    return QString("component-%1").arg(nextComponentId++);
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

bool CircuitCanvas::isLogicGate(const QString &typeName) const
{
    return typeName == "AndGate" || typeName == "OrGate" || typeName == "NotGate";
}

void CircuitCanvas::evaluateLogicGates()
{
    for (PlacedComponent &component : placedComponents) {
        const QString typeName = component.component.name();
        if (!isLogicGate(typeName)) {
            continue;
        }

        if (Pin *inputA = component.component.findPin(typeName == "NotGate" ? "IN" : "A")) {
            inputA->setState(component.inputA ? LogicState::High : LogicState::Low);
        }
        if (Pin *inputB = component.component.findPin("B")) {
            inputB->setState(component.inputB ? LogicState::High : LogicState::Low);
        }

        if (typeName == "AndGate") {
            component.outputValue = (component.inputA && component.inputB) ? 1 : 0;
        } else if (typeName == "OrGate") {
            component.outputValue = (component.inputA || component.inputB) ? 1 : 0;
        } else if (typeName == "NotGate") {
            component.outputValue = component.inputA ? 0 : 1;
        }

        if (Pin *output = component.component.findPin("OUT")) {
            output->setState(component.outputValue == 1 ? LogicState::High : LogicState::Low);
        }
    }
}

void CircuitCanvas::drawComponent(QPainter &painter, const PlacedComponent &component) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    QPen symbolPen(QColor(45, 95, 190), 2);
    symbolPen.setCosmetic(true);
    painter.setPen(symbolPen);

    const QString typeName = component.component.name();
    if (typeName == "Resistor") {
        drawResistor(painter);
    } else if (typeName == "Capacitor") {
        drawCapacitor(painter);
    } else if (typeName == "Inductor") {
        drawInductor(painter);
    } else if (typeName == "Diode") {
        drawDiode(painter, false, false);
    } else if (typeName == "Led") {
        drawDiode(painter, true, component.stateOn);
    } else if (typeName == "Switch") {
        drawSwitch(painter, component.stateOn);
    } else if (typeName == "Ground") {
        drawGround(painter);
    } else if (typeName == "VoltageSource") {
        drawVoltageSource(painter, component.stateOn ? 1 : 0);
    } else if (typeName == "AndGate") {
        drawAndGate(painter);
    } else if (typeName == "OrGate") {
        drawOrGate(painter);
    } else if (typeName == "NotGate") {
        drawNotGate(painter);
    }
}

void CircuitCanvas::drawComponentPins(QPainter &painter, const PlacedComponent &component) const
{
    painter.save();
    QPen pinPen(QColor(20, 65, 145), 2);
    pinPen.setCosmetic(true);
    painter.setPen(pinPen);
    painter.setBrush(Qt::white);

    for (const Pin &pin : component.component.pins()) {
        painter.drawEllipse(QPointF(pin.offset()), 4.0, 4.0);
    }
    painter.restore();
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

    const QPoint position = component.component.position();
    const QRectF labelRect(position.x() - 45, position.y() - 70, 90, 18);
    painter.drawText(labelRect, Qt::AlignCenter, component.label);

    painter.restore();
}

void CircuitCanvas::drawComponentStateText(QPainter &painter, const PlacedComponent &component) const
{
    QString stateText;

    const QString typeName = component.component.name();
    if (typeName == "Switch") {
        stateText = component.stateOn ? "ON" : "OFF";
    } else if (typeName == "VoltageSource") {
        stateText = component.stateOn ? "1" : "0";
    } else if (typeName == "Led") {
        stateText = component.stateOn ? "ON" : "OFF";
    } else if (typeName == "AndGate" || typeName == "OrGate") {
        stateText = QString("A=%1 B=%2 OUT=%3")
                        .arg(component.inputA ? 1 : 0)
                        .arg(component.inputB ? 1 : 0)
                        .arg(component.outputValue);
    } else if (typeName == "NotGate") {
        stateText = QString("IN=%1 OUT=%2")
                        .arg(component.inputA ? 1 : 0)
                        .arg(component.outputValue);
    } else if (isLogicGate(typeName)) {
        stateText = QString("OUT=%1").arg(component.outputValue);
    } else {
        return;
    }

    painter.save();

    const bool highState = (typeName == "Switch" && component.stateOn)
                           || (typeName == "VoltageSource" && component.stateOn)
                           || (typeName == "Led" && component.stateOn)
                           || (isLogicGate(typeName) && component.outputValue == 1);
    QPen statePen(highState ? QColor(0, 130, 0) : QColor(90, 90, 90));
    statePen.setCosmetic(true);
    painter.setPen(statePen);

    QFont stateFont = painter.font();
    stateFont.setBold(true);
    stateFont.setPointSize(8);
    painter.setFont(stateFont);

    const QPoint position = component.component.position();
    const QRectF stateRect(position.x() - 70, position.y() + 48, 140, 18);
    painter.drawText(stateRect, Qt::AlignCenter, stateText);

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

void CircuitCanvas::drawWire(QPainter &painter, const PlacedWire &wire, bool selected) const
{
    drawWirePath(painter, orthogonalWirePath(wire.startPoint, wire.endPoint), selected);
}

void CircuitCanvas::drawWirePath(QPainter &painter, const QVector<QPoint> &path, bool selected) const
{
    if (path.size() < 2) {
        return;
    }

    QPen wirePen(QColor(45, 95, 190), selected ? 4 : 2);
    wirePen.setCosmetic(true);
    if (selected) {
        wirePen.setStyle(Qt::DashLine);
    }

    painter.setPen(wirePen);
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i + 1 < path.size(); ++i) {
        painter.drawLine(path[i], path[i + 1]);
    }
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

void CircuitCanvas::drawDiode(QPainter &painter, bool led, bool ledOn) const
{
    painter.drawLine(-58, 0, -24, 0);
    painter.drawLine(16, 0, 58, 0);

    QPolygonF triangle;
    triangle << QPointF(-24, -22) << QPointF(-24, 22) << QPointF(14, 0);
    if (led && ledOn) {
        painter.save();
        painter.setBrush(QColor(255, 230, 80));
        painter.drawPolygon(triangle);
        painter.restore();
    }
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

void CircuitCanvas::drawSwitch(QPainter &painter, bool closed) const
{
    painter.drawLine(-58, 0, -18, 0);
    painter.drawEllipse(QPointF(-18, 0), 3, 3);
    painter.drawLine(QPointF(-18, 0), closed ? QPointF(22, 0) : QPointF(18, -22));
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

void CircuitCanvas::drawVoltageSource(QPainter &painter, int value) const
{
    painter.drawLine(-58, 0, -24, 0);
    painter.drawEllipse(QPointF(0, 0), 24, 24);
    painter.drawLine(24, 0, 58, 0);
    painter.drawText(QRectF(-18, -8, 12, 16), Qt::AlignCenter, "-");
    painter.drawText(QRectF(6, -8, 12, 16), Qt::AlignCenter, "+");
    painter.drawText(QRectF(-10, 18, 20, 16), Qt::AlignCenter, QString::number(value));
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
