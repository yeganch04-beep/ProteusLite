#ifndef COMPONENTLISTWIDGET_H
#define COMPONENTLISTWIDGET_H

#include <QString>
#include <QWidget>

class ComponentPreviewWidget;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class ComponentListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ComponentListWidget(QWidget *parent = nullptr);

signals:
    void componentTypeSelected(const QString &typeName);

private:
    void addLibraryItem(const QString &category,
                        const QString &label,
                        const QString &typeName);
    void applyFilter(const QString &filterText);
    void updatePreview(QTreeWidgetItem *item);
    void addToActiveList(QTreeWidgetItem *item);
    void removeSelectedActiveItem();
    void activateItem(QListWidgetItem *item);

    QLineEdit *searchEdit;
    QTreeWidget *libraryTree;
    ComponentPreviewWidget *previewWidget;
    QLabel *searchStatusLabel;
    QListWidget *activeList;
    QPushButton *addButton;
    QPushButton *removeButton;
};

#endif // COMPONENTLISTWIDGET_H
