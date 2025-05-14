#include "processpage.h"
#include "ui_processpage.h"
#include <QFileDialog>
#include <QDesktopServices>
#include "deconvolutiondialog/deconvolutiondialog.h"
#include "rgbaligndialog/rgbaligndialog.h"
#include "components/frame.h"

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

void ProcessPage::connectUI() {
    connect(ui->selectFilePushButton, &QPushButton::clicked, this, &ProcessPage::selectFile);
    connect(ui->saveAsPushButton, &QPushButton::clicked, this, &ProcessPage::saveFile);

    connect(ui->brightnessSlider, &QSlider::valueChanged, this, [this]() {
        processor.setBrightness(ui->brightnessSlider->value());
        processor.apply();
        display->show(processor.mat());
    });

    connect(ui->contrastSlider, &QSlider::valueChanged, this, [this]() {
        processor.setContrast(ui->contrastSlider->value());
        processor.apply();
        display->show(processor.mat());
    });

    connect(ui->saturationSlider, &QSlider::valueChanged, this, [this]() {
        processor.setSaturation(ui->saturationSlider->value());
        processor.apply();
        display->show(processor.mat());
    });

    connect(ui->deconvolutionPushButton, &QPushButton::clicked, this, [this]() {
        cv::Mat original = processor.orig();        
        auto dialog = new DeconvolutionDialog(original, this);

        connect(dialog, &DeconvolutionDialog::apply, this, [this](cv::Mat mat) {
            processor.load(mat);
            display->show(processor.mat());
        });

        dialog->exec();
    });

    connect(ui->rgbAlignPushButton, &QPushButton::clicked, this, [this]() {
        auto dialog = new RGBAlignDialog(this);
        dialog->exec();
    });
}
