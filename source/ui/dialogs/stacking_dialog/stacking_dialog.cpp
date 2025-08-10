#include "stacking_dialog.h"
#include "ui_stacking_dialog.h"
#include "components/frame.h"
#include "boost/asio/thread_pool.hpp"
#include "boost/asio/post.hpp"
#include "stacking/alignment.h"
#include "threading/analyze_thread.h"
#include <QMessageBox>
#include <QDebug>

StackingDialog::StackingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StackingDialog)
    , _analyzingThread(_files, _frameQualities)
{
    ui->setupUi(this);
    this->setWindowTitle("Stacking (Proxima)");

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, &StackingDialog::_analyzeFiles);

    connect(ui->localAlignmentCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->alignmentOptionsFrame->setEnabled(state == Qt::Checked);
    });

    connect(&_analyzingThread, &AnalyzeThread::progressUpdated, this, [this](int current) {
        ui->analyzingProgressEdit->setText(QString::number(current) + "/" + QString::number(_files.totalFrames()));
    });

    connect(&_analyzingThread, &AnalyzeThread::finished, this, [this]() {
        ui->analyzingProgressEdit->setText("Finished!");

        // Extract only indices from the <index, quality> vector
        std::vector<int> map;
        map.reserve(_frameQualities.size());
        for (const auto &[id, quality] : _frameQualities) {
            map.push_back(id);
        }

        _updateOutputDimensions();

        // Enable UI
        _enableStackingOptions(true);
        this->setEnabled(true);

        // Show sorted frames in main window
        emit analyzeFinished(&_files, map);
    });
}

StackingDialog::~StackingDialog() {
    delete ui;
}

void StackingDialog::includeFile(MediaFile *file, bool flag) {
    if (flag) {
        _files.addFile(file);
    }
    else {
        _files.removeFile(file);
    }

    ui->selectedFilesEdit->setText(QString::number(_files.fileCount()));
    ui->totalFramesEdit->setText(QString::number(_files.totalFrames()));

    if (_files.fileCount() > 0) {
        ui->analyzeGroupBox->setEnabled(true);
    }
}

void StackingDialog::_analyzeFiles() {
    if (!_files.allDimensionsEqual()) {
        QMessageBox::critical(this, "Error", "All files must have the same dimension.");
        return;
    }

    _frameQualities.clear();
    _frameQualities.resize(_files.totalFrames());

    // Block UI while processing
    _enableStackingOptions(false);
    this->setEnabled(false);

    _analyzingThread.start();
}

void StackingDialog::_enableStackingOptions(bool flag) {
    ui->alignmentGroupBox->setEnabled(flag);
    ui->optionsGroupBox->setEnabled(flag);
    ui->stackGroupBox->setEnabled(flag);
    ui->outputGroupBox->setEnabled(flag);
}

void StackingDialog::_estimateAlignmentConfig() {
    cv::Mat reference = _files.matAtFrame(_frameQualities[0].first);
    reference = Frame::centerObject(reference, reference.cols, reference.rows);

    int apSize = ui->apSizeSpinBox->value();
    AlignmentPointSet::Placement placement = ui->featureBasedApsCheckBox->isChecked()
        ? AlignmentPointSet::Placement::FeatureBased
        : AlignmentPointSet::Placement::Uniform;
    AlignmentPointSet::Config config{placement, apSize};

    AlignmentPointSet aps = AlignmentPointSet::estimate(reference, config);
}

void StackingDialog::_updateOutputDimensions() {
    cv::Mat frame = _files.matAtFrame(0);
    int width = frame.cols, height = frame.rows;
    ui->widthSpinBox->blockSignals(true);
    ui->heightSpinBox->blockSignals(true);
    ui->widthSpinBox->setValue(width);
    ui->heightSpinBox->setValue(height);
    ui->widthSpinBox->blockSignals(false);
    ui->heightSpinBox->blockSignals(false);
}
