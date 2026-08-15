#include "canvasview.h"

#include "circuitcomponent.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMimeData>

CanvasView::CanvasView(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    scene()->setSceneRect(-2000, -2000, 4000, 4000);

    setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setFocusPolicy(Qt::StrongFocus);
}

void CanvasView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        return;
    }

    QGraphicsView::dragEnterEvent(event);
}

void CanvasView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        return;
    }

    QGraphicsView::dragMoveEvent(event);
}

void CanvasView::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasText()) {
        QGraphicsView::dropEvent(event);
        return;
    }

    const QString text = event->mimeData()->text().trimmed();
    CircuitComponent::Type type;

    if (text == "Resistor") {
        type = CircuitComponent::Type::Resistor;
    } else if (text == "Capacitor") {
        type = CircuitComponent::Type::Capacitor;
    } else if (text == "Inductor") {
        type = CircuitComponent::Type::Inductor;
    } else if (text == "AndGate") {
        type = CircuitComponent::Type::AndGate;
    } else if (text == "OrGate") {
        type = CircuitComponent::Type::OrGate;
    } else if (text == "NotGate") {
        type = CircuitComponent::Type::NotGate;
    } else if (text == "Ground") {
        type = CircuitComponent::Type::Ground;
    } else if (text == "VoltageSource") {
        type = CircuitComponent::Type::VoltageSource;
    } else if (text == "Switch") {
        type = CircuitComponent::Type::Switch;
    } else if (text == "Led") {
        type = CircuitComponent::Type::Led;
    } else if (text == "Diode") {
        type = CircuitComponent::Type::Diode;
    } else {
        event->ignore();
        return;
    }

    CircuitComponent *component = new CircuitComponent(type);
    component->setPos(mapToScene(event->position().toPoint()));
    scene()->addItem(component);

    event->acceptProposedAction();
    setFocus();
}

void CanvasView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        const QList<QGraphicsItem *> items = scene()->selectedItems();
        for (QGraphicsItem *item : items) {
            delete item;
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_R) {
        const QList<QGraphicsItem *> items = scene()->selectedItems();
        for (QGraphicsItem *item : items) {
            item->setRotation(item->rotation() + 90.0);
        }
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}
