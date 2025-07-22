#include "main_window.h"
#include "ui_main_window.h"
#include <QFileDialog>

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
    ui->display->setAlignment(Qt::AlignCenter);
}

void MainWindow::connectUI() {
    connect(ui->addItemsPushButton, &QPushButton::clicked, this, [this]() {
        const auto files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath());
        if (files.isEmpty()) {
            return;
        }
        for (const auto &file : files) {
            ui->workspace->addItem(file.toStdString());
        }
        ui->totalFilesEdit->setText(QString::number(ui->workspace->count()));
    });

    connect(ui->clearWorkspacePushButton, &QPushButton::clicked, this, [this]() {
        ui->workspace->clear();
        ui->totalFilesEdit->setText("");
        ui->display->clear();
        ui->frameHorizontalSlider->setValue(0);
        ui->frameHorizontalSlider->setEnabled(false);
    });

    connect(ui->frameHorizontalSlider, &QSlider::valueChanged, this, [this](int current) {
        ui->display->show(currentFile->matAtFrame(current));
    });

    connect(ui->workspace, &Workspace::itemClicked, this, [this](MediaFile *file) {
        if (file == currentFile) {
            return;
        }
        currentFile = file;
        showFile();
    });
}

void MainWindow::showFile() {
    bool isVideo = currentFile->isVideo();
    ui->frameHorizontalSlider->setEnabled(isVideo);

    if (isVideo) {
        ui->frameHorizontalSlider->setMinimum(0);
        ui->frameHorizontalSlider->setMaximum(currentFile->frames() - 1);
    }

    ui->frameHorizontalSlider->setValue(0);
    emit ui->frameHorizontalSlider->valueChanged(0);
}
