#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H

#include "component.h"
#include "node.h"
#include "projectfile.h"
#include "wire.h"

#include <QPoint>
#include <QPointF>
#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

class QColor;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QTimer;
class QWheelEvent;

class CircuitCanvas : public QWidget
{
    Q_OBJECT

public:
    enum class SimulationState
    {
        Stopped,
        Running,
        Paused
    };
    Q_ENUM(SimulationState)

    explicit CircuitCanvas(QWidget *parent = nullptr);
    ~CircuitCanvas() override;

    QPoint snapToGrid(const QPoint &point) const;
    void setActiveComponentType(const QString &typeName);
    void setDocumentCanvasSize(const QSize &size);
    SimulationState simulationState() const;
    ProjectFileData projectData(const QString &projectName,
                                const QSize &canvasSize) const;
    bool loadProjectData(const ProjectFileData &project,
                         QString *errorMessage = nullptr);
    bool exportToPng(const QString &fileName,
                     QString *errorMessage = nullptr);

public slots:
    void runSimulation();
    void pauseSimulation();
    void stopSimulation();
    void resetSimulation();
    void stepSimulation();
    void editSelectedComponentProperties();

signals:
    void actionOccurred(const QString &message);
    void mousePositionChanged(const QPoint &position);
    void pinHoverChanged(const QString &pinDisplayName);
    void simulationStateChanged(CircuitCanvas::SimulationState state);
    void zoomChanged(int percentage);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    static constexpr int GridSpacing = 20;
    static constexpr int SimulationIntervalMs = 100;
    static constexpr double MinimumZoom = 0.5;
    static constexpr double MaximumZoom = 2.0;

    struct PlacedComponent
    {
        PlacedComponent(const Component &componentModel,
                        const QString &displayLabel,
                        const QString &propertyValue)
            : component(componentModel)
            , label(displayLabel)
            , value(propertyValue)
        {
        }

        Component component;
        QString label;
        QString value;
        int rotationDegrees = 0;
        bool mirrored = false;
        bool mirroredVertically = false;
        bool stateOn = false;
    };

    bool placeComponent(const QString &typeName, const QPoint &worldPosition);
    QPoint boundedComponentPosition(const QPoint &worldPosition) const;
    QPointF screenToWorld(const QPoint &screenPoint) const;
    void emitMousePosition(const QPoint &screenPoint);
    int componentAt(const QPoint &worldPoint) const;
    int wireAt(const QPoint &worldPoint) const;
    QVector<QPoint> orthogonalWirePath(const QPoint &startPoint, const QPoint &endPoint) const;
    double distanceToSegment(const QPoint &point, const QPoint &startPoint, const QPoint &endPoint) const;
    QRectF componentBounds(const QPoint &position) const;
    QVector<Pin> createPinsForComponent(const QString &typeName) const;
    QPoint pinWorldPosition(const PlacedComponent &component, const Pin &pin) const;
    bool findNearestPin(const QPoint &worldPoint,
                        QString *componentId,
                        QString *pinName,
                        QPoint *pinPosition = nullptr) const;
    const PlacedComponent *findComponent(const QString &componentId) const;
    PlacedComponent *findComponent(const QString &componentId);
    const Pin *findPin(const QString &componentId, const QString &pinName) const;
    Pin *findPin(const QString &componentId, const QString &pinName);
    bool wireEndpoints(const Wire &wire, QPoint *startPoint, QPoint *endPoint) const;
    bool isDuplicateConnection(const QString &startComponentId,
                               const QString &startPinName,
                               const QString &endComponentId,
                               const QString &endPinName) const;
    bool pinDirectionsAreCompatible(const Pin &startPin, const Pin &endPin) const;
    bool pinHasConnection(const QString &componentId, const QString &pinName) const;
    QString pinDisplayName(const QString &componentId, const QString &pinName) const;
    QString createComponentId();
    QString createWireId();
    QString componentDisplayName(const QString &typeName) const;
    QString defaultComponentValue(const QString &typeName) const;
    QString createComponentLabel(const QString &typeName);
    QString labelPrefix(const QString &typeName) const;
    void buildNodes();
    QString evaluateCircuit();
    void performSimulationStep();
    void resetSimulationRuntime();
    void setSimulationState(SimulationState state);
    QString logicStateText(LogicState state) const;
    QColor logicStateColor(LogicState state) const;
    void drawComponent(QPainter &painter, const PlacedComponent &component) const;
    void drawComponentPins(QPainter &painter, const PlacedComponent &component) const;
    void drawComponentLabel(QPainter &painter, const PlacedComponent &component) const;
    void drawComponentStateText(QPainter &painter, const PlacedComponent &component) const;
    void drawSelectedComponentBounds(QPainter &painter, const QRectF &bounds) const;
    void drawWire(QPainter &painter, const Wire &wire, bool selected) const;
    void drawWirePath(QPainter &painter,
                      const QVector<QPoint> &path,
                      LogicState state,
                      bool selected) const;
    void drawResistor(QPainter &painter) const;
    void drawCapacitor(QPainter &painter) const;
    void drawInductor(QPainter &painter) const;
    void drawDiode(QPainter &painter, bool led, bool ledOn) const;
    void drawSwitch(QPainter &painter, bool closed) const;
    void drawGround(QPainter &painter) const;
    void drawVoltageSource(QPainter &painter, int value) const;
    void drawBattery(QPainter &painter) const;
    void drawAndGate(QPainter &painter) const;
    void drawOrGate(QPainter &painter) const;
    void drawNotGate(QPainter &painter) const;
    void drawNandGate(QPainter &painter) const;
    void drawXorGate(QPainter &painter) const;
    void resetView();
    void setZoom(double newZoom, const QPoint &anchorPoint);

    double zoomFactor;
    QPointF panOffset;
    bool isPanning;
    bool isDraggingComponent;
    bool isWiringMode;
    bool hasWireStartPoint;
    QPoint lastPanPoint;
    QPoint wireStartPoint;
    QPoint previewWireEndPoint;
    QString wireStartComponentId;
    QString wireStartPinName;
    QString hoveredPinComponentId;
    QString hoveredPinName;
    QPoint hoveredPinPosition;
    QSize documentCanvasSize;
    QString activeComponentType;
    QVector<PlacedComponent> placedComponents;
    QVector<Wire> placedWires;
    QVector<Node> nodes;
    QHash<QString, int> labelCounters;
    QTimer *simulationTimer;
    SimulationState currentSimulationState;
    QString lastSimulationStatus;
    quint64 nextComponentId;
    quint64 nextWireId;
    int selectedComponentIndex;
    int selectedWireIndex;
};

#endif // CIRCUITCANVAS_H
