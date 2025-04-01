#include "processpage.h"
#include "ui_processpage.h"

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcessPage)
{
    ui->setupUi(this);
}

ProcessPage::~ProcessPage() {
    delete ui;
}
