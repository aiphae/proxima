#include "stackpage.h"
#include "ui_stackpage.h"

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StackPage)
{
    ui->setupUi(this);
}

StackPage::~StackPage() {
    delete ui;
}
