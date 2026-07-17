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
    };

    QPointF screenToWorld(const QPoint &screenPoint) const;
    void emitMousePosition(const QPoint &screenPoint);
    int componentAt(const QPoint &worldPoint) const;
    QRectF componentBounds(const QPoint &position) const;
    QString componentDisplayName(const QString &typeName) const;
    QString createComponentLabel(const QString &typeName);
    QString labelPrefix(const QString &typeName) const;
    void drawComponent(QPainter &painter, const QString &typeName) const;
    void drawComponentLabel(QPainter &painter, const PlacedComponent &component) const;
    void drawSelectedComponentBounds(QPainter &painter, const QRectF &bounds) const;
    void drawResistor(QPainter &painter) const;
    void drawCapacitor(QPainter &painter) const;
    void drawInductor(QPainter &painter) const;
    void drawDiode(QPainter &painter, bool led) const;
    void drawSwitch(QPainter &painter) const;
    void drawGround(QPainter &painter) const;
    void drawVoltageSource(QPainter &painter) const;
    void drawAndGate(QPainter &painter) const;
    void drawOrGate(QPainter &painter) const;
    void drawNotGate(QPainter &painter) const;
    void resetView();
    void setZoom(double newZoom, const QPoint &anchorPoint);

    double zoomFactor;
    QPointF panOffset;
    bool isPanning;
    bool isDraggingComponent;
    QPoint lastPanPoint;
    QString activeComponentType;
    QVector<PlacedComponent> placedComponents;
    QHash<QString, int> labelCounters;
    int selectedComponentIndex;
};

#endif // CIRCUITCANVAS_H
