#ifndef STARTPAGE_H
#define STARTPAGE_H

#include <QWidget>

class QListWidget;

class StartPage : public QWidget
{
    Q_OBJECT

public:
    explicit StartPage(QWidget *parent = nullptr);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void exitRequested();
    void recentProjectSelected(const QString &projectName);

private:
    QListWidget *recentProjectsList;
};

#endif // STARTPAGE_H
