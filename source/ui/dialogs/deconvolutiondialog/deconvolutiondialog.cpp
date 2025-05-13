#include "deconvolutiondialog.h"
#include "ui_deconvolutiondialog.h"
#include "threading/thread.h"

DeconvolutionDialog::DeconvolutionDialog(cv::Mat &mat, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeconvolutionDialog)
    , mat(mat)
{
    ui->setupUi(this);

    imageDisplay = std::make_unique<Display>(ui->previewDisplay);
    psfDisplay = std::make_unique<Display>(ui->psfDisplay);

    imageDisplay->show(mat);

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
        std::thread([this]() {
            cv::Mat result = Deconvolution::deconvolve(this->mat, Deconvolution::LucyRichardson, this->config,
                [this](int iteration) {
                    QMetaObject::invokeMethod(this, [this, iteration] {
                        ui->progressEdit->setText(QString::number(iteration));
                    });
                }
            );
            imageDisplay->show(result);
        }).detach();
    });

    ui->iterationsSlider->setValue(defaultIterations);
    ui->psfSizeSlider->setValue(defaultPSFSize * 100);
    ui->kurtosisSlider->setValue(defaultPSFKurtosis * 100);
}

DeconvolutionDialog::~DeconvolutionDialog() {
    delete ui;
}

void DeconvolutionDialog::updatePSF() {
    config.psf = computePSF(ui->psfSizeSpinBox->value(), ui->kurtosisDoubleSpinBox->value());
    psfDisplay->show(config.psf);
}
