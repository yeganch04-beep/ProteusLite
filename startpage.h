#ifndef STARTPAGE_H
#define STARTPAGE_H

#include <QWidget>
#include <QStringList>

class QListWidget;

class StartPage : public QWidget
{
    Q_OBJECT

public:
    explicit StartPage(QWidget *parent = nullptr);
    void setRecentProjects(const QStringList &filePaths);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void exitRequested();
    void recentProjectSelected(const QString &filePath);

private:
    QListWidget *recentProjectsList;
};

#endif // STARTPAGE_H
