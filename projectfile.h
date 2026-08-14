#ifndef PROJECTFILE_H
#define PROJECTFILE_H

#include <QPoint>
#include <QSize>
#include <QString>
#include <QVector>

struct ProjectComponentData
{
    QString id;
    QString type;
    QString label;
    QPoint position;
    int rotationDegrees = 0;
    bool stateOn = false;
};

struct ProjectWireData
{
    QString id;
    QString startComponentId;
    QString startPinName;
    QString endComponentId;
    QString endPinName;
};

struct ProjectFileData
{
    int formatVersion = 1;
    QString projectName;
    QSize canvasSize;
    QVector<ProjectComponentData> components;
    QVector<ProjectWireData> wires;
};

class ProjectFile
{
public:
    static bool save(const QString &fileName,
                     const ProjectFileData &project,
                     QString *errorMessage = nullptr);
    static bool load(const QString &fileName,
                     ProjectFileData *project,
                     QString *errorMessage = nullptr);
};

#endif // PROJECTFILE_H
