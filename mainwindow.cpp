#include "circuitcanvas.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto *canvas = new CircuitCanvas(this);
    ui->canvasLayout->addWidget(canvas);

    auto *coordinatesLabel = new QLabel("X: 0, Y: 0", this);
    ui->logLayout->addWidget(coordinatesLabel);

    connect(canvas, &CircuitCanvas::mousePositionChanged, this,
            [coordinatesLabel](const QPoint &position) {
                coordinatesLabel->setText(
                    QString("X: %1, Y: %2").arg(position.x()).arg(position.y()));
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}
