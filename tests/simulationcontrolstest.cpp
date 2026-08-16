#include "circuitcanvas.h"
#include "mainwindow.h"
#include "startpage.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class SimulationControlsTest : public QObject
{
    Q_OBJECT

private slots:
    void engineStateTransitions();
    void buttonStateRules();
    void manualStepDoesNotStartTimer();
    void pinHoverHighlightsNearestPin();
    void canvasDimensionsConstrainPlacement();
    void propertiesEditPersistsLabelAndState();
    void recentProjectsUseRealFilePaths();
    void resetPreservesCircuitAndRestoresInitialValues();
    void projectDataRoundTripPreservesComponents();
    void floatingInputsAreReported();
    void batteryDrivesLedHigh();
    void nandAndXorTruthTables();
    void mirroringPersistsAndCanvasExportsToPng();
};

void SimulationControlsTest::engineStateTransitions()
{
    CircuitCanvas canvas;
    const QList<QTimer *> timers = canvas.findChildren<QTimer *>(
        QString(), Qt::FindDirectChildrenOnly);

    QCOMPARE(timers.size(), 1);
    QTimer *const timer = timers.constFirst();
    QVERIFY(!timer->isActive());
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);

    canvas.runSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Running);
    QVERIFY(timer->isActive());
    const int firstTimerId = timer->timerId();

    canvas.runSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Running);
    QCOMPARE(canvas.findChildren<QTimer *>(QString(), Qt::FindDirectChildrenOnly).size(), 1);
    QCOMPARE(timer->timerId(), firstTimerId);

    canvas.pauseSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Paused);
    QVERIFY(!timer->isActive());

    canvas.runSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Running);
    QVERIFY(timer->isActive());

    canvas.stopSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);
    QVERIFY(!timer->isActive());

    canvas.runSimulation();
    canvas.pauseSimulation();
    canvas.stopSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);
    QVERIFY(!timer->isActive());

    canvas.runSimulation();
    canvas.resetSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);
    QVERIFY(!timer->isActive());

    canvas.runSimulation();
    canvas.pauseSimulation();
    canvas.resetSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);
    QVERIFY(!timer->isActive());
}

void SimulationControlsTest::buttonStateRules()
{
    MainWindow window;
    auto *const canvas = window.findChild<CircuitCanvas *>();
    auto *const runButton = window.findChild<QPushButton *>("simulationRunButton");
    auto *const pauseButton = window.findChild<QPushButton *>("simulationPauseButton");
    auto *const stopButton = window.findChild<QPushButton *>("simulationStopButton");
    auto *const resetButton = window.findChild<QPushButton *>("simulationResetButton");
    auto *const stepButton = window.findChild<QPushButton *>("simulationStepButton");
    auto *const propertiesButton = window.findChild<QPushButton *>("componentPropertiesButton");
    auto *const statusLabel = window.findChild<QLabel *>("simulationStatusLabel");

    QVERIFY(canvas != nullptr);
    QVERIFY(runButton != nullptr);
    QVERIFY(pauseButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(resetButton != nullptr);
    QVERIFY(stepButton != nullptr);
    QVERIFY(propertiesButton != nullptr);
    QVERIFY(statusLabel != nullptr);

    QVERIFY(runButton->isEnabled());
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QVERIFY(stepButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Stopped"));

    runButton->click();
    QVERIFY(!runButton->isEnabled());
    QVERIFY(pauseButton->isEnabled());
    QVERIFY(stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QVERIFY(!stepButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Running"));

    pauseButton->click();
    QVERIFY(runButton->isEnabled());
    QCOMPARE(runButton->text(), QString("Run"));
    QCOMPARE(runButton->toolTip(), QString("Resume simulation"));
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QVERIFY(stepButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Paused"));

    stopButton->click();
    QVERIFY(runButton->isEnabled());
    QCOMPARE(runButton->text(), QString("Run"));
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QVERIFY(stepButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Stopped"));
}

void SimulationControlsTest::manualStepDoesNotStartTimer()
{
    CircuitCanvas canvas;
    const QList<QTimer *> timers = canvas.findChildren<QTimer *>(
        QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(timers.size(), 1);

    QSignalSpy actionSpy(&canvas, &CircuitCanvas::actionOccurred);
    canvas.stepSimulation();

    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);
    QVERIFY(!timers.constFirst()->isActive());
    QCOMPARE(actionSpy.count(), 1);
    QCOMPARE(actionSpy.constFirst().constFirst().toString(),
             QString("Simulation advanced by one step"));

    canvas.runSimulation();
    canvas.stepSimulation();
    QCOMPARE(actionSpy.constLast().constFirst().toString(),
             QString("Pause the simulation before using Step"));
}

void SimulationControlsTest::pinHoverHighlightsNearestPin()
{
    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setActiveComponentType("VoltageSource");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(240, 200));

    QSignalSpy hoverSpy(&canvas, &CircuitCanvas::pinHoverChanged);
    QTest::mouseMove(&canvas, QPoint(298, 200));
    QVERIFY(!hoverSpy.isEmpty());
    QCOMPARE(hoverSpy.constLast().constFirst().toString(), QString("VDC1.OUT"));

    QTest::mouseMove(&canvas, QPoint(20, 20));
    QCOMPARE(hoverSpy.constLast().constFirst().toString(), QString());
}

void SimulationControlsTest::canvasDimensionsConstrainPlacement()
{
    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setDocumentCanvasSize(QSize(400, 300));
    canvas.setActiveComponentType("VoltageSource");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));

    const ProjectFileData project = canvas.projectData("Bounded", QSize(400, 300));
    QCOMPARE(project.components.size(), 1);
    QCOMPARE(project.components.constFirst().position, QPoint(80, 60));
}

void SimulationControlsTest::propertiesEditPersistsLabelAndState()
{
    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setActiveComponentType("VoltageSource");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(160, 200));
    canvas.setActiveComponentType("Led");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(360, 200));

    QTest::keyClick(&canvas, Qt::Key_W);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(218, 200));
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(302, 200));
    QTest::keyClick(&canvas, Qt::Key_W);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(160, 200));

    bool dialogHandled = false;
    QTimer::singleShot(0, &canvas, [&dialogHandled]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (dialog == nullptr) {
            return;
        }
        auto *idEdit = dialog->findChild<QLineEdit *>("propertyIdEdit");
        auto *labelEdit = dialog->findChild<QLineEdit *>("propertyLabelEdit");
        auto *valueEdit = dialog->findChild<QLineEdit *>("propertyValueEdit");
        auto *stateCombo = dialog->findChild<QComboBox *>("propertyStateCombo");
        auto *buttons = dialog->findChild<QDialogButtonBox *>();
        if (idEdit == nullptr || labelEdit == nullptr || valueEdit == nullptr
            || stateCombo == nullptr || buttons == nullptr) {
            dialog->reject();
            return;
        }
        idEdit->setText("input-source-a");
        labelEdit->setText("INPUT_A");
        valueEdit->setText("3.3 V");
        stateCombo->setCurrentIndex(1);
        buttons->button(QDialogButtonBox::Ok)->click();
        dialogHandled = true;
    });

    canvas.editSelectedComponentProperties();
    QVERIFY(dialogHandled);

    const ProjectFileData project = canvas.projectData("Properties", QSize(800, 600));
    QCOMPARE(project.components.size(), 2);
    QCOMPARE(project.wires.size(), 1);
    QCOMPARE(project.components.constFirst().id, QString("input-source-a"));
    QCOMPARE(project.components.constFirst().label, QString("INPUT_A"));
    QCOMPARE(project.components.constFirst().value, QString("3.3 V"));
    QVERIFY(project.components.constFirst().stateOn);
    QCOMPARE(project.wires.constFirst().startComponentId, QString("input-source-a"));
}

void SimulationControlsTest::recentProjectsUseRealFilePaths()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstPath = directory.filePath("First Demo.json");
    const QString secondPath = directory.filePath("Second Demo.json");
    QFile firstFile(firstPath);
    QFile secondFile(secondPath);
    QVERIFY(firstFile.open(QIODevice::WriteOnly));
    QVERIFY(secondFile.open(QIODevice::WriteOnly));
    firstFile.close();
    secondFile.close();

    StartPage page;
    page.setRecentProjects({firstPath, secondPath});
    page.resize(600, 400);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    auto *const list = page.findChild<QListWidget *>("recentProjectsList");
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);
    QCOMPARE(list->item(0)->text(), QString("First Demo"));
    QCOMPARE(list->item(0)->data(Qt::UserRole).toString(), firstPath);
    QCOMPARE(list->item(1)->data(Qt::UserRole).toString(), secondPath);

    QSignalSpy selectionSpy(&page, &StartPage::recentProjectSelected);
    const QRect itemRect = list->visualItemRect(list->item(0));
    QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.constFirst().constFirst().toString(), firstPath);
}

void SimulationControlsTest::resetPreservesCircuitAndRestoresInitialValues()
{
    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QSignalSpy actionSpy(&canvas, &CircuitCanvas::actionOccurred);
    const QPoint componentPosition(240, 200);

    canvas.setActiveComponentType("VoltageSource");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, componentPosition);
    QTest::mouseDClick(&canvas, Qt::LeftButton, Qt::NoModifier, componentPosition);
    QVERIFY(actionSpy.constLast().constFirst().toString().contains("set to 1"));

    canvas.runSimulation();
    canvas.resetSimulation();
    QCOMPARE(canvas.simulationState(), CircuitCanvas::SimulationState::Stopped);

    QTest::mouseDClick(&canvas, Qt::LeftButton, Qt::NoModifier, componentPosition);
    QVERIFY(actionSpy.constLast().constFirst().toString().contains("set to 1"));
}

void SimulationControlsTest::projectDataRoundTripPreservesComponents()
{
    CircuitCanvas source;
    source.resize(500, 400);
    source.show();
    QVERIFY(QTest::qWaitForWindowExposed(&source));

    const QPoint componentPosition(240, 200);
    source.setActiveComponentType("VoltageSource");
    QTest::mouseClick(&source, Qt::LeftButton, Qt::NoModifier, componentPosition);
    QTest::mouseDClick(&source, Qt::LeftButton, Qt::NoModifier, componentPosition);

    const ProjectFileData saved = source.projectData("Round trip", QSize(800, 600));
    QCOMPARE(saved.projectName, QString("Round trip"));
    QCOMPARE(saved.canvasSize, QSize(800, 600));
    QCOMPARE(saved.components.size(), 1);
    QVERIFY(saved.components.constFirst().stateOn);

    CircuitCanvas restored;
    QString errorMessage;
    QVERIFY2(restored.loadProjectData(saved, &errorMessage), qPrintable(errorMessage));
    const ProjectFileData loaded = restored.projectData("Round trip", QSize(800, 600));
    QCOMPARE(loaded.components.size(), 1);
    QCOMPARE(loaded.components.constFirst().id, saved.components.constFirst().id);
    QCOMPARE(loaded.components.constFirst().type, saved.components.constFirst().type);
    QCOMPARE(loaded.components.constFirst().position, saved.components.constFirst().position);
    QCOMPARE(loaded.components.constFirst().stateOn, saved.components.constFirst().stateOn);
}

void SimulationControlsTest::floatingInputsAreReported()
{
    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setActiveComponentType("AndGate");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(240, 200));

    QSignalSpy actionSpy(&canvas, &CircuitCanvas::actionOccurred);
    canvas.stepSimulation();
    QCOMPARE(actionSpy.count(), 1);
    const QString message = actionSpy.constFirst().constFirst().toString();
    QVERIFY(message.contains("Floating input(s)"));
    QVERIFY(message.contains("AND1.A"));
    QVERIFY(message.contains("AND1.B"));
}

void SimulationControlsTest::batteryDrivesLedHigh()
{
    CircuitCanvas canvas;
    canvas.resize(620, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setActiveComponentType("Battery");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(160, 200));
    canvas.setActiveComponentType("Led");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(420, 200));

    QTest::keyClick(&canvas, Qt::Key_W);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(218, 200));
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(362, 200));
    QTest::keyClick(&canvas, Qt::Key_W);
    canvas.stepSimulation();

    const ProjectFileData project = canvas.projectData("Battery", QSize(800, 600));
    QCOMPARE(project.components.size(), 2);
    QCOMPARE(project.components[0].type, QString("Battery"));
    QCOMPARE(project.components[1].type, QString("Led"));
    QVERIFY(project.components[1].stateOn);
}

void SimulationControlsTest::nandAndXorTruthTables()
{
    auto buildTwoInputCircuit = [](CircuitCanvas *canvas, const QString &gateType) {
        canvas->resize(720, 460);
        canvas->show();
        QVERIFY(QTest::qWaitForWindowExposed(canvas));

        canvas->setActiveComponentType("VoltageSource");
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 160));
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 280));
        canvas->setActiveComponentType(gateType);
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(360, 220));
        canvas->setActiveComponentType("Led");
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(580, 220));

        const QList<QPair<QPoint, QPoint>> connections{
            {QPoint(198, 160), QPoint(302, 204)},
            {QPoint(198, 280), QPoint(302, 236)},
            {QPoint(418, 220), QPoint(522, 220)}
        };
        for (const auto &connection : connections) {
            QTest::keyClick(canvas, Qt::Key_W);
            QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, connection.first);
            QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, connection.second);
            QTest::keyClick(canvas, Qt::Key_W);
        }
    };

    CircuitCanvas nandCanvas;
    buildTwoInputCircuit(&nandCanvas, "NandGate");
    QTest::mouseDClick(&nandCanvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 160));
    QTest::mouseDClick(&nandCanvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 280));
    nandCanvas.stepSimulation();
    ProjectFileData nandResult = nandCanvas.projectData("NAND", QSize(800, 600));
    QVERIFY(!nandResult.components.constLast().stateOn);
    QTest::mouseDClick(&nandCanvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 280));
    nandCanvas.stepSimulation();
    nandResult = nandCanvas.projectData("NAND", QSize(800, 600));
    QVERIFY(nandResult.components.constLast().stateOn);

    CircuitCanvas xorCanvas;
    buildTwoInputCircuit(&xorCanvas, "XorGate");
    QTest::mouseDClick(&xorCanvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 160));
    xorCanvas.stepSimulation();
    ProjectFileData xorResult = xorCanvas.projectData("XOR", QSize(800, 600));
    QVERIFY(xorResult.components.constLast().stateOn);
    QTest::mouseDClick(&xorCanvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 280));
    xorCanvas.stepSimulation();
    xorResult = xorCanvas.projectData("XOR", QSize(800, 600));
    QVERIFY(!xorResult.components.constLast().stateOn);
}

void SimulationControlsTest::mirroringPersistsAndCanvasExportsToPng()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    CircuitCanvas canvas;
    canvas.resize(500, 400);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));
    canvas.setDocumentCanvasSize(QSize(500, 400));
    canvas.setActiveComponentType("Resistor");
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(240, 200));
    QTest::keyClick(&canvas, Qt::Key_M);
    QTest::keyClick(&canvas, Qt::Key_M, Qt::ShiftModifier);

    const ProjectFileData saved = canvas.projectData("Mirrored", QSize(800, 600));
    QCOMPARE(saved.components.size(), 1);
    QVERIFY(saved.components.constFirst().mirrored);
    QVERIFY(saved.components.constFirst().mirroredVertically);

    CircuitCanvas restored;
    QString errorMessage;
    QVERIFY2(restored.loadProjectData(saved, &errorMessage), qPrintable(errorMessage));
    const ProjectFileData loaded = restored.projectData("Mirrored", QSize(800, 600));
    QVERIFY(loaded.components.constFirst().mirrored);
    QVERIFY(loaded.components.constFirst().mirroredVertically);

    const QString imagePath = directory.filePath("canvas.png");
    QVERIFY2(canvas.exportToPng(imagePath, &errorMessage), qPrintable(errorMessage));
    QVERIFY(QFile::exists(imagePath));
    const QImage image(imagePath);
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), canvas.size());
}

QTEST_MAIN(SimulationControlsTest)

#include "simulationcontrolstest.moc"
