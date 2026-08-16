#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStringList>

class QLabel;
class QPushButton;
class QStackedWidget;
class CircuitCanvas;
class StartPage;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createStartPage();
    void createMenuActions();
    void createNewProject();
    void openProject();
    bool openProjectFile(const QString &fileName);
    void saveProject();
    void saveProjectAs();
    void exportCanvasImage();
    bool writeProjectFile(const QString &fileName);
    void showEditorPage(const QString &projectName, int canvasWidth, int canvasHeight);
    void showStartPage();
    void updateSimulationControls();
    QStringList recentProjectPaths() const;
    void addRecentProject(const QString &fileName);
    void refreshRecentProjects();

    Ui::MainWindow *ui;
    QStackedWidget *pageStack;
    StartPage *startPage;
    QWidget *editorPage;
    CircuitCanvas *circuitCanvas;
    QPushButton *runButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *resetButton;
    QPushButton *stepButton;
    QPushButton *propertiesButton;
    QLabel *simulationStatusLabel;
    QLabel *projectLogLabel;
    QString currentProjectName;
    QString currentProjectFilePath;
    int currentCanvasWidth;
    int currentCanvasHeight;
};

#endif // MAINWINDOW_H
