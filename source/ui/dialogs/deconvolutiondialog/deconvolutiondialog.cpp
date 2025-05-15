#include "deconvolutiondialog.h"
#include "ui_deconvolutiondialog.h"
#include "components/frame.h"

DeconvolutionDialog::DeconvolutionDialog(cv::Mat &mat, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeconvolutionDialog)
    , original(mat)
{
    ui->setupUi(this);

    int previewSize = std::min(std::min(original.cols, original.rows), 200);
    preview = Frame::centerObject(original, previewSize, previewSize);

    ui->previewDisplay->loadOriginal(mat);
    psfDisplay = std::make_unique<Display>(ui->psfDisplay);

    connect(ui->iterationsSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        ui->iterationsSlider->blockSignals(true);
        ui->iterationsSlider->setValue(value);
        ui->iterationsSlider->blockSignals(false);
        config.iterations = value;
    });
    connect(ui->iterationsSlider, &QSlider::valueChanged, this, [this](int value) {
        ui->iterationsSpinBox->blockSignals(true);
        ui->iterationsSpinBox->setValue(value);
        ui->iterationsSpinBox->blockSignals(false);
        config.iterations = value;
    });

    connect(ui->psfSizeSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        ui->psfSizeSlider->blockSignals(true);
        ui->psfSizeSlider->setValue(value * 100);
        ui->psfSizeSlider->blockSignals(false);
        updatePSF();
    });
    connect(ui->psfSizeSlider, &QSlider::valueChanged, this, [this](int value) {
        ui->psfSizeSpinBox->blockSignals(true);
        ui->psfSizeSpinBox->setValue(value / 100.0);
        ui->psfSizeSpinBox->blockSignals(false);
        updatePSF();
    });

    connect(ui->kurtosisDoubleSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        ui->kurtosisSlider->blockSignals(true);
        ui->kurtosisSlider->setValue(value * 100);
        ui->kurtosisSlider->blockSignals(false);
        updatePSF();
    });
    connect(ui->kurtosisSlider, &QSlider::valueChanged, this, [this](int value) {
        ui->kurtosisDoubleSpinBox->blockSignals(true);
        ui->kurtosisDoubleSpinBox->setValue(value / 100.0);
        ui->kurtosisDoubleSpinBox->blockSignals(false);
        updatePSF();
    });

    connect(ui->previewPushButton, &QPushButton::clicked, this, [this]() {
        previewDeconvolution();
    });

    connect(ui->applyPushButton, &QPushButton::clicked, this, [this]() {
        applyDeconvolution();
    });

    ui->iterationsSlider->setValue(defaultIterations);
    ui->psfSizeSlider->setValue(defaultPSFSize * 100);
    ui->kurtosisSlider->setValue(defaultPSFKurtosis * 100);
}

DeconvolutionDialog::~DeconvolutionDialog() {
    delete ui;
}

void DeconvolutionDialog::previewDeconvolution() {
    std::thread([this]() {
        auto updateProgress = [this](int current, int total) {
            QMetaObject::invokeMethod(this, [this, current, total] {
                QString progress = QString("%1/%2").arg(current).arg(total);
                ui->progressEdit->setText(progress);
            });
        };
        cv::Mat current = ui->previewDisplay->current();
        cv::Mat result = Deconvolution::deconvolve(current, Deconvolution::LucyRichardson, this->config, updateProgress);
        ui->previewDisplay->show(result);
    }).detach();
}

void DeconvolutionDialog::applyDeconvolution() {
    std::thread([this]() {
        auto updateProgress = [this](int current, int total) {
            QMetaObject::invokeMethod(this, [this, current, total] {
                QString progress = QString("%1/%2").arg(current).arg(total);
                ui->progressEdit->setText(progress);
            });
        };
        cv::Mat result = Deconvolution::deconvolve(original, Deconvolution::LucyRichardson, this->config, updateProgress);
        emit apply(result);
    }).detach();
}

void DeconvolutionDialog::updatePSF() {
    config.psf = computePSF(ui->psfSizeSpinBox->value(), ui->kurtosisDoubleSpinBox->value());
    psfDisplay->show(config.psf);
}
