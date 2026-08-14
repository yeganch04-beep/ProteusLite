#include "projectfile.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

bool requireString(const QJsonObject &object,
                   const QString &key,
                   QString *value,
                   QString *errorMessage)
{
    const QJsonValue jsonValue = object.value(key);
    if (!jsonValue.isString() || jsonValue.toString().trimmed().isEmpty()) {
        setError(errorMessage, QString("Missing or invalid string field: %1").arg(key));
        return false;
    }
    *value = jsonValue.toString();
    return true;
}

QJsonObject pointToJson(const QPoint &point)
{
    return {{"x", point.x()}, {"y", point.y()}};
}

bool pointFromJson(const QJsonValue &value, QPoint *point, QString *errorMessage)
{
    if (!value.isObject()) {
        setError(errorMessage, "Component position must be a JSON object.");
        return false;
    }
    const QJsonObject object = value.toObject();
    if (!object.value("x").isDouble() || !object.value("y").isDouble()) {
        setError(errorMessage, "Component position must contain numeric x and y values.");
        return false;
    }
    *point = QPoint(object.value("x").toInt(), object.value("y").toInt());
    return true;
}

} // namespace

bool ProjectFile::save(const QString &fileName,
                       const ProjectFileData &project,
                       QString *errorMessage)
{
    if (fileName.trimmed().isEmpty()) {
        setError(errorMessage, "A destination file name is required.");
        return false;
    }
    if (!project.canvasSize.isValid() || project.canvasSize.isEmpty()) {
        setError(errorMessage, "The project canvas size is invalid.");
        return false;
    }

    QJsonArray components;
    for (const ProjectComponentData &component : project.components) {
        components.append(QJsonObject{
            {"id", component.id},
            {"type", component.type},
            {"label", component.label},
            {"position", pointToJson(component.position)},
            {"rotationDegrees", component.rotationDegrees},
            {"stateOn", component.stateOn}
        });
    }

    QJsonArray wires;
    for (const ProjectWireData &wire : project.wires) {
        wires.append(QJsonObject{
            {"id", wire.id},
            {"startComponentId", wire.startComponentId},
            {"startPinName", wire.startPinName},
            {"endComponentId", wire.endComponentId},
            {"endPinName", wire.endPinName}
        });
    }

    const QJsonObject root{
        {"formatVersion", project.formatVersion},
        {"projectName", project.projectName},
        {"canvasSize", QJsonObject{{"width", project.canvasSize.width()},
                                   {"height", project.canvasSize.height()}}},
        {"components", components},
        {"wires", wires}
    };

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(errorMessage, QString("Could not open the file for writing: %1").arg(file.errorString()));
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        setError(errorMessage, QString("Could not write the project file: %1").arg(file.errorString()));
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        setError(errorMessage, QString("Could not finish saving the project: %1").arg(file.errorString()));
        return false;
    }
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

bool ProjectFile::load(const QString &fileName,
                       ProjectFileData *project,
                       QString *errorMessage)
{
    if (project == nullptr) {
        setError(errorMessage, "The project output pointer is null.");
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QString("Could not open the project file: %1").arg(file.errorString()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QString("Invalid JSON project file: %1").arg(parseError.errorString()));
        return false;
    }

    const QJsonObject root = document.object();
    if (!root.value("formatVersion").isDouble()
        || root.value("formatVersion").toInt() != 1) {
        setError(errorMessage, "Unsupported or missing project format version.");
        return false;
    }

    ProjectFileData loaded;
    loaded.formatVersion = 1;
    if (!requireString(root, "projectName", &loaded.projectName, errorMessage)) {
        return false;
    }

    const QJsonValue canvasValue = root.value("canvasSize");
    if (!canvasValue.isObject()) {
        setError(errorMessage, "Missing or invalid canvasSize object.");
        return false;
    }
    const QJsonObject canvasObject = canvasValue.toObject();
    const int width = canvasObject.value("width").toInt();
    const int height = canvasObject.value("height").toInt();
    if (width <= 0 || height <= 0) {
        setError(errorMessage, "Canvas width and height must be positive.");
        return false;
    }
    loaded.canvasSize = QSize(width, height);

    if (!root.value("components").isArray() || !root.value("wires").isArray()) {
        setError(errorMessage, "The project must contain components and wires arrays.");
        return false;
    }

    QSet<QString> componentIds;
    for (const QJsonValue &value : root.value("components").toArray()) {
        if (!value.isObject()) {
            setError(errorMessage, "Every component must be a JSON object.");
            return false;
        }
        const QJsonObject object = value.toObject();
        ProjectComponentData component;
        if (!requireString(object, "id", &component.id, errorMessage)
            || !requireString(object, "type", &component.type, errorMessage)
            || !requireString(object, "label", &component.label, errorMessage)
            || !pointFromJson(object.value("position"), &component.position, errorMessage)) {
            return false;
        }
        if (componentIds.contains(component.id)) {
            setError(errorMessage, QString("Duplicate component id: %1").arg(component.id));
            return false;
        }
        componentIds.insert(component.id);
        component.rotationDegrees = object.value("rotationDegrees").toInt();
        component.stateOn = object.value("stateOn").toBool(false);
        loaded.components.append(component);
    }

    QSet<QString> wireIds;
    for (const QJsonValue &value : root.value("wires").toArray()) {
        if (!value.isObject()) {
            setError(errorMessage, "Every wire must be a JSON object.");
            return false;
        }
        const QJsonObject object = value.toObject();
        ProjectWireData wire;
        if (!requireString(object, "id", &wire.id, errorMessage)
            || !requireString(object, "startComponentId", &wire.startComponentId, errorMessage)
            || !requireString(object, "startPinName", &wire.startPinName, errorMessage)
            || !requireString(object, "endComponentId", &wire.endComponentId, errorMessage)
            || !requireString(object, "endPinName", &wire.endPinName, errorMessage)) {
            return false;
        }
        if (wireIds.contains(wire.id)) {
            setError(errorMessage, QString("Duplicate wire id: %1").arg(wire.id));
            return false;
        }
        if (!componentIds.contains(wire.startComponentId)
            || !componentIds.contains(wire.endComponentId)) {
            setError(errorMessage, QString("Wire %1 references a missing component.").arg(wire.id));
            return false;
        }
        wireIds.insert(wire.id);
        loaded.wires.append(wire);
    }

    *project = loaded;
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}
