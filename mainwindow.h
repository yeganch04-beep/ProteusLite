#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class CanvasView;
class ComponentListWidget;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    CanvasView *m_canvasView;
    ComponentListWidget *m_compSidebar;
    QDockWidget *m_dock;
};
#endif
