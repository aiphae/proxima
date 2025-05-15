#include "home_page.h"
#include "ui_home_page.h"

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomePage)
{
    ui->setupUi(this);
    connectButtons();
}

HomePage::~HomePage() {
    delete ui;
}

void HomePage::connectButtons() {
    QVector<QPushButton *> buttons = {ui->stackPagePushButton, ui->processPagePushButton};
    for (int i = 0; i < buttons.size(); ++i) {
        connect(buttons[i], &QPushButton::clicked, this, [this, i]() {
            // 'i + 1' because 0 is reserved for home page
            emit currentPageChanged(i + 1);
        });
    }
}
