#include "circuitcanvas.h"
#include "componentlistwidget.h"
#include "mainwindow.h"
#include "newprojectdialog.h"
#include "projectfile.h"
#include "startpage.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
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
    , stepButton(nullptr)
    , propertiesButton(nullptr)
    , simulationStatusLabel(nullptr)
    , projectLogLabel(nullptr)
    , currentCanvasWidth(0)
    , currentCanvasHeight(0)
    , currentInfiniteCanvas(false)
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
    stepButton = new QPushButton("Step", simulationGroupBox);
    simulationStatusLabel = new QLabel(simulationGroupBox);

    runButton->setObjectName("simulationRunButton");
    pauseButton->setObjectName("simulationPauseButton");
    stopButton->setObjectName("simulationStopButton");
    resetButton->setObjectName("simulationResetButton");
    stepButton->setObjectName("simulationStepButton");
    simulationStatusLabel->setObjectName("simulationStatusLabel");
    simulationStatusLabel->setAlignment(Qt::AlignCenter);
    simulationStatusLabel->setMinimumWidth(130);

    simulationLayout->addWidget(runButton);
    simulationLayout->addWidget(pauseButton);
    simulationLayout->addWidget(stopButton);
    simulationLayout->addWidget(resetButton);
    simulationLayout->addWidget(stepButton);
    simulationLayout->addStretch();
    simulationLayout->addWidget(simulationStatusLabel);

    auto *workspaceLayout = new QHBoxLayout();

    auto *componentsGroupBox = new QGroupBox("Components", editorPage);
    componentsGroupBox->setMinimumWidth(180);
    componentsGroupBox->setMaximumWidth(240);
    auto *componentsLayout = new QVBoxLayout(componentsGroupBox);
    auto *componentList = new ComponentListWidget(componentsGroupBox);
    componentsLayout->addWidget(componentList);
    propertiesButton = new QPushButton("Properties...", componentsGroupBox);
    propertiesButton->setObjectName("componentPropertiesButton");
    propertiesButton->setToolTip("Edit the selected component label and interactive state (P)");
    componentsLayout->addWidget(propertiesButton);

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
    connect(propertiesButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::editSelectedComponentProperties);

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
    connect(stepButton, &QPushButton::clicked,
            circuitCanvas, &CircuitCanvas::stepSimulation);
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
            this, &MainWindow::openProject);
    connect(startPage, &StartPage::exitRequested,
            qApp, &QApplication::quit);
    connect(startPage, &StartPage::recentProjectSelected,
            this, &MainWindow::openProjectFile);
    refreshRecentProjects();
}

void MainWindow::createMenuActions()
{
    auto *fileMenu = menuBar()->addMenu("File");

    auto *newProjectAction = fileMenu->addAction("New Project");
    auto *openProjectAction = fileMenu->addAction("Open Project");
    auto *saveProjectAction = fileMenu->addAction("Save Project");
    auto *saveProjectAsAction = fileMenu->addAction("Save Project As...");
    auto *exportCanvasAction = fileMenu->addAction("Export Canvas as PNG...");
    auto *backToStartAction = fileMenu->addAction("Back to Start Page");
    fileMenu->addSeparator();
    auto *exitAction = fileMenu->addAction("Exit");

    connect(newProjectAction, &QAction::triggered, this, &MainWindow::createNewProject);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(saveProjectAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(exportCanvasAction, &QAction::triggered, this, &MainWindow::exportCanvasImage);
    connect(backToStartAction, &QAction::triggered, this, &MainWindow::showStartPage);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::createNewProject()
{
    NewProjectDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    currentProjectFilePath.clear();
    showEditorPage(dialog.projectName(),
                   dialog.canvasWidth(),
                   dialog.canvasHeight(),
                   dialog.isInfiniteCanvas());
}

void MainWindow::openProject()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, "Open ProteusLite Project", QString(),
        "ProteusLite projects (*.json);;All files (*.*)");
    if (fileName.isEmpty()) {
        return;
    }

    openProjectFile(fileName);
}

bool MainWindow::openProjectFile(const QString &fileName)
{
    if (!QFileInfo::exists(fileName)) {
        QSettings settings("ProteusLite", "ProteusLite");
        QStringList paths = recentProjectPaths();
        paths.removeAll(QDir::cleanPath(QFileInfo(fileName).absoluteFilePath()));
        settings.setValue("recentProjects", paths);
        refreshRecentProjects();
        QMessageBox::warning(this, "Open Project", "The selected recent project no longer exists.");
        return false;
    }

    ProjectFileData project;
    QString errorMessage;
    if (!ProjectFile::load(fileName, &project, &errorMessage)) {
        QMessageBox::critical(this, "Open Project", errorMessage);
        return false;
    }
    if (!circuitCanvas->loadProjectData(project, &errorMessage)) {
        QMessageBox::critical(this, "Open Project", errorMessage);
        return false;
    }

    showEditorPage(project.projectName,
                   project.canvasSize.width(),
                   project.canvasSize.height(),
                   project.infiniteCanvas);
    currentProjectFilePath = fileName;
    addRecentProject(fileName);
    if (projectLogLabel != nullptr) {
        projectLogLabel->setText(QString("Project opened: %1").arg(fileName));
    }
    return true;
}

void MainWindow::saveProject()
{
    if (pageStack->currentWidget() != editorPage) {
        QMessageBox::information(this, "Save Project", "Create or open a project first.");
        return;
    }
    if (currentProjectFilePath.isEmpty()) {
        saveProjectAs();
        return;
    }
    writeProjectFile(currentProjectFilePath);
}

void MainWindow::saveProjectAs()
{
    if (pageStack->currentWidget() != editorPage) {
        QMessageBox::information(this, "Save Project", "Create or open a project first.");
        return;
    }

    QString suggestedName = currentProjectName.trimmed();
    if (suggestedName.isEmpty()) {
        suggestedName = "Untitled Project";
    }
    suggestedName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save ProteusLite Project", suggestedName + ".json",
        "ProteusLite projects (*.json);;All files (*.*)");
    if (fileName.isEmpty()) {
        return;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += ".json";
    }
    if (writeProjectFile(fileName)) {
        currentProjectFilePath = fileName;
    }
}

void MainWindow::exportCanvasImage()
{
    if (pageStack->currentWidget() != editorPage) {
        QMessageBox::information(this, "Export Canvas", "Create or open a project first.");
        return;
    }

    QString suggestedName = currentProjectName.trimmed();
    if (suggestedName.isEmpty()) {
        suggestedName = "ProteusLite Circuit";
    }
    suggestedName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Circuit Canvas", suggestedName + ".png",
        "PNG images (*.png)");
    if (fileName.isEmpty()) {
        return;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += ".png";
    }

    QString errorMessage;
    if (!circuitCanvas->exportToPng(fileName, &errorMessage)) {
        QMessageBox::critical(this, "Export Canvas", errorMessage);
        return;
    }

    if (projectLogLabel != nullptr) {
        projectLogLabel->setText(QString("Canvas exported to PNG: %1").arg(fileName));
    }
}

bool MainWindow::writeProjectFile(const QString &fileName)
{
    const ProjectFileData project = circuitCanvas->projectData(
        currentProjectName, QSize(currentCanvasWidth, currentCanvasHeight));
    QString errorMessage;
    if (!ProjectFile::save(fileName, project, &errorMessage)) {
        QMessageBox::critical(this, "Save Project", errorMessage);
        return false;
    }
    currentProjectFilePath = fileName;
    addRecentProject(fileName);
    if (projectLogLabel != nullptr) {
        projectLogLabel->setText(QString("Project saved: %1").arg(fileName));
    }
    return true;
}

void MainWindow::showEditorPage(const QString &projectName,
                                int canvasWidth,
                                int canvasHeight,
                                bool infiniteCanvas)
{
    currentProjectName = projectName;
    currentCanvasWidth = canvasWidth;
    currentCanvasHeight = canvasHeight;
    currentInfiniteCanvas = infiniteCanvas;
    circuitCanvas->setDocumentCanvasSize(
        QSize(currentCanvasWidth, currentCanvasHeight), currentInfiniteCanvas);

    setWindowTitle(QString("ProteusLite - %1").arg(projectName));

    if (projectLogLabel != nullptr) {
        projectLogLabel->setText(
            currentInfiniteCanvas
                ? QString("New project created: %1 (Infinite Canvas)").arg(projectName)
                : QString("New project created: %1 (%2 x %3)")
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
    refreshRecentProjects();
    pageStack->setCurrentWidget(startPage);
}

QStringList MainWindow::recentProjectPaths() const
{
    QSettings settings("ProteusLite", "ProteusLite");
    const QStringList storedPaths = settings.value("recentProjects").toStringList();
    QStringList validPaths;
    for (const QString &path : storedPaths) {
        const QString absolutePath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
        if (QFileInfo::exists(absolutePath) && !validPaths.contains(absolutePath)) {
            validPaths.append(absolutePath);
        }
        if (validPaths.size() == 5) {
            break;
        }
    }
    return validPaths;
}

void MainWindow::addRecentProject(const QString &fileName)
{
    const QString absolutePath = QDir::cleanPath(QFileInfo(fileName).absoluteFilePath());
    QStringList paths = recentProjectPaths();
    paths.removeAll(absolutePath);
    paths.prepend(absolutePath);
    while (paths.size() > 5) {
        paths.removeLast();
    }

    QSettings settings("ProteusLite", "ProteusLite");
    settings.setValue("recentProjects", paths);
    refreshRecentProjects();
}

void MainWindow::refreshRecentProjects()
{
    if (startPage != nullptr) {
        startPage->setRecentProjects(recentProjectPaths());
    }
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
    stepButton->setEnabled(!isRunning);
    stepButton->setToolTip(isPaused
                               ? "Evaluate exactly one simulation step while paused"
                               : "Evaluate exactly one simulation step without starting the timer");

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
