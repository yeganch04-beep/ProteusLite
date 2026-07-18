#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H

#include <QPoint>
#include <QPointF>
#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

class QPainter;

class CircuitCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit CircuitCanvas(QWidget *parent = nullptr);

    QPoint snapToGrid(const QPoint &point) const;
    void setActiveComponentType(const QString &typeName);

signals:
    void actionOccurred(const QString &message);
    void mousePositionChanged(const QPoint &position);
    void zoomChanged(int percentage);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    static constexpr int GridSpacing = 20;
    static constexpr double MinimumZoom = 0.5;
    static constexpr double MaximumZoom = 2.0;

    struct PlacedComponent
    {
        QString typeName;
        QString label;
        QPoint position;
        int rotationDegrees = 0;
        bool stateOn = false;
        bool inputA = false;
        bool inputB = false;
        int outputValue = 0;
    };

    struct PlacedWire
    {
        QPoint startPoint;
        QPoint endPoint;
    };

    QPointF screenToWorld(const QPoint &screenPoint) const;
    void emitMousePosition(const QPoint &screenPoint);
    int componentAt(const QPoint &worldPoint) const;
    int wireAt(const QPoint &worldPoint) const;
    QVector<QPoint> orthogonalWirePath(const QPoint &startPoint, const QPoint &endPoint) const;
    double distanceToSegment(const QPoint &point, const QPoint &startPoint, const QPoint &endPoint) const;
    QRectF componentBounds(const QPoint &position) const;
    QString componentDisplayName(const QString &typeName) const;
    QString createComponentLabel(const QString &typeName);
    QString labelPrefix(const QString &typeName) const;
    bool isLogicGate(const QString &typeName) const;
    void evaluateLogicGates();
    void drawComponent(QPainter &painter, const PlacedComponent &component) const;
    void drawComponentLabel(QPainter &painter, const PlacedComponent &component) const;
    void drawComponentStateText(QPainter &painter, const PlacedComponent &component) const;
    void drawSelectedComponentBounds(QPainter &painter, const QRectF &bounds) const;
    void drawWire(QPainter &painter, const PlacedWire &wire, bool selected) const;
    void drawWirePath(QPainter &painter, const QVector<QPoint> &path, bool selected) const;
    void drawResistor(QPainter &painter) const;
    void drawCapacitor(QPainter &painter) const;
    void drawInductor(QPainter &painter) const;
    void drawDiode(QPainter &painter, bool led, bool ledOn) const;
    void drawSwitch(QPainter &painter, bool closed) const;
    void drawGround(QPainter &painter) const;
    void drawVoltageSource(QPainter &painter, int value) const;
    void drawAndGate(QPainter &painter) const;
    void drawOrGate(QPainter &painter) const;
    void drawNotGate(QPainter &painter) const;
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
    QString activeComponentType;
    QVector<PlacedComponent> placedComponents;
    QVector<PlacedWire> placedWires;
    QHash<QString, int> labelCounters;
    int selectedComponentIndex;
    int selectedWireIndex;
};

#endif // CIRCUITCANVAS_H
