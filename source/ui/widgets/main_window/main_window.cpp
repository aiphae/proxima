#include "main_window.h"
#include "ui_main_window.h"
#include <QFileDialog>
#include "stacking_dialog/stacking_dialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Proxima v1.0");
    setupUI();
    connectUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupUI() {
    ui->mediaViewer->setMinimumSize(300, 300);
}

void MainWindow::connectUI() {
    // 'Add items' push button
    connect(ui->addItemsPushButton, &QPushButton::clicked, this, [this]() {
        const auto files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath());
        if (files.isEmpty()) {
            return;
        }
        for (const auto &file : files) {
            ui->workspace->addItem(file.toStdString());
        }
        ui->totalFilesEdit->setText(QString::number(ui->workspace->itemCount()));
    });

    // 'Clear workspace' push button
    connect(ui->clearWorkspacePushButton, &QPushButton::clicked, this, [this]() {
        ui->workspace->clear();
        ui->totalFilesEdit->setText("");
        ui->mediaViewer->clear();
    });

    // Clicking on items in workspace
    connect(ui->workspace, &Workspace::itemClicked, this, [this](MediaFile *file) {
        if (file == currentFile) {
            return;
        }
        currentFile = file;
        showFile();
    });

    // 'Stacking' dialog push button
    connect(ui->stackingPushButton, &QPushButton::clicked, this, [this]() {
        auto stackingDialog = new StackingDialog(this);
        stackingDialog->show(); // show() to not block the main window
        ui->workspace->enableMultipleSelection(true);
        connect(stackingDialog, &StackingDialog::analyzeFinished, this, [this](MediaCollection *collection, const std::vector<int> &map) {
            ui->mediaViewer->show(*collection, map);
        });
        // When dialog is closed
        connect(stackingDialog, &QDialog::finished, this, [this]() {
            // Disable multiple selection and reset check boxes
            ui->workspace->enableMultipleSelection(false);
            ui->workspace->resetMultipleSelection();
        });
        connect(ui->workspace, &Workspace::itemChecked, this, [stackingDialog](MediaFile *file, bool flag) {
            stackingDialog->includeFile(file, flag);
        });
    });
}

void MainWindow::showFile() {
    ui->mediaViewer->show(currentFile);
}
