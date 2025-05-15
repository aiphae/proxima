#ifndef RGB_ALIGN_DIALOG_H
#define RGB_ALIGN_DIALOG_H

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

#endif // RGB_ALIGN_DIALOG_H
