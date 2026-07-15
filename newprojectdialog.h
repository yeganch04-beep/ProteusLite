#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    QString projectName() const;
    int canvasWidth() const;
    int canvasHeight() const;

private:
    void updateCanvasSizeFields();

    QLineEdit *projectNameEdit;
    QComboBox *sizePresetComboBox;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
};

#endif // NEWPROJECTDIALOG_H
