#include "main_window.h"
#include "ui_main_window.h"
#include "home_page/home_page.h"
#include "stack_page/stack_page.h"
#include "process_page/process_page.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Proxima v1.0");
    initializePages();
    connectButtons();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initializePages() {
    auto homePage = new HomePage(this);
    ui->stackedWidget->insertWidget(static_cast<int>(Pages::Home), homePage);

    auto stackPage = new StackPage(this);
    ui->stackedWidget->insertWidget(static_cast<int>(Pages::Stack), stackPage);

    auto processPage = new ProcessPage(this);
    ui->stackedWidget->insertWidget(static_cast<int>(Pages::Process), processPage);

    // Move to home page
    ui->stackedWidget->setCurrentIndex(static_cast<int>(Pages::Home));

    connect(homePage, &HomePage::currentPageChanged, this, &MainWindow::updateCurrentPage);
}

void MainWindow::connectButtons() {
    // Home button
    connect(ui->homePushButton, &QPushButton::clicked, this, [this]() {
        updateCurrentPage(static_cast<int>(Pages::Home));
    });
}

void MainWindow::updateCurrentPage(int pageIndex) {
    ui->stackedWidget->setCurrentIndex(pageIndex);
}
