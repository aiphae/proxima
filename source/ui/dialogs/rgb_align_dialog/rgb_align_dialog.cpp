#include "rgb_align_dialog.h"
#include "ui_rgb_align_dialog.h"

RGBAlignDialog::RGBAlignDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RGBAlignDialog)
{
    ui->setupUi(this);
}

RGBAlignDialog::~RGBAlignDialog()
{
    delete ui;
}
