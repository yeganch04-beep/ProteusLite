#include "startpage.h"

#include <QBrush>
#include <QColor>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

StartPage::StartPage(QWidget *parent)
    : QWidget(parent)
    , recentProjectsList(new QListWidget(this))
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(80, 60, 80, 60);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("ProteusLite", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *subtitleLabel = new QLabel("OOP Circuit Simulator", this);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(13);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);

    auto *buttonsLayout = new QHBoxLayout();
    auto *newProjectButton = new QPushButton("New Project", this);
    auto *openProjectButton = new QPushButton("Open Project", this);
    auto *exitButton = new QPushButton("Exit", this);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(newProjectButton);
    buttonsLayout->addWidget(openProjectButton);
    buttonsLayout->addWidget(exitButton);
    buttonsLayout->addStretch();

    auto *recentLabel = new QLabel("Recent Projects", this);
    QFont recentFont = recentLabel->font();
    recentFont.setBold(true);
    recentLabel->setFont(recentFont);

    recentProjectsList->setObjectName("recentProjectsList");
    recentProjectsList->setMaximumHeight(120);

    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(recentLabel);
    mainLayout->addWidget(recentProjectsList);
    mainLayout->addStretch();

    connect(newProjectButton, &QPushButton::clicked, this, &StartPage::newProjectRequested);
    connect(openProjectButton, &QPushButton::clicked, this, &StartPage::openProjectRequested);
    connect(exitButton, &QPushButton::clicked, this, &StartPage::exitRequested);

    connect(recentProjectsList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item) {
                const QString filePath = item->data(Qt::UserRole).toString();
                if (!filePath.isEmpty()) {
                    emit recentProjectSelected(filePath);
                }
            });

    setRecentProjects({});
}

void StartPage::setRecentProjects(const QStringList &filePaths)
{
    recentProjectsList->clear();

    for (const QString &filePath : filePaths) {
        const QFileInfo fileInfo(filePath);
        auto *item = new QListWidgetItem(fileInfo.completeBaseName(), recentProjectsList);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setToolTip(fileInfo.absoluteFilePath());
    }

    if (recentProjectsList->count() == 0) {
        auto *emptyItem = new QListWidgetItem("No recent projects yet", recentProjectsList);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QBrush(QColor(120, 120, 120)));
    }
}
