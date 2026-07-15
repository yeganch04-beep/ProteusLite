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

    auto *zoomLabel = new QLabel("Zoom: 100%", this);
    ui->logLayout->addWidget(zoomLabel);

    connect(canvas, &CircuitCanvas::mousePositionChanged, this,
            [coordinatesLabel](const QPoint &position) {
                coordinatesLabel->setText(
                    QString("X: %1, Y: %2").arg(position.x()).arg(position.y()));
            });

    connect(canvas, &CircuitCanvas::zoomChanged, this,
            [zoomLabel](int percentage) {
                zoomLabel->setText(QString("Zoom: %1%").arg(percentage));
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}
