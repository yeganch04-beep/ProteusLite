#include "circuitcanvas.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTimer>
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
    , documentCanvasSize(794, 1123)
    , simulationTimer(new QTimer(this))
    , currentSimulationState(SimulationState::Stopped)
    , nextComponentId(1)
    , nextWireId(1)
    , selectedComponentIndex(-1)
    , selectedWireIndex(-1)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);

    simulationTimer->setInterval(SimulationIntervalMs);
    simulationTimer->setTimerType(Qt::PreciseTimer);
    connect(simulationTimer, &QTimer::timeout,
            this, &CircuitCanvas::performSimulationStep);
}

CircuitCanvas::~CircuitCanvas()
{
    simulationTimer->stop();
}

void CircuitCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (event == nullptr || event->mimeData() == nullptr) {
        return;
    }

    const QString typeName = event->mimeData()->text().trimmed();
    if (createPinsForComponent(typeName).isEmpty()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void CircuitCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    if (event == nullptr || event->mimeData() == nullptr) {
        return;
    }

    const QString typeName = event->mimeData()->text().trimmed();
    if (createPinsForComponent(typeName).isEmpty()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void CircuitCanvas::dropEvent(QDropEvent *event)
{
    if (event == nullptr || event->mimeData() == nullptr) {
        return;
    }

    const QString typeName = event->mimeData()->text().trimmed();
    const QPoint worldPoint = screenToWorld(event->position().toPoint()).toPoint();

    if (!placeComponent(typeName, worldPoint)) {
        event->ignore();
        return;
    }

    activeComponentType = typeName;
    event->acceptProposedAction();
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

void CircuitCanvas::setDocumentCanvasSize(const QSize &size)
{
    if (!size.isValid() || size.isEmpty()) {
        return;
    }
    documentCanvasSize = size;
    resetView();
    emit actionOccurred(QString("Canvas size applied: %1 x %2")
                            .arg(documentCanvasSize.width())
                            .arg(documentCanvasSize.height()));
    update();
}

QPoint CircuitCanvas::boundedComponentPosition(const QPoint &worldPosition) const
{
    QPoint bounded = snapToGrid(worldPosition);
    if (!documentCanvasSize.isValid() || documentCanvasSize.isEmpty()) {
        return bounded;
    }

    constexpr int HorizontalMargin = 80;
    constexpr int VerticalMargin = 60;
    const int maximumX = std::max(HorizontalMargin,
                                  (documentCanvasSize.width() - HorizontalMargin)
                                      / GridSpacing * GridSpacing);
    const int maximumY = std::max(VerticalMargin,
                                  (documentCanvasSize.height() - VerticalMargin)
                                      / GridSpacing * GridSpacing);
    bounded.setX(std::clamp(bounded.x(), HorizontalMargin, maximumX));
    bounded.setY(std::clamp(bounded.y(), VerticalMargin, maximumY));
    return bounded;
}

bool CircuitCanvas::placeComponent(const QString &typeName, const QPoint &worldPosition)
{
    const QString normalizedTypeName = typeName.trimmed();
    const QVector<Pin> pins = createPinsForComponent(normalizedTypeName);
    if (normalizedTypeName.isEmpty() || pins.isEmpty()) {
        emit actionOccurred(QString("Placement failed: unsupported component type: %1")
                                .arg(normalizedTypeName));
        return false;
    }

    const QPoint snappedPosition = boundedComponentPosition(worldPosition);
    Component model(createComponentId(), normalizedTypeName, snappedPosition);
    for (const Pin &pin : pins) {
        model.addPin(pin);
    }

    placedComponents.append(
        PlacedComponent(model,
                        createComponentLabel(normalizedTypeName),
                        defaultComponentValue(normalizedTypeName)));
    selectedComponentIndex = placedComponents.size() - 1;
    selectedWireIndex = -1;
    evaluateCircuit();

    emit actionOccurred(QString("Placed %1 at X: %2, Y: %3")
                            .arg(componentDisplayName(normalizedTypeName))
                            .arg(snappedPosition.x())
                            .arg(snappedPosition.y()));
    update();
    return true;
}

CircuitCanvas::SimulationState CircuitCanvas::simulationState() const
{
    return currentSimulationState;
}

ProjectFileData CircuitCanvas::projectData(const QString &projectName,
                                           const QSize &canvasSize) const
{
    ProjectFileData project;
    project.projectName = projectName;
    project.canvasSize = canvasSize;
    project.components.reserve(placedComponents.size());
    project.wires.reserve(placedWires.size());

    for (const PlacedComponent &placed : placedComponents) {
        ProjectComponentData component;
        component.id = placed.component.id();
        component.type = placed.component.name();
        component.label = placed.label;
        component.value = placed.value;
        component.position = placed.component.position();
        component.rotationDegrees = placed.rotationDegrees;
        component.mirrored = placed.mirrored;
        component.mirroredVertically = placed.mirroredVertically;
        component.stateOn = placed.stateOn;
        project.components.append(component);
    }

    for (const Wire &placed : placedWires) {
        ProjectWireData wire;
        wire.id = placed.id();
        wire.startComponentId = placed.startComponentId();
        wire.startPinName = placed.startPinName();
        wire.endComponentId = placed.endComponentId();
        wire.endPinName = placed.endPinName();
        project.wires.append(wire);
    }

    return project;
}

bool CircuitCanvas::loadProjectData(const ProjectFileData &project,
                                    QString *errorMessage)
{
    auto fail = [&](const QString &message) {
        placedComponents.clear();
        placedWires.clear();
        nodes.clear();
        labelCounters.clear();
        nextComponentId = 1;
        nextWireId = 1;
        selectedComponentIndex = -1;
        selectedWireIndex = -1;
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        update();
        return false;
    };

    stopSimulation();
    placedComponents.clear();
    placedWires.clear();
    nodes.clear();
    labelCounters.clear();
    activeComponentType.clear();
    selectedComponentIndex = -1;
    selectedWireIndex = -1;
    isDraggingComponent = false;
    isWiringMode = false;
    hasWireStartPoint = false;
    wireStartComponentId.clear();
    wireStartPinName.clear();

    QSet<QString> componentIds;
    quint64 maximumComponentId = 0;
    quint64 maximumWireId = 0;
    const QRegularExpression componentIdPattern("^component-(\\d+)$");
    const QRegularExpression wireIdPattern("^wire-(\\d+)$");
    const QRegularExpression labelPattern("^(.+?)(\\d+)$");

    for (const ProjectComponentData &stored : project.components) {
        if (stored.id.trimmed().isEmpty() || componentIds.contains(stored.id)) {
            return fail(QString("Invalid or duplicate component id: %1").arg(stored.id));
        }
        const QVector<Pin> pins = createPinsForComponent(stored.type);
        if (pins.isEmpty()) {
            return fail(QString("Unsupported component type: %1").arg(stored.type));
        }

        Component model(stored.id, stored.type, stored.position);
        for (const Pin &pin : pins) {
            model.addPin(pin);
        }
        PlacedComponent placed(model,
                               stored.label,
                               stored.value.isEmpty()
                                   ? defaultComponentValue(stored.type)
                                   : stored.value);
        placed.rotationDegrees = ((stored.rotationDegrees % 360) + 360) % 360;
        placed.mirrored = stored.mirrored;
        placed.mirroredVertically = stored.mirroredVertically;
        placed.stateOn = stored.stateOn;
        placedComponents.append(placed);
        componentIds.insert(stored.id);

        const QRegularExpressionMatch idMatch = componentIdPattern.match(stored.id);
        if (idMatch.hasMatch()) {
            maximumComponentId = std::max(maximumComponentId, idMatch.captured(1).toULongLong());
        }
        const QRegularExpressionMatch labelMatch = labelPattern.match(stored.label);
        if (labelMatch.hasMatch()) {
            const QString prefix = labelMatch.captured(1);
            labelCounters.insert(prefix,
                                 std::max(labelCounters.value(prefix),
                                          labelMatch.captured(2).toInt()));
        }
    }

    QSet<QString> wireIds;
    for (const ProjectWireData &stored : project.wires) {
        if (stored.id.trimmed().isEmpty() || wireIds.contains(stored.id)) {
            return fail(QString("Invalid or duplicate wire id: %1").arg(stored.id));
        }
        const Pin *startPin = findPin(stored.startComponentId, stored.startPinName);
        const Pin *endPin = findPin(stored.endComponentId, stored.endPinName);
        if (startPin == nullptr || endPin == nullptr) {
            return fail(QString("Wire %1 references a missing component pin.").arg(stored.id));
        }
        if (!pinDirectionsAreCompatible(*startPin, *endPin)) {
            return fail(QString("Wire %1 has incompatible pin directions.").arg(stored.id));
        }
        if (isDuplicateConnection(stored.startComponentId, stored.startPinName,
                                  stored.endComponentId, stored.endPinName)) {
            return fail(QString("Wire %1 duplicates an existing connection.").arg(stored.id));
        }
        if ((startPin->type() == PinType::Input
             && pinHasConnection(stored.startComponentId, stored.startPinName))
            || (endPin->type() == PinType::Input
                && pinHasConnection(stored.endComponentId, stored.endPinName))) {
            return fail(QString("Wire %1 connects to an input pin that is already used.").arg(stored.id));
        }

        placedWires.append(Wire(stored.id,
                                stored.startComponentId,
                                stored.startPinName,
                                stored.endComponentId,
                                stored.endPinName));
        wireIds.insert(stored.id);
        const QRegularExpressionMatch idMatch = wireIdPattern.match(stored.id);
        if (idMatch.hasMatch()) {
            maximumWireId = std::max(maximumWireId, idMatch.captured(1).toULongLong());
        }
    }

    nextComponentId = maximumComponentId + 1;
    nextWireId = maximumWireId + 1;
    resetSimulationRuntime();
    lastSimulationStatus.clear();
    for (int index = 0; index < placedComponents.size(); ++index) {
        placedComponents[index].stateOn = project.components[index].stateOn;
    }
    evaluateCircuit();
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    emit actionOccurred(QString("Loaded %1 component(s) and %2 wire(s)")
                            .arg(placedComponents.size())
                            .arg(placedWires.size()));
    update();
    return true;
}

bool CircuitCanvas::exportToPng(const QString &fileName,
                                QString *errorMessage)
{
    if (fileName.trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "A destination file name is required.";
        }
        return false;
    }

    if (!documentCanvasSize.isValid() || documentCanvasSize.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "The document canvas size is invalid.";
        }
        return false;
    }

    QPixmap image(documentCanvasSize);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen gridPen(QColor(225, 225, 225));
    painter.setPen(gridPen);
    for (int x = 0; x <= documentCanvasSize.width(); x += GridSpacing) {
        painter.drawLine(QPoint(x, 0), QPoint(x, documentCanvasSize.height()));
    }
    for (int y = 0; y <= documentCanvasSize.height(); y += GridSpacing) {
        painter.drawLine(QPoint(0, y), QPoint(documentCanvasSize.width(), y));
    }

    for (const Wire &wire : placedWires) {
        drawWire(painter, wire, false);
    }
    for (const PlacedComponent &component : placedComponents) {
        painter.save();
        painter.translate(component.component.position());
        painter.rotate(component.rotationDegrees);
        if (component.mirrored || component.mirroredVertically) {
            painter.scale(component.mirrored ? -1.0 : 1.0,
                          component.mirroredVertically ? -1.0 : 1.0);
        }
        drawComponent(painter, component);
        drawComponentPins(painter, component);
        painter.restore();
        drawComponentLabel(painter, component);
        drawComponentStateText(painter, component);
    }
    painter.end();

    if (!image.save(fileName, "PNG")) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Could not export the circuit canvas to: %1").arg(fileName);
        }
        return false;
    }

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

void CircuitCanvas::runSimulation()
{
    if (currentSimulationState == SimulationState::Running) {
        return;
    }

    const bool resuming = currentSimulationState == SimulationState::Paused;
    setSimulationState(SimulationState::Running);
    performSimulationStep();
    simulationTimer->start();
    QString message = resuming ? "Simulation resumed" : "Simulation started";
    if (!lastSimulationStatus.isEmpty()) {
        message += " - " + lastSimulationStatus;
    }
    emit actionOccurred(message);
}

void CircuitCanvas::pauseSimulation()
{
    if (currentSimulationState != SimulationState::Running) {
        return;
    }

    simulationTimer->stop();
    setSimulationState(SimulationState::Paused);
    emit actionOccurred("Simulation paused");
}

void CircuitCanvas::stopSimulation()
{
    simulationTimer->stop();
    if (currentSimulationState == SimulationState::Stopped) {
        return;
    }

    setSimulationState(SimulationState::Stopped);
    emit actionOccurred("Simulation stopped");
}

void CircuitCanvas::resetSimulation()
{
    simulationTimer->stop();
    resetSimulationRuntime();
    lastSimulationStatus.clear();
    setSimulationState(SimulationState::Stopped);
    emit actionOccurred("Simulation reset");
    update();
}

void CircuitCanvas::stepSimulation()
{
    if (currentSimulationState == SimulationState::Running) {
        emit actionOccurred("Pause the simulation before using Step");
        return;
    }

    simulationTimer->stop();
    performSimulationStep();
    QString message = "Simulation advanced by one step";
    if (!lastSimulationStatus.isEmpty()) {
        message += " - " + lastSimulationStatus;
    }
    emit actionOccurred(message);
}

void CircuitCanvas::editSelectedComponentProperties()
{
    if (selectedComponentIndex < 0 || selectedComponentIndex >= placedComponents.size()) {
        emit actionOccurred("Select a component before opening Properties");
        return;
    }

    PlacedComponent &selected = placedComponents[selectedComponentIndex];
    QDialog dialog(this);
    dialog.setWindowTitle(QString("Properties - %1").arg(selected.label));
    dialog.setMinimumWidth(360);

    auto *layout = new QFormLayout(&dialog);
    auto *typeLabel = new QLabel(componentDisplayName(selected.component.name()), &dialog);
    auto *idEdit = new QLineEdit(selected.component.id(), &dialog);
    auto *labelEdit = new QLineEdit(selected.label, &dialog);
    auto *valueEdit = new QLineEdit(selected.value, &dialog);
    idEdit->setObjectName("propertyIdEdit");
    labelEdit->setObjectName("propertyLabelEdit");
    valueEdit->setObjectName("propertyValueEdit");
    idEdit->setMaxLength(60);
    labelEdit->setMaxLength(40);
    valueEdit->setMaxLength(60);
    layout->addRow("Type:", typeLabel);
    layout->addRow("ID:", idEdit);
    layout->addRow("Label:", labelEdit);
    layout->addRow("Value:", valueEdit);

    QComboBox *stateCombo = nullptr;
    const QString typeName = selected.component.name();
    if (typeName == "VoltageSource" || typeName == "Switch") {
        stateCombo = new QComboBox(&dialog);
        stateCombo->setObjectName("propertyStateCombo");
        if (typeName == "VoltageSource") {
            stateCombo->addItems({"0 (Low)", "1 (High)"});
        } else {
            stateCombo->addItems({"Open", "Closed"});
        }
        stateCombo->setCurrentIndex(selected.stateOn ? 1 : 0);
        layout->addRow("State:", stateCombo);
    } else {
        auto *stateLabel = new QLabel("Not interactive", &dialog);
        layout->addRow("State:", stateLabel);
    }

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    while (dialog.exec() == QDialog::Accepted) {
        const QString newId = idEdit->text().trimmed();
        const QString newLabel = labelEdit->text().trimmed();
        const QString newValue = valueEdit->text().trimmed();
        if (newId.isEmpty() || newLabel.isEmpty()) {
            QMessageBox::warning(&dialog, "Properties", "ID and Label cannot be empty.");
            continue;
        }

        bool duplicateId = false;
        bool duplicateLabel = false;
        for (int index = 0; index < placedComponents.size(); ++index) {
            if (index == selectedComponentIndex) {
                continue;
            }
            duplicateId = duplicateId
                          || placedComponents[index].component.id().compare(
                                 newId, Qt::CaseInsensitive) == 0;
            duplicateLabel = duplicateLabel
                             || placedComponents[index].label.compare(
                                    newLabel, Qt::CaseInsensitive) == 0;
        }
        if (duplicateId) {
            QMessageBox::warning(&dialog, "Properties", "Component IDs must be unique.");
            continue;
        }
        if (duplicateLabel) {
            QMessageBox::warning(&dialog, "Properties", "Component labels must be unique.");
            continue;
        }

        const QString oldId = selected.component.id();
        const QString oldLabel = selected.label;
        if (oldId != newId) {
            selected.component.setId(newId);
            for (Wire &wire : placedWires) {
                wire.replaceComponentId(oldId, newId);
            }
            if (hoveredPinComponentId == oldId) {
                hoveredPinComponentId = newId;
            }
            if (wireStartComponentId == oldId) {
                wireStartComponentId = newId;
            }
        }
        selected.label = newLabel;
        selected.value = newValue;
        if (stateCombo != nullptr) {
            selected.stateOn = stateCombo->currentIndex() == 1;
        }

        const QString simulationStatus = evaluateCircuit();
        QString message = QString("Properties updated: %1 -> %2; ID=%3; Value=%4")
                              .arg(oldLabel,
                                   selected.label,
                                   selected.component.id(),
                                   selected.value.isEmpty() ? "-" : selected.value);
        if (!simulationStatus.isEmpty()) {
            message += " - " + simulationStatus;
        }
        emit actionOccurred(message);
        update();
        return;
    }
}

void CircuitCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_0) {
        resetView();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete && selectedComponentIndex >= 0) {
        const QString componentId = placedComponents[selectedComponentIndex].component.id();
        const QString name = componentDisplayName(placedComponents[selectedComponentIndex].component.name());
        int deletedWireCount = 0;
        for (int wireIndex = placedWires.size() - 1; wireIndex >= 0; --wireIndex) {
            const Wire &wire = placedWires[wireIndex];
            if (wire.startComponentId() == componentId || wire.endComponentId() == componentId) {
                placedWires.removeAt(wireIndex);
                ++deletedWireCount;
            }
        }
        placedComponents.removeAt(selectedComponentIndex);
        selectedComponentIndex = -1;
        selectedWireIndex = -1;
        QString message = deletedWireCount == 0
                              ? QString("Deleted component: %1").arg(name)
                              : QString("Deleted component: %1; deleted %2 connected wire(s)")
                                    .arg(name)
                                    .arg(deletedWireCount);
        const QString simulationStatus = evaluateCircuit();
        if (!simulationStatus.isEmpty()) {
            message += " - " + simulationStatus;
        }
        emit actionOccurred(message);
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete && selectedWireIndex >= 0) {
        placedWires.removeAt(selectedWireIndex);
        selectedWireIndex = -1;
        QString message = "Wire deleted; circuit reevaluated";
        const QString simulationStatus = evaluateCircuit();
        if (!simulationStatus.isEmpty()) {
            message += " - " + simulationStatus;
        }
        emit actionOccurred(message);
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_W) {
        isWiringMode = !isWiringMode;
        hasWireStartPoint = false;
        wireStartComponentId.clear();
        wireStartPinName.clear();
        isDraggingComponent = false;
        selectedComponentIndex = -1;
        selectedWireIndex = -1;
        emit actionOccurred(isWiringMode ? "Wire mode enabled" : "Wire mode disabled");
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape && hasWireStartPoint) {
        hasWireStartPoint = false;
        wireStartComponentId.clear();
        wireStartPinName.clear();
        emit actionOccurred("Wire creation cancelled");
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E) {
        const QString simulationStatus = evaluateCircuit();
        emit actionOccurred(simulationStatus.isEmpty()
                                ? "Circuit evaluated"
                                : QString("Circuit evaluated - %1").arg(simulationStatus));
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_P) {
        editSelectedComponentProperties();
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

    if (event->key() == Qt::Key_M && selectedComponentIndex >= 0) {
        PlacedComponent &component = placedComponents[selectedComponentIndex];
        const bool verticalMirror = event->modifiers().testFlag(Qt::ShiftModifier);
        if (verticalMirror) {
            component.mirroredVertically = !component.mirroredVertically;
        } else {
            component.mirrored = !component.mirrored;
        }
        const QString simulationStatus = evaluateCircuit();
        const bool mirrorEnabled = verticalMirror
                                       ? component.mirroredVertically
                                       : component.mirrored;
        QString message = QString("Mirrored component %1: %2 (%3)")
                              .arg(verticalMirror ? "vertically" : "horizontally")
                              .arg(componentDisplayName(component.component.name()))
                              .arg(mirrorEnabled ? "ON" : "OFF");
        if (!simulationStatus.isEmpty()) {
            message += " - " + simulationStatus;
        }
        emit actionOccurred(message);
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

    const bool directlyInteractive = component.component.name() == "VoltageSource"
                                     || component.component.name() == "Switch";
    if (currentSimulationState != SimulationState::Running && !directlyInteractive) {
        editSelectedComponentProperties();
        event->accept();
        return;
    }

    QString actionMessage;

    const QString typeName = component.component.name();
    if (typeName == "Switch") {
        component.stateOn = !component.stateOn;
        actionMessage = QString("Switch toggled %1").arg(component.stateOn ? "CLOSED" : "OPEN");
    } else if (typeName == "VoltageSource") {
        component.stateOn = !component.stateOn;
        actionMessage = QString("Digital Voltage Source set to %1").arg(component.stateOn ? 1 : 0);
    } else {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QString simulationStatus = evaluateCircuit();
    if (!simulationStatus.isEmpty()) {
        actionMessage += " - " + simulationStatus;
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
        if (isWiringMode) {
            selectedComponentIndex = -1;
            selectedWireIndex = -1;

            QString componentId;
            QString pinName;
            QPoint pinPosition;
            const bool pinFound = findNearestPin(worldPoint, &componentId, &pinName, &pinPosition);

            if (!hasWireStartPoint) {
                if (!pinFound) {
                    emit actionOccurred("Invalid wire start: click on or near a component pin");
                    update();
                    event->accept();
                    return;
                }

                wireStartComponentId = componentId;
                wireStartPinName = pinName;
                wireStartPoint = pinPosition;
                previewWireEndPoint = pinPosition;
                hasWireStartPoint = true;
                emit actionOccurred(QString("Wire started at %1")
                                        .arg(pinDisplayName(componentId, pinName)));
            } else {
                if (!pinFound) {
                    emit actionOccurred("Invalid wire endpoint: click on or near a component pin");
                    update();
                    event->accept();
                    return;
                }
                if (componentId == wireStartComponentId && pinName == wireStartPinName) {
                    emit actionOccurred("Invalid wire endpoint: a pin cannot be connected to itself");
                    update();
                    event->accept();
                    return;
                }
                if (isDuplicateConnection(wireStartComponentId, wireStartPinName,
                                          componentId, pinName)) {
                    emit actionOccurred("Duplicate connection rejected");
                    update();
                    event->accept();
                    return;
                }

                const Pin *startPin = findPin(wireStartComponentId, wireStartPinName);
                const Pin *endPin = findPin(componentId, pinName);
                if (startPin == nullptr || endPin == nullptr
                    || !pinDirectionsAreCompatible(*startPin, *endPin)) {
                    emit actionOccurred("Invalid pin direction: Input-to-Input and Output-to-Output connections are not allowed");
                    update();
                    event->accept();
                    return;
                }
                if ((startPin->type() == PinType::Input
                     && pinHasConnection(wireStartComponentId, wireStartPinName))
                    || (endPin->type() == PinType::Input
                        && pinHasConnection(componentId, pinName))) {
                    emit actionOccurred("Input pin already has a connection; a second independent connection is not allowed");
                    update();
                    event->accept();
                    return;
                }

                const QString startDisplayName = pinDisplayName(wireStartComponentId, wireStartPinName);
                const QString endDisplayName = pinDisplayName(componentId, pinName);
                placedWires.append(Wire(createWireId(),
                                        wireStartComponentId,
                                        wireStartPinName,
                                        componentId,
                                        pinName));
                selectedWireIndex = placedWires.size() - 1;
                hasWireStartPoint = false;
                wireStartComponentId.clear();
                wireStartPinName.clear();
                QString message = QString("Connected %1 to %2")
                                      .arg(startDisplayName, endDisplayName);
                const QString simulationStatus = evaluateCircuit();
                if (!simulationStatus.isEmpty()) {
                    message += " - " + simulationStatus;
                }
                emit actionOccurred(message);
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
            placeComponent(activeComponentType, worldPoint);
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
    painter.fillRect(rect(), QColor(226, 232, 240));

    painter.translate(panOffset);
    painter.scale(zoomFactor, zoomFactor);

    const QRectF documentRect(0.0,
                              0.0,
                              documentCanvasSize.width(),
                              documentCanvasSize.height());
    painter.fillRect(documentRect, Qt::white);
    QPen pageBorderPen(QColor(148, 163, 184), 1.2);
    pageBorderPen.setCosmetic(true);
    painter.setPen(pageBorderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(documentRect);

    QPen gridPen(QColor(225, 225, 225));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    const QPointF topLeft = screenToWorld(QPoint(0, 0));
    const QPointF bottomRight = screenToWorld(QPoint(width(), height()));

    const int firstX = std::max(0, static_cast<int>(std::floor(topLeft.x() / GridSpacing)) * GridSpacing);
    const int lastX = std::min(documentCanvasSize.width(),
                               static_cast<int>(std::ceil(bottomRight.x() / GridSpacing)) * GridSpacing);
    const int firstY = std::max(0, static_cast<int>(std::floor(topLeft.y() / GridSpacing)) * GridSpacing);
    const int lastY = std::min(documentCanvasSize.height(),
                               static_cast<int>(std::ceil(bottomRight.y() / GridSpacing)) * GridSpacing);

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
        drawWirePath(painter,
                     orthogonalWirePath(wireStartPoint, previewWireEndPoint),
                     LogicState::Undefined,
                     true);
    }

    for (int i = 0; i < placedComponents.size(); ++i) {
        const PlacedComponent &component = placedComponents[i];

        painter.save();
        painter.translate(component.component.position());
        painter.rotate(component.rotationDegrees);
        if (component.mirrored || component.mirroredVertically) {
            painter.scale(component.mirrored ? -1.0 : 1.0,
                          component.mirroredVertically ? -1.0 : 1.0);
        }
        drawComponent(painter, component);
        drawComponentPins(painter, component);
        painter.restore();

        drawComponentLabel(painter, component);
        drawComponentStateText(painter, component);

        if (i == selectedComponentIndex) {
            drawSelectedComponentBounds(painter, componentBounds(component.component.position()));
        }
    }

    if (!hoveredPinComponentId.isEmpty() && !hoveredPinName.isEmpty()) {
        painter.save();
        QPen hoverPen(QColor(245, 158, 11), 2.5);
        hoverPen.setCosmetic(true);
        painter.setPen(hoverPen);
        painter.setBrush(QColor(254, 243, 199, 210));
        painter.drawEllipse(QPointF(hoveredPinPosition), 8.0, 8.0);

        QFont hoverFont = painter.font();
        hoverFont.setBold(true);
        hoverFont.setPointSize(8);
        painter.setFont(hoverFont);
        painter.setPen(QColor(146, 64, 14));
        painter.drawText(QRectF(hoveredPinPosition.x() - 55,
                                hoveredPinPosition.y() - 28,
                                110,
                                16),
                         Qt::AlignCenter,
                         pinDisplayName(hoveredPinComponentId, hoveredPinName));
        painter.restore();
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
        placedComponents[selectedComponentIndex].component.setPosition(
            boundedComponentPosition(worldPoint));
        update();
    }

    if (isWiringMode && hasWireStartPoint) {
        const QPoint worldPoint = screenToWorld(event->pos()).toPoint();
        previewWireEndPoint = snapToGrid(worldPoint);
        update();
    }

    const QPoint hoverWorldPoint = screenToWorld(event->pos()).toPoint();
    QString hoverComponentId;
    QString hoverPinName;
    QPoint hoverPosition;
    const bool pinFound = findNearestPin(hoverWorldPoint,
                                         &hoverComponentId,
                                         &hoverPinName,
                                         &hoverPosition);
    const bool hoverChanged = hoveredPinComponentId != hoverComponentId
                              || hoveredPinName != hoverPinName
                              || (pinFound && hoveredPinPosition != hoverPosition);
    if (pinFound) {
        hoveredPinComponentId = hoverComponentId;
        hoveredPinName = hoverPinName;
        hoveredPinPosition = hoverPosition;
    } else {
        hoveredPinComponentId.clear();
        hoveredPinName.clear();
    }
    if (hoverChanged) {
        emit pinHoverChanged(pinFound
                                 ? pinDisplayName(hoveredPinComponentId, hoveredPinName)
                                 : QString());
        update();
    }

    emitMousePosition(event->pos());
}

void CircuitCanvas::leaveEvent(QEvent *event)
{
    if (!hoveredPinComponentId.isEmpty() || !hoveredPinName.isEmpty()) {
        hoveredPinComponentId.clear();
        hoveredPinName.clear();
        emit pinHoverChanged(QString());
        update();
    }
    QWidget::leaveEvent(event);
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
        QPoint startPoint;
        QPoint endPoint;
        if (!wireEndpoints(placedWires[i], &startPoint, &endPoint)) {
            continue;
        }
        const QVector<QPoint> path = orthogonalWirePath(startPoint, endPoint);

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

    if (typeName == "AndGate" || typeName == "OrGate"
        || typeName == "NandGate" || typeName == "XorGate") {
        return {
            Pin("A", PinType::Input, QPoint(-58, -16)),
            Pin("B", PinType::Input, QPoint(-58, 16)),
            Pin("OUT", PinType::Output, QPoint(58, 0))
        };
    }
    if (typeName == "NotGate") {
        return {
            Pin("IN", PinType::Input, QPoint(-58, 0)),
            Pin("OUT", PinType::Output, QPoint(58, 0))
        };
    }
    if (typeName == "VoltageSource") {
        return {pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::Low)};
    }
    if (typeName == "Battery") {
        return {pinWithState("OUT", PinType::Output, QPoint(58, 0), LogicState::High)};
    }
    if (typeName == "Switch") {
        return {
            Pin("IN", PinType::Input, QPoint(-58, 0)),
            Pin("OUT", PinType::Output, QPoint(58, 0))
        };
    }
    if (typeName == "Led") {
        return {Pin("IN", PinType::Input, QPoint(-58, 0))};
    }
    if (typeName == "Ground") {
        return {pinWithState("GND", PinType::Bidirectional, QPoint(0, -38), LogicState::Low)};
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
    QPoint offset = pin.offset();
    if (component.mirrored) {
        offset.setX(-offset.x());
    }
    if (component.mirroredVertically) {
        offset.setY(-offset.y());
    }
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

const CircuitCanvas::PlacedComponent *CircuitCanvas::findComponent(const QString &componentId) const
{
    for (const PlacedComponent &component : placedComponents) {
        if (component.component.id() == componentId) {
            return &component;
        }
    }
    return nullptr;
}

CircuitCanvas::PlacedComponent *CircuitCanvas::findComponent(const QString &componentId)
{
    for (PlacedComponent &component : placedComponents) {
        if (component.component.id() == componentId) {
            return &component;
        }
    }
    return nullptr;
}

const Pin *CircuitCanvas::findPin(const QString &componentId, const QString &pinName) const
{
    const PlacedComponent *component = findComponent(componentId);
    return component == nullptr ? nullptr : component->component.findPin(pinName);
}

Pin *CircuitCanvas::findPin(const QString &componentId, const QString &pinName)
{
    PlacedComponent *component = findComponent(componentId);
    return component == nullptr ? nullptr : component->component.findPin(pinName);
}

bool CircuitCanvas::wireEndpoints(const Wire &wire, QPoint *startPoint, QPoint *endPoint) const
{
    const PlacedComponent *startComponent = findComponent(wire.startComponentId());
    const PlacedComponent *endComponent = findComponent(wire.endComponentId());
    if (startComponent == nullptr || endComponent == nullptr) {
        return false;
    }

    const Pin *startPin = startComponent->component.findPin(wire.startPinName());
    const Pin *endPin = endComponent->component.findPin(wire.endPinName());
    if (startPin == nullptr || endPin == nullptr) {
        return false;
    }

    if (startPoint != nullptr) {
        *startPoint = pinWorldPosition(*startComponent, *startPin);
    }
    if (endPoint != nullptr) {
        *endPoint = pinWorldPosition(*endComponent, *endPin);
    }
    return true;
}

bool CircuitCanvas::isDuplicateConnection(const QString &startComponentId,
                                          const QString &startPinName,
                                          const QString &endComponentId,
                                          const QString &endPinName) const
{
    for (const Wire &wire : placedWires) {
        const bool sameDirection = wire.startComponentId() == startComponentId
                                   && wire.startPinName() == startPinName
                                   && wire.endComponentId() == endComponentId
                                   && wire.endPinName() == endPinName;
        const bool reverseDirection = wire.startComponentId() == endComponentId
                                      && wire.startPinName() == endPinName
                                      && wire.endComponentId() == startComponentId
                                      && wire.endPinName() == startPinName;
        if (sameDirection || reverseDirection) {
            return true;
        }
    }
    return false;
}

bool CircuitCanvas::pinDirectionsAreCompatible(const Pin &startPin, const Pin &endPin) const
{
    return !((startPin.type() == PinType::Input && endPin.type() == PinType::Input)
             || (startPin.type() == PinType::Output && endPin.type() == PinType::Output));
}

bool CircuitCanvas::pinHasConnection(const QString &componentId, const QString &pinName) const
{
    for (const Wire &wire : placedWires) {
        if ((wire.startComponentId() == componentId && wire.startPinName() == pinName)
            || (wire.endComponentId() == componentId && wire.endPinName() == pinName)) {
            return true;
        }
    }
    return false;
}

QString CircuitCanvas::pinDisplayName(const QString &componentId, const QString &pinName) const
{
    const PlacedComponent *component = findComponent(componentId);
    return QString("%1.%2").arg(component == nullptr ? componentId : component->label, pinName);
}

QString CircuitCanvas::createComponentId()
{
    QString candidate;
    do {
        candidate = QString("component-%1").arg(nextComponentId++);
    } while (findComponent(candidate) != nullptr);
    return candidate;
}

QString CircuitCanvas::createWireId()
{
    return QString("wire-%1").arg(nextWireId++);
}

QString CircuitCanvas::componentDisplayName(const QString &typeName) const
{
    if (typeName == "Led") {
        return "LED";
    }
    if (typeName == "VoltageSource") {
        return "Digital Voltage Source";
    }
    if (typeName == "Battery") {
        return "Digital Battery";
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
    if (typeName == "NandGate") {
        return "NAND Gate";
    }
    if (typeName == "XorGate") {
        return "XOR Gate";
    }

    return typeName;
}

QString CircuitCanvas::defaultComponentValue(const QString &typeName) const
{
    if (typeName == "Resistor") {
        return "1 kOhm";
    }
    if (typeName == "Capacitor") {
        return "1 uF";
    }
    if (typeName == "Inductor") {
        return "1 mH";
    }
    if (typeName == "Diode") {
        return "Generic";
    }
    if (typeName == "Led") {
        return "Red";
    }
    if (typeName == "VoltageSource") {
        return "5 V";
    }
    if (typeName == "Battery") {
        return "9 V";
    }
    return QString();
}

QString CircuitCanvas::createComponentLabel(const QString &typeName)
{
    const QString prefix = labelPrefix(typeName);
    int nextNumber = labelCounters.value(prefix, 0) + 1;
    auto labelExists = [this](const QString &candidate) {
        return std::any_of(placedComponents.cbegin(), placedComponents.cend(),
                           [&candidate](const PlacedComponent &component) {
                               return component.label.compare(candidate, Qt::CaseInsensitive) == 0;
                           });
    };
    while (labelExists(QString("%1%2").arg(prefix).arg(nextNumber))) {
        ++nextNumber;
    }
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
    if (typeName == "Battery") {
        return "BAT";
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
    if (typeName == "NandGate") {
        return "NAND";
    }
    if (typeName == "XorGate") {
        return "XOR";
    }

    return "U";
}

void CircuitCanvas::buildNodes()
{
    nodes.clear();
    for (PlacedComponent &component : placedComponents) {
        for (const Pin &pin : component.component.pins()) {
            component.component.findPin(pin.name())->setConnectedNodeId(QString());
        }
    }

    QVector<QString> pinReferences;
    QHash<QString, int> referenceIndices;
    QVector<int> parents;

    auto referenceFor = [](const QString &componentId, const QString &pinName) {
        return componentId + ":" + pinName;
    };
    auto registerReference = [&](const QString &reference) {
        const auto existing = referenceIndices.constFind(reference);
        if (existing != referenceIndices.cend()) {
            return existing.value();
        }
        const int index = pinReferences.size();
        pinReferences.append(reference);
        referenceIndices.insert(reference, index);
        parents.append(index);
        return index;
    };
    auto findRoot = [&](int index) {
        int root = index;
        while (parents[root] != root) {
            root = parents[root];
        }
        while (parents[index] != index) {
            const int next = parents[index];
            parents[index] = root;
            index = next;
        }
        return root;
    };

    for (const Wire &wire : placedWires) {
        const int startIndex = registerReference(referenceFor(wire.startComponentId(), wire.startPinName()));
        const int endIndex = registerReference(referenceFor(wire.endComponentId(), wire.endPinName()));
        const int startRoot = findRoot(startIndex);
        const int endRoot = findRoot(endIndex);
        if (startRoot != endRoot) {
            parents[endRoot] = startRoot;
        }
    }

    QHash<int, int> nodeIndexByRoot;
    for (int referenceIndex = 0; referenceIndex < pinReferences.size(); ++referenceIndex) {
        const int root = findRoot(referenceIndex);
        if (!nodeIndexByRoot.contains(root)) {
            nodeIndexByRoot.insert(root, nodes.size());
            nodes.append(Node(QString("node-%1").arg(nodes.size() + 1)));
        }
        nodes[nodeIndexByRoot.value(root)].addConnectedPinRef(pinReferences[referenceIndex]);
    }

    for (const Wire &wire : placedWires) {
        const QString startReference = referenceFor(wire.startComponentId(), wire.startPinName());
        const int root = findRoot(referenceIndices.value(startReference));
        nodes[nodeIndexByRoot.value(root)].addWireId(wire.id());
    }

    for (PlacedComponent &component : placedComponents) {
        for (const Pin &pin : component.component.pins()) {
            const QString reference = referenceFor(component.component.id(), pin.name());
            const auto referenceIndex = referenceIndices.constFind(reference);
            if (referenceIndex == referenceIndices.cend()) {
                continue;
            }
            const int root = findRoot(referenceIndex.value());
            component.component.findPin(pin.name())->setConnectedNodeId(nodes[nodeIndexByRoot.value(root)].id());
        }
    }
}

QString CircuitCanvas::evaluateCircuit()
{
    constexpr int MaximumIterations = 100;
    buildNodes();

    auto setPinState = [](Pin *pin, LogicState state) {
        if (pin == nullptr || pin->state() == state) {
            return false;
        }
        pin->setState(state);
        return true;
    };

    for (PlacedComponent &component : placedComponents) {
        const QString typeName = component.component.name();
        for (const Pin &pin : component.component.pins()) {
            LogicState initialState = LogicState::Undefined;
            if (typeName == "VoltageSource" && pin.name() == "OUT") {
                initialState = component.stateOn ? LogicState::High : LogicState::Low;
            } else if (typeName == "Battery" && pin.name() == "OUT") {
                initialState = LogicState::High;
            } else if (typeName == "Ground" && pin.name() == "GND") {
                initialState = LogicState::Low;
            }
            component.component.findPin(pin.name())->setState(initialState);
        }
        if (typeName == "Led") {
            component.stateOn = false;
        }
    }

    bool stabilized = false;
    QStringList conflictingNodeIds;

    for (int iteration = 0; iteration < MaximumIterations; ++iteration) {
        bool changed = false;
        conflictingNodeIds.clear();

        for (Node &node : nodes) {
            bool hasHighDriver = false;
            bool hasLowDriver = false;

            for (const PlacedComponent &component : placedComponents) {
                const QString typeName = component.component.name();
                for (const Pin &pin : component.component.pins()) {
                    if (pin.connectedNodeId() != node.id()) {
                        continue;
                    }
                    const bool isDriver = pin.type() == PinType::Output
                                          || (typeName == "Ground" && pin.name() == "GND");
                    if (!isDriver) {
                        continue;
                    }
                    hasHighDriver = hasHighDriver || pin.state() == LogicState::High;
                    hasLowDriver = hasLowDriver || pin.state() == LogicState::Low;
                }
            }

            LogicState nodeState = LogicState::Undefined;
            if (hasHighDriver && hasLowDriver) {
                conflictingNodeIds.append(node.id());
            } else if (hasHighDriver) {
                nodeState = LogicState::High;
            } else if (hasLowDriver) {
                nodeState = LogicState::Low;
            }
            if (node.state() != nodeState) {
                node.setState(nodeState);
                changed = true;
            }
        }

        for (PlacedComponent &component : placedComponents) {
            const QString typeName = component.component.name();
            for (const Pin &pin : component.component.pins()) {
                if (pin.connectedNodeId().isEmpty()) {
                    continue;
                }
                if (pin.type() == PinType::Output
                    || (typeName == "Ground" && pin.name() == "GND")) {
                    continue;
                }
                LogicState nodeState = LogicState::Undefined;
                for (const Node &node : nodes) {
                    if (node.id() == pin.connectedNodeId()) {
                        nodeState = node.state();
                        break;
                    }
                }
                changed = setPinState(component.component.findPin(pin.name()), nodeState) || changed;
            }

            if (typeName == "VoltageSource") {
                changed = setPinState(component.component.findPin("OUT"),
                                      component.stateOn ? LogicState::High : LogicState::Low) || changed;
            } else if (typeName == "Battery") {
                changed = setPinState(component.component.findPin("OUT"), LogicState::High) || changed;
            } else if (typeName == "Ground") {
                changed = setPinState(component.component.findPin("GND"), LogicState::Low) || changed;
            } else if (typeName == "Switch") {
                const Pin *input = component.component.findPin("IN");
                changed = setPinState(component.component.findPin("OUT"),
                                      component.stateOn && input != nullptr
                                          ? input->state()
                                          : LogicState::Undefined) || changed;
            } else if (typeName == "AndGate" || typeName == "OrGate"
                       || typeName == "NandGate" || typeName == "XorGate") {
                const LogicState inputA = component.component.findPin("A")->state();
                const LogicState inputB = component.component.findPin("B")->state();
                LogicState output = LogicState::Undefined;
                if (typeName == "AndGate") {
                    if (inputA == LogicState::Low || inputB == LogicState::Low) {
                        output = LogicState::Low;
                    } else if (inputA == LogicState::High && inputB == LogicState::High) {
                        output = LogicState::High;
                    }
                } else if (typeName == "OrGate") {
                    if (inputA == LogicState::High || inputB == LogicState::High) {
                        output = LogicState::High;
                    } else if (inputA == LogicState::Low && inputB == LogicState::Low) {
                        output = LogicState::Low;
                    }
                } else if (typeName == "NandGate") {
                    if (inputA == LogicState::Low || inputB == LogicState::Low) {
                        output = LogicState::High;
                    } else if (inputA == LogicState::High && inputB == LogicState::High) {
                        output = LogicState::Low;
                    }
                } else if (inputA != LogicState::Undefined
                           && inputB != LogicState::Undefined) {
                    output = inputA == inputB ? LogicState::Low : LogicState::High;
                }
                changed = setPinState(component.component.findPin("OUT"), output) || changed;
            } else if (typeName == "NotGate") {
                const LogicState input = component.component.findPin("IN")->state();
                const LogicState output = input == LogicState::Low
                                              ? LogicState::High
                                              : (input == LogicState::High
                                                     ? LogicState::Low
                                                     : LogicState::Undefined);
                changed = setPinState(component.component.findPin("OUT"), output) || changed;
            } else if (typeName == "Led") {
                const Pin *input = component.component.findPin("IN");
                component.stateOn = input != nullptr && input->state() == LogicState::High;
            }
        }

        if (!changed) {
            stabilized = true;
            break;
        }
    }

    for (Wire &wire : placedWires) {
        wire.setState(LogicState::Undefined);
        const Pin *startPin = findPin(wire.startComponentId(), wire.startPinName());
        if (startPin == nullptr) {
            continue;
        }
        for (const Node &node : nodes) {
            if (node.id() == startPin->connectedNodeId()) {
                wire.setState(node.state());
                break;
            }
        }
    }

    QStringList statusParts;
    if (!conflictingNodeIds.isEmpty()) {
        statusParts.append(QString("Signal conflict on %1: High and Low output drivers; node state is Undefined")
                               .arg(conflictingNodeIds.join(", ")));
    }
    if (!stabilized) {
        statusParts.append(QString("Warning: circuit did not stabilize after %1 iterations")
                               .arg(MaximumIterations));
    }

    QStringList floatingInputs;
    for (const PlacedComponent &component : placedComponents) {
        for (const Pin &pin : component.component.pins()) {
            if (pin.type() == PinType::Input && pin.state() == LogicState::Undefined) {
                floatingInputs.append(QString("%1.%2").arg(component.label, pin.name()));
            }
        }
    }
    if (!floatingInputs.isEmpty()) {
        statusParts.append(QString("Floating input(s): %1")
                               .arg(floatingInputs.join(", ")));
    }
    return statusParts.join("; ");
}

void CircuitCanvas::performSimulationStep()
{
    lastSimulationStatus = evaluateCircuit();
    update();
}

void CircuitCanvas::resetSimulationRuntime()
{
    nodes.clear();

    for (PlacedComponent &component : placedComponents) {
        component.stateOn = false;
        const QString typeName = component.component.name();

        for (const Pin &pin : component.component.pins()) {
            Pin *runtimePin = component.component.findPin(pin.name());
            if (runtimePin == nullptr) {
                continue;
            }

            LogicState initialState = LogicState::Undefined;
            if ((typeName == "VoltageSource" && pin.name() == "OUT")
                || (typeName == "Ground" && pin.name() == "GND")) {
                initialState = LogicState::Low;
            } else if (typeName == "Battery" && pin.name() == "OUT") {
                initialState = LogicState::High;
            }
            runtimePin->setState(initialState);
            runtimePin->setConnectedNodeId(QString());
        }
    }

    for (Wire &wire : placedWires) {
        wire.setState(LogicState::Undefined);
    }
}

void CircuitCanvas::setSimulationState(SimulationState state)
{
    if (currentSimulationState == state) {
        return;
    }

    currentSimulationState = state;
    emit simulationStateChanged(currentSimulationState);
}

QString CircuitCanvas::logicStateText(LogicState state) const
{
    if (state == LogicState::Low) {
        return "0";
    }
    if (state == LogicState::High) {
        return "1";
    }
    return "U";
}

QColor CircuitCanvas::logicStateColor(LogicState state) const
{
    if (state == LogicState::Low) {
        return QColor(45, 95, 190);
    }
    if (state == LogicState::High) {
        return QColor(0, 145, 70);
    }
    return QColor(125, 125, 125);
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
    } else if (typeName == "Battery") {
        drawBattery(painter);
    } else if (typeName == "AndGate") {
        drawAndGate(painter);
    } else if (typeName == "OrGate") {
        drawOrGate(painter);
    } else if (typeName == "NotGate") {
        drawNotGate(painter);
    } else if (typeName == "NandGate") {
        drawNandGate(painter);
    } else if (typeName == "XorGate") {
        drawXorGate(painter);
    }
}

void CircuitCanvas::drawComponentPins(QPainter &painter, const PlacedComponent &component) const
{
    painter.save();
    for (const Pin &pin : component.component.pins()) {
        const bool floatingInput = pin.type() == PinType::Input
                                   && pin.state() == LogicState::Undefined;
        const QColor color = floatingInput
                                 ? QColor(245, 158, 11)
                                 : logicStateColor(pin.state());
        QPen pinPen(color.darker(125), 2);
        pinPen.setCosmetic(true);
        painter.setPen(pinPen);
        painter.setBrush(color);
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
        stateText = QString("%1 IN=%2 OUT=%3")
                        .arg(component.stateOn ? "CLOSED" : "OPEN",
                             logicStateText(component.component.findPin("IN")->state()),
                             logicStateText(component.component.findPin("OUT")->state()));
    } else if (typeName == "VoltageSource") {
        stateText = QString("OUT=%1")
                        .arg(logicStateText(component.component.findPin("OUT")->state()));
    } else if (typeName == "Battery") {
        stateText = QString("OUT=%1")
                        .arg(logicStateText(component.component.findPin("OUT")->state()));
    } else if (typeName == "Led") {
        stateText = QString("IN=%1 %2")
                        .arg(logicStateText(component.component.findPin("IN")->state()),
                             component.stateOn ? "ON" : "OFF");
    } else if (typeName == "AndGate" || typeName == "OrGate"
               || typeName == "NandGate" || typeName == "XorGate") {
        stateText = QString("A=%1 B=%2 OUT=%3")
                        .arg(logicStateText(component.component.findPin("A")->state()),
                             logicStateText(component.component.findPin("B")->state()),
                             logicStateText(component.component.findPin("OUT")->state()));
    } else if (typeName == "NotGate") {
        stateText = QString("IN=%1 OUT=%2")
                        .arg(logicStateText(component.component.findPin("IN")->state()),
                             logicStateText(component.component.findPin("OUT")->state()));
    } else if (typeName == "Ground") {
        stateText = QString("GND=%1")
                        .arg(logicStateText(component.component.findPin("GND")->state()));
    } else if (!component.value.isEmpty()) {
        stateText = component.value;
    } else {
        return;
    }

    if (!component.value.isEmpty() && stateText != component.value) {
        stateText += " | " + component.value;
    }

    painter.save();

    const Pin *outputPin = component.component.findPin("OUT");
    const bool highState = (component.stateOn && typeName == "Led")
                           || (outputPin != nullptr && outputPin->state() == LogicState::High);
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

void CircuitCanvas::drawWire(QPainter &painter, const Wire &wire, bool selected) const
{
    QPoint startPoint;
    QPoint endPoint;
    if (wireEndpoints(wire, &startPoint, &endPoint)) {
        drawWirePath(painter, orthogonalWirePath(startPoint, endPoint), wire.state(), selected);
    }
}

void CircuitCanvas::drawWirePath(QPainter &painter,
                                 const QVector<QPoint> &path,
                                 LogicState state,
                                 bool selected) const
{
    if (path.size() < 2) {
        return;
    }

    QPen wirePen(logicStateColor(state), selected ? 4 : 2);
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
    painter.drawText(QRectF(-10, 24, 20, 16), Qt::AlignCenter, QString::number(value));
}

void CircuitCanvas::drawBattery(QPainter &painter) const
{
    painter.drawLine(-58, 0, -22, 0);
    painter.drawLine(-22, -24, -22, 24);
    painter.drawLine(2, -14, 2, 14);
    painter.drawLine(2, 0, 58, 0);
    painter.drawText(QRectF(-45, -34, 18, 18), Qt::AlignCenter, "+");
    painter.drawText(QRectF(9, -29, 18, 18), Qt::AlignCenter, "-");
}

void CircuitCanvas::drawAndGate(QPainter &painter) const
{
    painter.drawLine(-58, -16, -28, -16);
    painter.drawLine(-58, 16, -28, 16);
    painter.drawLine(30, 0, 58, 0);

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

void CircuitCanvas::drawNandGate(QPainter &painter) const
{
    painter.drawLine(-58, -16, -28, -16);
    painter.drawLine(-58, 16, -28, 16);
    painter.drawLine(36, 0, 58, 0);

    QPainterPath path;
    path.moveTo(-28, -30);
    path.lineTo(0, -30);
    path.arcTo(QRectF(-30, -30, 60, 60), 90, -180);
    path.lineTo(-28, 30);
    path.closeSubpath();
    painter.drawPath(path);
    painter.drawEllipse(QPointF(33, 0), 4, 4);
}

void CircuitCanvas::drawXorGate(QPainter &painter) const
{
    painter.drawLine(-58, -16, -30, -16);
    painter.drawLine(-58, 16, -30, 16);
    painter.drawLine(26, 0, 58, 0);

    QPainterPath gatePath;
    gatePath.moveTo(-34, -30);
    gatePath.quadTo(-12, 0, -34, 30);
    gatePath.quadTo(2, 26, 28, 0);
    gatePath.quadTo(2, -26, -34, -30);
    gatePath.closeSubpath();
    painter.drawPath(gatePath);

    QPainterPath exclusiveCurve;
    exclusiveCurve.moveTo(-43, -30);
    exclusiveCurve.quadTo(-21, 0, -43, 30);
    painter.drawPath(exclusiveCurve);
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
