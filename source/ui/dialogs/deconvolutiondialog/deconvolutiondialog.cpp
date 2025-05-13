#include "deconvolutiondialog.h"
#include "ui_deconvolutiondialog.h"

DeconvolutionDialog::DeconvolutionDialog(cv::Mat &mat, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeconvolutionDialog)
    , mat(mat)
{
    ui->setupUi(this);

    imageDisplay = std::make_unique<Display>(ui->previewDisplay);
    psfDisplay = std::make_unique<Display>(ui->psfDisplay);

    imageDisplay->show(mat);

    computePSF();

    connect(ui->previewPushButton, &QPushButton::clicked, this, [this]() {
        cv::Mat result = Deconvolution::deconvolve(this->mat, Deconvolution::LucyRichardson, this->config);
        imageDisplay->show(result);
    });

    connect(ui->iterationsSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        config.iterations = value;
    });

    connect(ui->psfSizeSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {
        computePSF();
    });

    connect(ui->kurtosisDoubleSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {
        computePSF();
    });
}

DeconvolutionDialog::~DeconvolutionDialog() {
    delete ui;
}

void DeconvolutionDialog::computePSF() {
    PSF psf{ui->psfSizeSpinBox->value(), ui->kurtosisDoubleSpinBox->value()};
    config.psf = psf;
    psfDisplay->show(psf.psf);
}
