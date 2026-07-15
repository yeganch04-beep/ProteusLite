#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H

#include <QPoint>
#include <QPointF>
#include <QWidget>

class CircuitCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit CircuitCanvas(QWidget *parent = nullptr);

    QPoint snapToGrid(const QPoint &point) const;

signals:
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

    QPointF screenToWorld(const QPoint &screenPoint) const;
    void emitMousePosition(const QPoint &screenPoint);
    void resetView();
    void setZoom(double newZoom, const QPoint &anchorPoint);

    double zoomFactor;
    QPointF panOffset;
    bool isPanning;
    QPoint lastPanPoint;
};

#endif // CIRCUITCANVAS_H
