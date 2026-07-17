#include "mainwindow.h"
#include "canvasview.h"
#include "componentlistwidget.h"
#include <QDockWidget>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Proteus-like Circuit Builder");

    m_canvasView = new CanvasView(this);
    setCentralWidget(m_canvasView);

    m_compSidebar = new ComponentListWidget(this);
    m_dock = new QDockWidget("Components Library", this);
    m_dock->setWidget(m_compSidebar);

    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    statusBar()->showMessage("System ready.");
}
