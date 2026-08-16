#include "projectfile.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class ProjectFileTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripPreservesProjectData();
    void invalidJsonIsRejected();
};

void ProjectFileTest::roundTripPreservesProjectData()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath("round-trip.json");

    ProjectFileData source;
    source.projectName = "Logic demo";
    source.canvasSize = QSize(1123, 794);

    ProjectComponentData input;
    input.id = "component-1";
    input.type = "VoltageSource";
    input.label = "VDC1";
    input.value = "5 V";
    input.position = QPoint(100, 200);
    input.rotationDegrees = 90;
    input.stateOn = true;
    source.components.append(input);

    ProjectComponentData output;
    output.id = "component-2";
    output.type = "Led";
    output.label = "LED1";
    output.position = QPoint(300, 200);
    source.components.append(output);

    ProjectWireData wire;
    wire.id = "wire-1";
    wire.startComponentId = input.id;
    wire.startPinName = "OUT";
    wire.endComponentId = output.id;
    wire.endPinName = "IN";
    source.wires.append(wire);

    QString errorMessage;
    QVERIFY2(ProjectFile::save(fileName, source, &errorMessage), qPrintable(errorMessage));

    ProjectFileData loaded;
    QVERIFY2(ProjectFile::load(fileName, &loaded, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(loaded.formatVersion, 1);
    QCOMPARE(loaded.projectName, source.projectName);
    QCOMPARE(loaded.canvasSize, source.canvasSize);
    QCOMPARE(loaded.components.size(), 2);
    QCOMPARE(loaded.wires.size(), 1);
    QCOMPARE(loaded.components[0].id, input.id);
    QCOMPARE(loaded.components[0].type, input.type);
    QCOMPARE(loaded.components[0].label, input.label);
    QCOMPARE(loaded.components[0].value, input.value);
    QCOMPARE(loaded.components[0].position, input.position);
    QCOMPARE(loaded.components[0].rotationDegrees, input.rotationDegrees);
    QCOMPARE(loaded.components[0].stateOn, input.stateOn);
    QCOMPARE(loaded.wires[0].startComponentId, wire.startComponentId);
    QCOMPARE(loaded.wires[0].endComponentId, wire.endComponentId);
}

void ProjectFileTest::invalidJsonIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath("invalid.json");

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("{not-json") > 0);
    file.close();

    ProjectFileData project;
    QString errorMessage;
    QVERIFY(!ProjectFile::load(fileName, &project, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

QTEST_APPLESS_MAIN(ProjectFileTest)

#include "projectfiletest.moc"
