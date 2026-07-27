#include "circuitcanvas.h"
#include "componentlistwidget.h"
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
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pageStack(new QStackedWidget(this))
    , startPage(nullptr)
    , editorPage(nullptr)
    , circuitCanvas(nullptr)
    , runButton(nullptr)
    , pauseButton(nullptr)
    , stopButton(nullptr)
    , resetButton(nullptr)
    , simulationStatusLabel(nullptr)
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

    auto *simulationGroupBox = new QGroupBox("Simulation", editorPage);
    auto *simulationLayout = new QHBoxLayout(simulationGroupBox);
    runButton = new QPushButton("Run", simulationGroupBox);
    pauseButton = new QPushButton("Pause", simulationGroupBox);
    stopButton = new QPushButton("Stop", simulationGroupBox);
    resetButton = new QPushButton("Reset", simulationGroupBox);
    simulationStatusLabel = new QLabel(simulationGroupBox);

    runButton->setObjectName("simulationRunButton");
    pauseButton->setObjectName("simulationPauseButton");
    stopButton->setObjectName("simulationStopButton");
    resetButton->setObjectName("simulationResetButton");
    simulationStatusLabel->setObjectName("simulationStatusLabel");
    simulationStatusLabel->setAlignment(Qt::AlignCenter);
    simulationStatusLabel->setMinimumWidth(130);

    simulationLayout->addWidget(runButton);
    simulationLayout->addWidget(pauseButton);
    simulationLayout->addWidget(stopButton);
    simulationLayout->addWidget(resetButton);
    simulationLayout->addStretch();
    simulationLayout->addWidget(simulationStatusLabel);

    auto *workspaceLayout = new QHBoxLayout();

    auto *componentsGroupBox = new QGroupBox("Components", editorPage);
    componentsGroupBox->setMinimumWidth(180);
    componentsGroupBox->setMaximumWidth(240);
    auto *componentsLayout = new QVBoxLayout(componentsGroupBox);
    auto *componentList = new ComponentListWidget(componentsGroupBox);
    componentsLayout->addWidget(componentList);

    auto *canvasGroupBox = new QGroupBox("Circuit Canvas", editorPage);
    auto *canvasLayout = new QVBoxLayout(canvasGroupBox);

    workspaceLayout->addWidget(componentsGroupBox);
    workspaceLayout->addWidget(canvasGroupBox);

    auto *logGroupBox = new QGroupBox("Log", editorPage);
    logGroupBox->setMinimumHeight(120);
    logGroupBox->setMaximumHeight(160);
    auto *logLayout = new QVBoxLayout(logGroupBox);

    mainLayout->addWidget(simulationGroupBox);
    mainLayout->addLayout(workspaceLayout);
    mainLayout->addWidget(logGroupBox);

    pageStack->addWidget(startPage);
    pageStack->addWidget(editorPage);
    setCentralWidget(pageStack);

    circuitCanvas = new CircuitCanvas(this);
    canvasLayout->addWidget(circuitCanvas);

    projectLogLabel = new QLabel("No project created yet.", this);
    logLayout->addWidget(projectLogLabel);

    auto *coordinatesLabel = new QLabel("X: 0, Y: 0", this);
    logLayout->addWidget(coordinatesLabel);

    auto *zoomLabel = new QLabel("Zoom: 100%", this);
    logLayout->addWidget(zoomLabel);

    connect(circuitCanvas, &CircuitCanvas::mousePositionChanged, this,
            [coordinatesLabel](const QPoint &position) {
                coordinatesLabel->setText(
                    QString("X: %1, Y: %2").arg(position.x()).arg(position.y()));
            });

    connect(circuitCanvas, &CircuitCanvas::zoomChanged, this,
            [zoomLabel](int percentage) {
                zoomLabel->setText(QString("Zoom: %1%").arg(percentage));
            });

    connect(componentList, &ComponentListWidget::componentTypeSelected,
            circuitCanvas, &CircuitCanvas::setActiveComponentType);

    connect(circuitCanvas, &CircuitCanvas::actionOccurred, this,
            [this](const QString &message) {
                if (projectLogLabel != nullptr) {
                    projectLogLabel->setText(message);
                }
            });

    connect(runButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::runSimulation);
    connect(pauseButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::pauseSimulation);
    connect(stopButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::stopSimulation);
    connect(resetButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::resetSimulation);
    connect(circuitCanvas, &CircuitCanvas::simulationStateChanged, this,
            [this](CircuitCanvas::SimulationState) {
                updateSimulationControls();
            });

    updateSimulationControls();

    createMenuActions();
    showStartPage();
}

MainWindow::~MainWindow()
{
    if (circuitCanvas != nullptr) {
        circuitCanvas->stopSimulation();
    }
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
    if (circuitCanvas != nullptr) {
        circuitCanvas->stopSimulation();
    }
    setWindowTitle("ProteusLite");
    pageStack->setCurrentWidget(startPage);
}

void MainWindow::updateSimulationControls()
{
    if (circuitCanvas == nullptr) {
        return;
    }

    const CircuitCanvas::SimulationState state = circuitCanvas->simulationState();
    const bool isStopped = state == CircuitCanvas::SimulationState::Stopped;
    const bool isRunning = state == CircuitCanvas::SimulationState::Running;
    const bool isPaused = state == CircuitCanvas::SimulationState::Paused;

    runButton->setEnabled(!isRunning);
    runButton->setText("Run");
    runButton->setToolTip(isPaused ? "Resume simulation" : "Start simulation");
    pauseButton->setEnabled(isRunning);
    stopButton->setEnabled(!isStopped);
    resetButton->setEnabled(true);

    if (isRunning) {
        simulationStatusLabel->setText("Running");
        simulationStatusLabel->setStyleSheet(
            "QLabel { color: #145a32; background: #d5f5e3; border: 1px solid #58d68d; "
            "border-radius: 4px; padding: 4px 10px; font-weight: bold; }");
    } else if (isPaused) {
        simulationStatusLabel->setText("Paused");
        simulationStatusLabel->setStyleSheet(
            "QLabel { color: #7d6608; background: #fcf3cf; border: 1px solid #f4d03f; "
            "border-radius: 4px; padding: 4px 10px; font-weight: bold; }");
    } else {
        simulationStatusLabel->setText("Stopped");
        simulationStatusLabel->setStyleSheet(
            "QLabel { color: #424949; background: #e5e7e9; border: 1px solid #aab7b8; "
            "border-radius: 4px; padding: 4px 10px; font-weight: bold; }");
    }
}
