#ifndef COMPONENTLISTWIDGET_H
#define COMPONENTLISTWIDGET_H

#include <QListWidget>
#include <QPoint>

class ComponentListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit ComponentListWidget(QWidget *parent = nullptr);

signals:
    void componentTypeSelected(const QString &typeName);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void addComponentItem(const QString &label, const QString &typeName);

    QPoint m_dragStartPos;
};

#endif
