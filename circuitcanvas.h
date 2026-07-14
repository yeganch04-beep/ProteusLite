#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H

#include <QPoint>
#include <QWidget>

class CircuitCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit CircuitCanvas(QWidget *parent = nullptr);

    QPoint snapToGrid(const QPoint &point) const;

signals:
    void mousePositionChanged(const QPoint &position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    static constexpr int GridSpacing = 20;
};

#endif // CIRCUITCANVAS_H
