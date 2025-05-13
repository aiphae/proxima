#include "rgbaligndialog.h"
#include "ui_rgbaligndialog.h"

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
