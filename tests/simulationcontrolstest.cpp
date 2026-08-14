#include "circuitcanvas.h"
#include "mainwindow.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

class SimulationControlsTest : public QObject
{
    Q_OBJECT

private slots:
    void engineStateTransitions();
    void buttonStateRules();
    void resetPreservesCircuitAndRestoresInitialValues();
    void projectDataRoundTripPreservesComponents();
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
    auto *const statusLabel = window.findChild<QLabel *>("simulationStatusLabel");

    QVERIFY(canvas != nullptr);
    QVERIFY(runButton != nullptr);
    QVERIFY(pauseButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(resetButton != nullptr);
    QVERIFY(statusLabel != nullptr);

    QVERIFY(runButton->isEnabled());
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Stopped"));

    runButton->click();
    QVERIFY(!runButton->isEnabled());
    QVERIFY(pauseButton->isEnabled());
    QVERIFY(stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Running"));

    pauseButton->click();
    QVERIFY(runButton->isEnabled());
    QCOMPARE(runButton->text(), QString("Run"));
    QCOMPARE(runButton->toolTip(), QString("Resume simulation"));
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Paused"));

    stopButton->click();
    QVERIFY(runButton->isEnabled());
    QCOMPARE(runButton->text(), QString("Run"));
    QVERIFY(!pauseButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
    QVERIFY(resetButton->isEnabled());
    QCOMPARE(statusLabel->text(), QString("Stopped"));
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

QTEST_MAIN(SimulationControlsTest)

#include "simulationcontrolstest.moc"
