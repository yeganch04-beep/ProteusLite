#include "circuitcanvas.h"
#include "mainwindow.h"
#include "newprojectdialog.h"
#include "startpage.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pageStack(new QStackedWidget(this))
    , startPage(nullptr)
    , editorPage(nullptr)
    , projectLogLabel(nullptr)
    , currentCanvasWidth(0)
    , currentCanvasHeight(0)
{
    ui->setupUi(this);

    QWidget *oldCentralWidget = takeCentralWidget();
    delete oldCentralWidget;

    createStartPage();

    editorPage = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(editorPage);
    auto *workspaceLayout = new QHBoxLayout();

    auto *componentsGroupBox = new QGroupBox("Components", editorPage);
    componentsGroupBox->setMinimumWidth(180);
    componentsGroupBox->setMaximumWidth(240);
    componentsGroupBox->setLayout(new QVBoxLayout());

    auto *canvasGroupBox = new QGroupBox("Circuit Canvas", editorPage);
    auto *canvasLayout = new QVBoxLayout(canvasGroupBox);

    workspaceLayout->addWidget(componentsGroupBox);
    workspaceLayout->addWidget(canvasGroupBox);

    auto *logGroupBox = new QGroupBox("Log", editorPage);
    logGroupBox->setMinimumHeight(120);
    logGroupBox->setMaximumHeight(160);
    auto *logLayout = new QVBoxLayout(logGroupBox);

    mainLayout->addLayout(workspaceLayout);
    mainLayout->addWidget(logGroupBox);

    pageStack->addWidget(startPage);
    pageStack->addWidget(editorPage);
    setCentralWidget(pageStack);

    auto *canvas = new CircuitCanvas(this);
    canvasLayout->addWidget(canvas);

    projectLogLabel = new QLabel("No project created yet.", this);
    logLayout->addWidget(projectLogLabel);

    auto *coordinatesLabel = new QLabel("X: 0, Y: 0", this);
    logLayout->addWidget(coordinatesLabel);

    auto *zoomLabel = new QLabel("Zoom: 100%", this);
    logLayout->addWidget(zoomLabel);

    connect(canvas, &CircuitCanvas::mousePositionChanged, this,
            [coordinatesLabel](const QPoint &position) {
                coordinatesLabel->setText(
                    QString("X: %1, Y: %2").arg(position.x()).arg(position.y()));
            });

    connect(canvas, &CircuitCanvas::zoomChanged, this,
            [zoomLabel](int percentage) {
                zoomLabel->setText(QString("Zoom: %1%").arg(percentage));
            });

    createMenuActions();
    showStartPage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createStartPage()
{
    startPage = new StartPage(this);

    connect(startPage, &StartPage::newProjectRequested,
            this, &MainWindow::createNewProject);
    connect(startPage, &StartPage::openProjectRequested,
            this, &MainWindow::openProjectPlaceholder);
    connect(startPage, &StartPage::exitRequested,
            qApp, &QApplication::quit);
    connect(startPage, &StartPage::recentProjectSelected, this,
            [this](const QString &projectName) {
                QMessageBox::information(this, "Recent Project",
                                         projectName + " is a placeholder recent project.");
            });
}

void MainWindow::createMenuActions()
{
    auto *fileMenu = menuBar()->addMenu("File");

    auto *newProjectAction = fileMenu->addAction("New Project");
    auto *openProjectAction = fileMenu->addAction("Open Project");
    auto *backToStartAction = fileMenu->addAction("Back to Start Page");
    fileMenu->addSeparator();
    auto *exitAction = fileMenu->addAction("Exit");

    connect(newProjectAction, &QAction::triggered, this, &MainWindow::createNewProject);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProjectPlaceholder);
    connect(backToStartAction, &QAction::triggered, this, &MainWindow::showStartPage);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::createNewProject()
{
    NewProjectDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    showEditorPage(dialog.projectName(), dialog.canvasWidth(), dialog.canvasHeight());
}

void MainWindow::openProjectPlaceholder()
{
    QMessageBox::information(this, "Open Project",
                             "Open Project will be implemented later.");
}

void MainWindow::showEditorPage(const QString &projectName, int canvasWidth, int canvasHeight)
{
    currentCanvasWidth = canvasWidth;
    currentCanvasHeight = canvasHeight;

    setWindowTitle(QString("ProteusLite - %1").arg(projectName));

    if (projectLogLabel != nullptr) {
        projectLogLabel->setText(QString("New project created: %1 (%2 x %3)")
                                     .arg(projectName)
                                     .arg(currentCanvasWidth)
                                     .arg(currentCanvasHeight));
    }

    pageStack->setCurrentWidget(editorPage);
}

void MainWindow::showStartPage()
{
    setWindowTitle("ProteusLite");
    pageStack->setCurrentWidget(startPage);
}
