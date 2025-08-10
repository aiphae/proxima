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
    _setupUI();
    _connectUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::_setupUI() {
    ui->mediaViewer->setMinimumSize(300, 300);
}

void MainWindow::_connectUI() {
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
        if (file == _currentFile) {
            return;
        }
        _currentFile = file;
        _showFile();
    });

    // 'Stacking' dialog push button
    connect(ui->stackingPushButton, &QPushButton::clicked, this, [this]() {
        auto stackingDialog = _initializeStackingDialog();
        stackingDialog->show();
    });
}

void MainWindow::_showFile() {
    ui->mediaViewer->show(_currentFile);
}

StackingDialog *MainWindow::_initializeStackingDialog() {
    auto stackingDialog = new StackingDialog(this);

    ui->workspace->enableMultipleSelection(true);

    connect(stackingDialog, &StackingDialog::analyzeFinished, this, [this](MediaCollection *collection, const std::vector<int> &map) {
        // Show sorted frames
        ui->mediaViewer->show(*collection, map);
        // Block the workspace (for convenience)
        ui->workspaceFrame->setEnabled(false);
    });

    connect(stackingDialog, &StackingDialog::previewConfigChanged, this, [this](ModifyingFunction func) {
        ui->mediaViewer->setModifyingFunction(func);
    });

    // When dialog is closed
    connect(stackingDialog, &QDialog::finished, this, [this]() {
        // Disable multiple selection and reset check boxes
        ui->workspace->enableMultipleSelection(false);
        ui->workspace->resetMultipleSelection();
        ui->workspaceFrame->setEnabled(true);
    });

    connect(ui->workspace, &Workspace::itemChecked, this, [stackingDialog](MediaFile *file, bool flag) {
        stackingDialog->includeFile(file, flag);
    });

    return stackingDialog;
}
