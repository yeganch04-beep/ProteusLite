#include "componentlistwidget.h"

#include <QAbstractItemView>
#include <QDrag>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int ComponentTypeRole = Qt::UserRole;

class DraggableComponentTree final : public QTreeWidget
{
public:
    using QTreeWidget::QTreeWidget;

protected:
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override
    {
        auto *mimeData = new QMimeData;
        if (!items.isEmpty()) {
            const QString typeName = items.constFirst()->data(0, ComponentTypeRole).toString();
            if (!typeName.isEmpty()) {
                mimeData->setText(typeName);
            }
        }
        return mimeData;
    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction;
    }
};

class DraggableComponentList final : public QListWidget
{
public:
    using QListWidget::QListWidget;

protected:
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override
    {
        auto *mimeData = new QMimeData;
        if (!items.isEmpty()) {
            const QString typeName = items.constFirst()->data(ComponentTypeRole).toString();
            if (!typeName.isEmpty()) {
                mimeData->setText(typeName);
            }
        }
        return mimeData;
    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction;
    }
};

} // namespace

class ComponentPreviewWidget final : public QWidget
{
public:
    explicit ComponentPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(105);
        setToolTip("Schematic preview of the selected library component");
    }

    void setComponent(const QString &label, const QString &typeName)
    {
        m_label = label;
        m_typeName = typeName;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(248, 249, 251));
        painter.setPen(QPen(QColor(205, 210, 218), 1));
        painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 6, 6);

        if (m_typeName.isEmpty()) {
            painter.setPen(QColor(100, 105, 115));
            painter.drawText(rect(), Qt::AlignCenter, "Select a component\nto preview it");
            return;
        }

        QFont titleFont = painter.font();
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(QColor(45, 50, 60));
        painter.drawText(QRect(8, 6, width() - 16, 20), Qt::AlignCenter, m_label);

        painter.save();
        painter.translate(width() / 2.0, 66.0);
        painter.setPen(QPen(QColor(45, 105, 180), 2));
        painter.setBrush(Qt::NoBrush);

        if (m_typeName == "Resistor") {
            painter.drawLine(-58, 0, -27, 0);
            painter.drawRect(QRectF(-27, -11, 54, 22));
            painter.drawLine(27, 0, 58, 0);
        } else if (m_typeName == "Capacitor") {
            painter.drawLine(-58, 0, -12, 0);
            painter.drawLine(-12, -20, -12, 20);
            painter.drawLine(12, -20, 12, 20);
            painter.drawLine(12, 0, 58, 0);
        } else if (m_typeName == "Inductor") {
            painter.drawLine(-58, 0, -36, 0);
            painter.drawLine(36, 0, 58, 0);
            QPainterPath path;
            path.moveTo(-36, 0);
            path.arcTo(QRectF(-36, -13, 18, 26), 180, -180);
            path.arcTo(QRectF(-18, -13, 18, 26), 180, -180);
            path.arcTo(QRectF(0, -13, 18, 26), 180, -180);
            path.arcTo(QRectF(18, -13, 18, 26), 180, -180);
            painter.drawPath(path);
        } else if (m_typeName == "Diode" || m_typeName == "Led") {
            painter.drawLine(-58, 0, -23, 0);
            painter.drawLine(18, 0, 58, 0);
            QPolygonF triangle;
            triangle << QPointF(-23, -20) << QPointF(-23, 20) << QPointF(14, 0);
            painter.drawPolygon(triangle);
            painter.drawLine(18, -22, 18, 22);
            if (m_typeName == "Led") {
                painter.drawLine(QPointF(-1, -25), QPointF(13, -39));
                painter.drawLine(QPointF(13, -39), QPointF(10, -30));
                painter.drawLine(QPointF(13, -39), QPointF(4, -36));
            }
        } else if (m_typeName == "Switch") {
            painter.drawLine(-58, 0, -20, 0);
            painter.drawEllipse(QPointF(-20, 0), 3, 3);
            painter.drawLine(QPointF(-20, 0), QPointF(20, -20));
            painter.drawEllipse(QPointF(24, 0), 3, 3);
            painter.drawLine(24, 0, 58, 0);
        } else if (m_typeName == "Ground") {
            painter.drawLine(0, -28, 0, -7);
            painter.drawLine(-25, -7, 25, -7);
            painter.drawLine(-16, 3, 16, 3);
            painter.drawLine(-8, 13, 8, 13);
        } else if (m_typeName == "VoltageSource") {
            painter.drawLine(-58, 0, -24, 0);
            painter.drawEllipse(QPointF(0, 0), 24, 24);
            painter.drawLine(24, 0, 58, 0);
            painter.drawText(QRectF(-17, -9, 12, 18), Qt::AlignCenter, "-");
            painter.drawText(QRectF(5, -9, 12, 18), Qt::AlignCenter, "+");
        } else if (m_typeName == "AndGate") {
            painter.drawLine(-58, -13, -25, -13);
            painter.drawLine(-58, 13, -25, 13);
            painter.drawLine(25, 0, 58, 0);
            QPainterPath path;
            path.moveTo(-25, -27);
            path.lineTo(0, -27);
            path.arcTo(QRectF(-27, -27, 54, 54), 90, -180);
            path.lineTo(-25, 27);
            path.closeSubpath();
            painter.drawPath(path);
        } else if (m_typeName == "OrGate") {
            painter.drawLine(-58, -13, -28, -13);
            painter.drawLine(-58, 13, -28, 13);
            painter.drawLine(27, 0, 58, 0);
            QPainterPath path;
            path.moveTo(-33, -27);
            path.quadTo(-12, 0, -33, 27);
            path.quadTo(3, 24, 28, 0);
            path.quadTo(3, -24, -33, -27);
            path.closeSubpath();
            painter.drawPath(path);
        } else if (m_typeName == "NotGate") {
            painter.drawLine(-58, 0, -25, 0);
            painter.drawLine(31, 0, 58, 0);
            QPolygonF triangle;
            triangle << QPointF(-25, -24) << QPointF(-25, 24) << QPointF(20, 0);
            painter.drawPolygon(triangle);
            painter.drawEllipse(QPointF(26, 0), 5, 5);
        }

        painter.restore();
    }

private:
    QString m_label;
    QString m_typeName;
};

ComponentListWidget::ComponentListWidget(QWidget *parent)
    : QWidget(parent)
    , searchEdit(new QLineEdit(this))
    , libraryTree(new DraggableComponentTree(this))
    , previewWidget(new ComponentPreviewWidget(this))
    , searchStatusLabel(new QLabel(this))
    , activeList(new DraggableComponentList(this))
    , addButton(new QPushButton("Add to Active", this))
    , removeButton(new QPushButton("Remove", this))
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(6);

    searchEdit->setPlaceholderText("Search components...");
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setToolTip("Filter by component name or category");

    libraryTree->setHeaderHidden(true);
    libraryTree->setRootIsDecorated(true);
    libraryTree->setAlternatingRowColors(true);
    libraryTree->setSelectionMode(QAbstractItemView::SingleSelection);
    libraryTree->setDragEnabled(true);
    libraryTree->setDragDropMode(QAbstractItemView::DragOnly);
    libraryTree->setDefaultDropAction(Qt::CopyAction);
    libraryTree->setMinimumHeight(170);

    activeList->setSelectionMode(QAbstractItemView::SingleSelection);
    activeList->setDragEnabled(true);
    activeList->setDragDropMode(QAbstractItemView::DragOnly);
    activeList->setDefaultDropAction(Qt::CopyAction);
    activeList->setMinimumHeight(80);
    activeList->setToolTip("Click to select or drag an active component onto the canvas");

    searchStatusLabel->setText("No matching components.");
    searchStatusLabel->setAlignment(Qt::AlignCenter);
    searchStatusLabel->setStyleSheet("color: #8a3c3c;");
    searchStatusLabel->hide();

    auto *libraryLabel = new QLabel("Component Library", this);
    QFont sectionFont = libraryLabel->font();
    sectionFont.setBold(true);
    libraryLabel->setFont(sectionFont);

    auto *previewLabel = new QLabel("Schematic Preview", this);
    previewLabel->setFont(sectionFont);

    auto *activeLabel = new QLabel("Active Components", this);
    activeLabel->setFont(sectionFont);

    auto *activeHint = new QLabel("Double-click a library item or use Add.", this);
    activeHint->setWordWrap(true);
    activeHint->setStyleSheet("color: #666;");

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);

    mainLayout->addWidget(libraryLabel);
    mainLayout->addWidget(searchEdit);
    mainLayout->addWidget(libraryTree, 2);
    mainLayout->addWidget(searchStatusLabel);
    mainLayout->addWidget(previewLabel);
    mainLayout->addWidget(previewWidget);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(activeLabel);
    mainLayout->addWidget(activeHint);
    mainLayout->addWidget(activeList, 1);

    addLibraryItem("Analog", "Resistor", "Resistor");
    addLibraryItem("Analog", "Capacitor", "Capacitor");
    addLibraryItem("Analog", "Inductor", "Inductor");
    addLibraryItem("Analog", "Diode", "Diode");
    addLibraryItem("Interactive & Output", "LED", "Led");
    addLibraryItem("Interactive & Output", "Switch", "Switch");
    addLibraryItem("Sources", "Ground", "Ground");
    addLibraryItem("Sources", "Digital Voltage Source", "VoltageSource");
    addLibraryItem("Digital Logic", "AND Gate", "AndGate");
    addLibraryItem("Digital Logic", "OR Gate", "OrGate");
    addLibraryItem("Digital Logic", "NOT Gate", "NotGate");
    libraryTree->expandAll();

    connect(searchEdit, &QLineEdit::textChanged,
            this, &ComponentListWidget::applyFilter);
    connect(libraryTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
                updatePreview(current);
            });
    connect(libraryTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int) {
                addToActiveList(item);
            });
    connect(addButton, &QPushButton::clicked, this,
            [this]() {
                addToActiveList(libraryTree->currentItem());
            });
    connect(removeButton, &QPushButton::clicked,
            this, &ComponentListWidget::removeSelectedActiveItem);
    connect(activeList, &QListWidget::itemClicked,
            this, &ComponentListWidget::activateItem);
    connect(activeList, &QListWidget::itemDoubleClicked,
            this, &ComponentListWidget::activateItem);
}

void ComponentListWidget::addLibraryItem(const QString &category,
                                         const QString &label,
                                         const QString &typeName)
{
    QTreeWidgetItem *categoryItem = nullptr;
    for (int index = 0; index < libraryTree->topLevelItemCount(); ++index) {
        QTreeWidgetItem *candidate = libraryTree->topLevelItem(index);
        if (candidate->text(0) == category) {
            categoryItem = candidate;
            break;
        }
    }

    if (categoryItem == nullptr) {
        categoryItem = new QTreeWidgetItem(libraryTree, QStringList{category});
        categoryItem->setFlags(categoryItem->flags()
                               & ~Qt::ItemIsSelectable
                               & ~Qt::ItemIsDragEnabled);
        QFont categoryFont = categoryItem->font(0);
        categoryFont.setBold(true);
        categoryItem->setFont(0, categoryFont);
    }

    auto *componentItem = new QTreeWidgetItem(categoryItem, QStringList{label});
    componentItem->setData(0, ComponentTypeRole, typeName);
    componentItem->setToolTip(0, QString("Double-click to add %1 to the active list").arg(label));
    componentItem->setFlags(componentItem->flags() | Qt::ItemIsDragEnabled);
}

void ComponentListWidget::applyFilter(const QString &filterText)
{
    const QString filter = filterText.trimmed();
    int visibleComponentCount = 0;

    for (int categoryIndex = 0;
         categoryIndex < libraryTree->topLevelItemCount();
         ++categoryIndex) {
        QTreeWidgetItem *categoryItem = libraryTree->topLevelItem(categoryIndex);
        const bool categoryMatches = categoryItem->text(0).contains(filter, Qt::CaseInsensitive);
        int visibleChildren = 0;

        for (int childIndex = 0; childIndex < categoryItem->childCount(); ++childIndex) {
            QTreeWidgetItem *componentItem = categoryItem->child(childIndex);
            const QString typeName = componentItem->data(0, ComponentTypeRole).toString();
            const bool matches = filter.isEmpty()
                                 || categoryMatches
                                 || componentItem->text(0).contains(filter, Qt::CaseInsensitive)
                                 || typeName.contains(filter, Qt::CaseInsensitive);
            componentItem->setHidden(!matches);
            if (matches) {
                ++visibleChildren;
                ++visibleComponentCount;
            }
        }

        categoryItem->setHidden(visibleChildren == 0);
        if (!filter.isEmpty() && visibleChildren > 0) {
            categoryItem->setExpanded(true);
        }
    }

    searchStatusLabel->setVisible(visibleComponentCount == 0);
}

void ComponentListWidget::updatePreview(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        previewWidget->setComponent(QString(), QString());
        return;
    }

    const QString typeName = item->data(0, ComponentTypeRole).toString();
    if (typeName.isEmpty()) {
        previewWidget->setComponent(QString(), QString());
        return;
    }

    previewWidget->setComponent(item->text(0), typeName);
}

void ComponentListWidget::addToActiveList(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    const QString typeName = item->data(0, ComponentTypeRole).toString();
    if (typeName.isEmpty()) {
        return;
    }

    for (int index = 0; index < activeList->count(); ++index) {
        QListWidgetItem *activeItem = activeList->item(index);
        if (activeItem->data(ComponentTypeRole).toString() == typeName) {
            activeList->setCurrentItem(activeItem);
            activateItem(activeItem);
            return;
        }
    }

    auto *activeItem = new QListWidgetItem(item->text(0), activeList);
    activeItem->setData(ComponentTypeRole, typeName);
    activeItem->setToolTip(QString("Click to select or drag %1 onto the canvas")
                               .arg(item->text(0)));
    activeList->setCurrentItem(activeItem);
    activateItem(activeItem);
}

void ComponentListWidget::removeSelectedActiveItem()
{
    const int selectedRow = activeList->currentRow();
    if (selectedRow < 0) {
        return;
    }

    delete activeList->takeItem(selectedRow);

    if (activeList->count() == 0) {
        emit componentTypeSelected(QString());
        return;
    }

    const int nextRow = qMin(selectedRow, activeList->count() - 1);
    activeList->setCurrentRow(nextRow);
    activateItem(activeList->item(nextRow));
}

void ComponentListWidget::activateItem(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    const QString typeName = item->data(ComponentTypeRole).toString();
    if (!typeName.isEmpty()) {
        emit componentTypeSelected(typeName);
    }
}
