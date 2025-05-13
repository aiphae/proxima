#ifndef DECONVOLUTIONDIALOG_H
#define DECONVOLUTIONDIALOG_H

#include <QDialog>
#include "components/display.h"
#include "processing/deconvolution.h"

namespace Ui {
class DeconvolutionDialog;
}

class DeconvolutionDialog : public QDialog {
    Q_OBJECT

public:
    explicit DeconvolutionDialog(cv::Mat &mat, QWidget *parent = nullptr);
    ~DeconvolutionDialog();

private:
    Ui::DeconvolutionDialog *ui;
    std::unique_ptr<Display> imageDisplay;
    std::unique_ptr<Display> psfDisplay;

    Deconvolution::Config config;
    cv::Mat &mat;

    void computePSF();

    static constexpr int defaultIterations = 30;
    static constexpr double defaultPSFSize = 1.5;
    static constexpr double defaultPSFKurtosis = 2.0;
};

#endif // DECONVOLUTIONDIALOG_H
