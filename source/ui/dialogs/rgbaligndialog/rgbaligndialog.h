#ifndef RGBALIGNDIALOG_H
#define RGBALIGNDIALOG_H

#include <QDialog>

namespace Ui {
class RGBAlignDialog;
}

class RGBAlignDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RGBAlignDialog(QWidget *parent = nullptr);
    ~RGBAlignDialog();

private:
    Ui::RGBAlignDialog *ui;
};

#endif // RGBALIGNDIALOG_H
