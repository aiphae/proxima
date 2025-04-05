#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "homepage/homepage.h"
#include "preprocesspage/preprocesspage.h"
#include "stackpage/stackpage.h"
#include "processpage/processpage.h"

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

    auto alignPage = new AlignPage(this);
    ui->stackedWidget->insertWidget(static_cast<int>(Pages::Align), alignPage);

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

    // Settings button
    // Later ...
}

void MainWindow::updateCurrentPage(int pageIndex) {
    ui->stackedWidget->setCurrentIndex(pageIndex);
}
