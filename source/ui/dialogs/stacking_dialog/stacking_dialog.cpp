#include "stacking_dialog.h"
#include "ui_stacking_dialog.h"
#include "components/frame.h"
#include "stacking/alignment.h"
#include "threading/analyze_thread.h"
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>

StackingDialog::StackingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StackingDialog)
    , _analyzingThread(_files, _frameQualities)
    , _stackThread(_files, _config, _percentages, _frameQualities, _outputDir)
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

    connect(ui->stackPushButton, &QPushButton::clicked, this, &StackingDialog::_stack);

    connect(&_stackThread, &_StackThread::frameProcessed, this, [this](QString current) {
        ui->stackingProgressEdit->setText(current);
    });

    connect(&_stackThread, &_StackThread::finished, this, [this](const std::vector<std::string> &output) {
        for (const auto &file : output) {
            _output.emplace_back(file);
        }
    });

    connect(ui->localAlignmentCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        _estimateAlignmentPoints();
        _updateModifyingFunction();
        emit previewConfigChanged(_modifyingFunction);
    });

    connect(ui->apSizeSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        _estimateAlignmentPoints();
        _updateModifyingFunction();
        emit previewConfigChanged(_modifyingFunction);
    });

    connect(ui->featureBasedApsCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        _estimateAlignmentPoints();
        _updateModifyingFunction();
        emit previewConfigChanged(_modifyingFunction);
    });

    connect(ui->showApsCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        _updateModifyingFunction();
        emit previewConfigChanged(_modifyingFunction);
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

void StackingDialog::_stack() {
    const QString path = QFileDialog::getExistingDirectory();
    if (path.isEmpty()) {
        return;
    }

    _outputDir = path.toStdString();

    _collectConfig();

    _stackThread.start();
}

void StackingDialog::_collectConfig() {
    static const std::vector<QSpinBox *> percentageSpinBoxes{
        ui->toStackSpinBox_1, ui->toStackSpinBox_2,
        ui->toStackSpinBox_3, ui->toStackSpinBox_4
    };

    for (int i = 0; i < percentageSpinBoxes.size(); ++i) {
        _percentages[i] = percentageSpinBoxes[i]->value();
    }

    if (ui->upsampleCheckBox->isChecked()) {
        _config.upsample = ui->upsampleFactorSpinBox->value();
    }
    else {
        _config.upsample = 1.0;
    }
}

void StackingDialog::closeEvent(QCloseEvent *event) {
    // Return saved files' paths if the check box is checked
    if (ui->addToWorkspaceCheckBox->isChecked()) {
        emit closed(_output);
    }
    // Else return an empty vector
    else {
        emit closed({});
    }
    emit finished(0);
}

void StackingDialog::_estimateAlignmentPoints() {
    if (!ui->localAlignmentCheckBox->isChecked()) {
        _aps.clear();
        return;
    }

    cv::Mat reference = _files.matAtFrame(_frameQualities[0].first);
    reference = Frame::centerObject(reference, reference.cols, reference.rows);

    int apSize = ui->apSizeSpinBox->value();
    AlignmentPointSet::Placement placement = ui->featureBasedApsCheckBox->isChecked()
        ? AlignmentPointSet::Placement::FeatureBased
        : AlignmentPointSet::Placement::Uniform;

    _aps = AlignmentPointSet::estimate(reference, {placement, apSize});
}

void StackingDialog::_updateModifyingFunction() {
    _modifyingFunction = [this](cv::Mat &mat) -> void {
        mat = Frame::centerObject(mat, mat.rows, mat.cols);
        for (const auto &ap : _aps) {
            cv::Rect rect = ap.rect();
            rect &= cv::Rect{0, 0, mat.cols, mat.rows};
            cv::rectangle(mat, rect, cv::Scalar(0, 255, 0), 1);
        }
        mat = Frame::expandBorders(mat, _config.outputWidth, _config.outputHeight);
    };
}
