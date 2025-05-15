#include "process_page.h"
#include "ui_process_page.h"
#include <QFileDialog>
#include <QDesktopServices>
#include "deconvolution_dialog/deconvolution_dialog.h"
#include "rgb_align_dialog/rgb_align_dialog.h"

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcessPage)
{
    ui->setupUi(this);
    display = std::make_unique<Display>(ui->imageDisplay);
    connectUI();
}

ProcessPage::~ProcessPage() {
    delete ui;
}

void ProcessPage::selectFile() {
    QString file = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        "",
        "Images (*.png *.jpg *.jpeg *.tif)"
    );

    if (file.isEmpty()) {
        return;
    }

    processor.load(cv::imread(file.toStdString()));
    display->show(processor.mat());
}

void ProcessPage::saveFile() {
    QString outputDir = QFileDialog::getExistingDirectory();
    if (outputDir.isEmpty()) {
        return;
    }

    QString filename = outputDir + "/proxima-processed-" + QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm") + ".tif";
    cv::imwrite(filename.toStdString(), processor.mat());

    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
}

void ProcessPage::displayProcessed() {
    processor.apply();
    display->show(processor.mat());
}

void ProcessPage::connectUI() {
    connect(ui->selectFilePushButton, &QPushButton::clicked, this, &ProcessPage::selectFile);
    connect(ui->saveAsPushButton, &QPushButton::clicked, this, &ProcessPage::saveFile);

    connect(ui->brightnessSlider, &QSlider::valueChanged, this, [this]() {
        processor.setBrightness(ui->brightnessSlider->value());
        displayProcessed();
    });

    connect(ui->contrastSlider, &QSlider::valueChanged, this, [this]() {
        processor.setContrast(ui->contrastSlider->value());
        displayProcessed();
    });

    connect(ui->saturationSlider, &QSlider::valueChanged, this, [this]() {
        processor.setSaturation(ui->saturationSlider->value());
        displayProcessed();
    });

    connect(ui->deconvolutionPushButton, &QPushButton::clicked, this, [this]() {
        cv::Mat original = processor.orig();
        auto dialog = new DeconvolutionDialog(original, this);

        connect(dialog, &DeconvolutionDialog::apply, this, [this, dialog](cv::Mat mat) {
            processor.load(mat);
            displayProcessed();
            dialog->close();
        });

        dialog->exec();
    });

    connect(ui->rgbAlignPushButton, &QPushButton::clicked, this, [this]() {
        auto dialog = new RGBAlignDialog(this);
        dialog->exec();
    });
}
