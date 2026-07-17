#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QStackedWidget;
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
    void openProjectPlaceholder();
    void showEditorPage(const QString &projectName, int canvasWidth, int canvasHeight);
    void showStartPage();

    Ui::MainWindow *ui;
    QStackedWidget *pageStack;
    StartPage *startPage;
    QWidget *editorPage;
    QLabel *projectLogLabel;
    int currentCanvasWidth;
    int currentCanvasHeight;
};

#endif // MAINWINDOW_H
