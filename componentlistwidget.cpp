#include "componentlistwidget.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

ComponentListWidget::ComponentListWidget(QWidget *parent)
    : QListWidget(parent)
{
    addComponentItem("Resistor", "Resistor");
    addComponentItem("Capacitor", "Capacitor");
    addComponentItem("Inductor", "Inductor");
    addComponentItem("Diode", "Diode");
    addComponentItem("LED", "Led");
    addComponentItem("Switch", "Switch");
    addComponentItem("Ground", "Ground");
    addComponentItem("Digital Voltage Source", "VoltageSource");
    addComponentItem("AND Gate", "AndGate");
    addComponentItem("OR Gate", "OrGate");
    addComponentItem("NOT Gate", "NotGate");
}

void ComponentListWidget::addComponentItem(const QString &label, const QString &typeName)
{
    auto *item = new QListWidgetItem(label, this);
    item->setData(Qt::UserRole, typeName);
}

void ComponentListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
    }

    QListWidget::mousePressEvent(event);
}

void ComponentListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    QListWidgetItem *item = currentItem();
    if (!item) {
        return;
    }

    const QString typeName = item->data(Qt::UserRole).toString();
    if (typeName.isEmpty()) {
        return;
    }

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setText(typeName);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}
