#include "process_page.h"
#include "ui_process_page.h"
#include <QFileDialog>
#include <QDesktopServices>
#include "deconvolution_dialog/deconvolution_dialog.h"
#include "rgb_align_dialog/rgb_align_dialog.h"
#include "data/media_file.h"

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcessPage)
{
    ui->setupUi(this);
    display = std::make_unique<Display>(ui->imageDisplay);
    setupUI();
    resetUI();
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
        "Images (*.png *.jpg *.jpeg *.tif *.tiff)"
    );

    if (file.isEmpty()) {
        return;
    }

    resetUI();

    processor.reset();
    MediaFile image(file);
    processor.load(image.matAtFrame(0));
    ui->selectedFileEdit->setText(QString::fromStdString(image.filename()));
    display->show(processor.mat());
}

void ProcessPage::saveFile() {
    QString outputDir = QFileDialog::getExistingDirectory();
    if (outputDir.isEmpty()) {
        return;
    }

    QString filename = outputDir + "/proxima-processed-" + QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm") + ".tif";
    cv::Mat result = processor.mat();
    result.convertTo(result, CV_16UC3, 65535.0);
    cv::imwrite(filename.toStdString(), result, {cv::IMWRITE_TIFF_COMPRESSION, 1});

    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
}

void ProcessPage::displayProcessed() {
    processor.apply();
    display->show(processor.mat());
}

void ProcessPage::setupUI() {
    gainSliders = {
        ui->gainSlider1, ui->gainSlider2, ui->gainSlider3,
        ui->gainSlider4, ui->gainSlider5
    };
}

void ProcessPage::resetUI() {
    for (auto &gainSlider : gainSliders) {
        gainSlider->setValue(gainSlider->minimum());
    }

    for (auto &slider : {ui->brightnessSlider, ui->contrastSlider, ui->saturationSlider}) {
        slider->setValue(slider->minimum() / 2 + slider->maximum() / 2);
    }
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

    for (QSlider* slider : gainSliders) {
        connect(slider, &QSlider::valueChanged, this, [this]() {
            if (processor.mat().empty()) {
                return;
            }

            std::vector<float> gains;
            for (auto &s : gainSliders) {
                gains.push_back(static_cast<float>(s->value()) / 50.0f);
            }

            processor.setWaveletsGains(gains);
            displayProcessed();
        });
    }
}
