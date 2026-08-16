#include "newprojectdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
    , projectNameEdit(new QLineEdit(this))
    , sizePresetComboBox(new QComboBox(this))
    , widthSpinBox(new QSpinBox(this))
    , heightSpinBox(new QSpinBox(this))
{
    setWindowTitle("New Project");

    projectNameEdit->setText("Untitled Project");

    sizePresetComboBox->addItems({"A4", "A3", "Infinite Canvas", "Custom"});

    widthSpinBox->setRange(200, 5000);
    heightSpinBox->setRange(200, 5000);

    auto *formLayout = new QFormLayout();
    formLayout->addRow("Project name:", projectNameEdit);
    formLayout->addRow("Canvas size:", sizePresetComboBox);
    formLayout->addRow("Width:", widthSpinBox);
    formLayout->addRow("Height:", heightSpinBox);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    connect(sizePresetComboBox, &QComboBox::currentTextChanged,
            this, &NewProjectDialog::updateCanvasSizeFields);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewProjectDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewProjectDialog::reject);

    updateCanvasSizeFields();
}

QString NewProjectDialog::projectName() const
{
    const QString name = projectNameEdit->text().trimmed();
    return name.isEmpty() ? "Untitled Project" : name;
}

int NewProjectDialog::canvasWidth() const
{
    return widthSpinBox->value();
}

int NewProjectDialog::canvasHeight() const
{
    return heightSpinBox->value();
}

bool NewProjectDialog::isInfiniteCanvas() const
{
    return sizePresetComboBox->currentText() == "Infinite Canvas";
}

void NewProjectDialog::updateCanvasSizeFields()
{
    const QString preset = sizePresetComboBox->currentText();
    const bool customSize = preset == "Custom";

    if (preset == "A4") {
        widthSpinBox->setValue(794);
        heightSpinBox->setValue(1123);
    } else if (preset == "A3") {
        widthSpinBox->setValue(1123);
        heightSpinBox->setValue(1587);
    }

    widthSpinBox->setEnabled(customSize);
    heightSpinBox->setEnabled(customSize);
}
