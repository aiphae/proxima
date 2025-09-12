#include "stack_dialog.h"
#include "components/frame.h"
#include "stacking/alignment.h"
#include "threading/analysis_thread.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QPushButton>
#include <ranges>

StackDialog::StackDialog(QWidget *parent) : QDialog(parent) {
    setupUI_();
    connectUI_();
}

void StackDialog::includeFile(MediaFile *file, const bool flag) {
    if (flag) {
        files_.addFile(file);
    }
    else {
        files_.removeFile(file);
    }

    // Update UI
    selectedFilesEdit_->setText(QString::number(files_.fileCount()));
    totalFramesEdit_->setText(QString::number(files_.totalFrames()));

    // Enable analysis if 1 or more files have been selected
    analysisGroupBox_->setEnabled(files_.fileCount() > 0);
}

void StackDialog::analyzeFiles_() {
    if (!files_.allDimensionsEqual()) {
        QMessageBox::critical(this, "Error", "All files must have the same dimension.");
        return;
    }

    frameQualities_.assign(files_.totalFrames(), {});

    // Block UI while processing
    enableStackingOptions_(false);
    this->setEnabled(false);

    auto *thread = new AnalysisThread(files_, frameQualities_, this);

    connect(thread, &AnalysisThread::progressUpdated, this, [this](const int current) {
        analysisProgressEdit_->setText(QString::number(current) + "/" + QString::number(files_.totalFrames()));
    });
    connect(thread, &AnalysisThread::finished, this, [this, thread]() {
        analysisProgressEdit_->setText("Finished!");

        // Extract only indices from the <index, quality> vector
        std::vector<int> map;
        map.reserve(frameQualities_.size());
        for (const auto &id: frameQualities_ | std::views::keys) {
            map.push_back(id);
        }

        updateOutputDimensions_();
        enableStackingOptions_(true);
        this->setEnabled(true);

        // Show sorted frames in main window
        emit analysisFinished(&files_, map);
        thread->deleteLater();
    });

    thread->start();
}

void StackDialog::enableStackingOptions_(const bool flag) const {
    alignmentGroupBox_->setEnabled(flag);
    outputOptionsGroupBox_->setEnabled(flag);
    stackingOptionsGroupBox_->setEnabled(flag);
    stackGroupBox_->setEnabled(flag);
}

void StackDialog::updateOutputDimensions_() {
    const cv::Mat frame = files_.matAtFrame(0);
    const int width = frame.cols, height = frame.rows;

    // Update config
    config_.outputWidth = width;
    config_.outputHeight = height;

    // Block signals to avoid recursive triggering
    cropWidthSpinBox_->blockSignals(true);
    cropHeightSpinBox_->blockSignals(true);

    // Set minimum as 1/2 of the original width/height
    cropWidthSpinBox_->setMinimum(static_cast<int>(width * 0.5));
    cropHeightSpinBox_->setMinimum(static_cast<int>(height * 0.5));
    // Set maximum as 3x of the original width/height
    cropWidthSpinBox_->setMaximum(3 * width);
    cropHeightSpinBox_->setMaximum(3 * height);
    // Set values to current dimensions
    cropWidthSpinBox_->setValue(width);
    cropHeightSpinBox_->setValue(height);

    // Enable signals
    cropWidthSpinBox_->blockSignals(false);
    cropHeightSpinBox_->blockSignals(false);
}

void StackDialog::stack_() {
    const QString path = QFileDialog::getExistingDirectory();
    if (path.isEmpty()) {
        return;
    }

    std::string outputDir = path.toStdString();

    std::array<int, 4> percentages{};
    for (int i = 0; i < percentages.size(); ++i) {
        percentages[i] = percentageSpinBoxes_[i]->value();
    }

    auto *thread = new StackThread(files_, config_, percentages, frameQualities_, outputDir, this);

    // Connect thread's signals
    connect(thread, &StackThread::frameProcessed, this, [this](const auto &status) {
        stackProgressEdit_->setText(status);
    });
    connect(thread, &StackThread::singleStackCompleted, this, [this](const auto &output) {
        emit singleStackCompleted(output);
    });
    connect(thread, &StackThread::finished, this, [thread]() {
         thread->deleteLater();
    });

    thread->start();
}

void StackDialog::estimateAlignmentPoints_() {
    if (!localAlignmentCheckBox_->isChecked()) {
        aps_.clear();
        return;
    }

    cv::Mat reference = files_.matAtFrame(frameQualities_[0].first);
    reference = Frame::centerObject(reference, reference.cols, reference.rows);

    const int apSize = apSizeSpinBox_->value();
    const AlignmentPointSet::Placement placement = featureBasedAlignmentCheckBox_->isChecked()
        ? AlignmentPointSet::Placement::FeatureBased
        : AlignmentPointSet::Placement::Uniform;

    aps_ = AlignmentPointSet::estimate(reference, {placement, apSize});
}

void StackDialog::updateModifyingFunction_() {
    modifyingFunction_ = [this](cv::Mat &mat) -> void {
        mat = Frame::centerObject(mat, mat.rows, mat.cols);
        for (const auto &ap : aps_) {
            cv::Rect rect = ap.rect();
            rect &= cv::Rect{0, 0, mat.cols, mat.rows};
            cv::rectangle(mat, rect, cv::Scalar(0, 255, 0), 1);
        }
        mat = Frame::expandBorders(mat, config_.outputWidth, config_.outputHeight);
    };
}

void StackDialog::connectUI_() {
    // 'Analyze frames' push button
    connect(analyzeFramesPushButton_, &QPushButton::clicked, this, &StackDialog::analyzeFiles_);

    // 'Local alignment' check box
    connect(localAlignmentCheckBox_, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState state) {
        const bool checked = state == Qt::Checked;
        localAlignmentOptionsFrame_->setEnabled(checked);
        if (checked) {
            estimateAlignmentPoints_();
            updateModifyingFunction_();
            emit previewConfigChanged(modifyingFunction_);
        }
    });

    // 'AP size' spin box
    connect(apSizeSpinBox_, &QSpinBox::valueChanged, this, [this]() {
        estimateAlignmentPoints_();
        updateModifyingFunction_();
        emit previewConfigChanged(modifyingFunction_);
    });

    // 'Feature-based AP placement' check box
    connect(featureBasedAlignmentCheckBox_, &QCheckBox::checkStateChanged, this, [this]() {
        estimateAlignmentPoints_();
        updateModifyingFunction_();
        emit previewConfigChanged(modifyingFunction_);
    });

    // 'Show APs' check box
    connect(showApsCheckBox_, &QCheckBox::checkStateChanged, this, [this]() {
        updateModifyingFunction_();
        emit previewConfigChanged(modifyingFunction_);
    });

    // Output dimensions spin boxes
    connect(cropWidthSpinBox_, &QSpinBox::valueChanged, this, [this](const int value) {
        config_.outputWidth = value;
        updateModifyingFunction_();
        emit previewConfigChanged(modifyingFunction_);
    });
    connect(cropHeightSpinBox_, &QSpinBox::valueChanged, this, [this](const int value) {
        config_.outputHeight = value;
        updateModifyingFunction_();
        emit previewConfigChanged(modifyingFunction_);
    });

    // Upsample
    connect(upsampleCheckBox_, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState state) {
        config_.upsample = state == Qt::Checked ? upsampleFactorDoubleSpinBox_->value(): 1.0;
    });
    connect(upsampleFactorDoubleSpinBox_, &QDoubleSpinBox::valueChanged, this, [this](const double value) {
        config_.upsample = value;
    });

    // 'Stack' push button
    connect(stackPushButton_, &QPushButton::clicked, this, &StackDialog::stack_);
}

void StackDialog::setupUI_() {
    setWindowTitle("Stack (Proxima)");

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QFont font;
    font.setPointSize(10);
    setFont(font);

    const auto mainLayout = new QGridLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->setColumnMinimumWidth(0, 250);
    mainLayout->setColumnMinimumWidth(1, 250);

    // 'INPUT' GROUP BOX
    const auto inputGroupBox = new QGroupBox("Input", this);
    const auto inputLayout = new QGridLayout(inputGroupBox);

    selectedFilesEdit_ = new QLabel("", inputGroupBox);
    totalFramesEdit_ = new QLabel("", inputGroupBox);

    inputLayout->addWidget(new QLabel("Selected files:", inputGroupBox), 0, 0);
    inputLayout->addWidget(selectedFilesEdit_, 0, 1);
    inputLayout->addWidget(new QLabel("Total frames:", inputGroupBox), 1, 0);
    inputLayout->addWidget(totalFramesEdit_, 1, 1);

    // Add to the main layout
    mainLayout->addWidget(inputGroupBox, 0, 0);

    // 'ANALYSIS' FRAMES GROUP BOX
    analysisGroupBox_ = new QGroupBox("Analysis", this);
    analysisGroupBox_->setEnabled(false); // Disable by default
    const auto analysisLayout = new QGridLayout(analysisGroupBox_);

    analyzeFramesPushButton_ = new QPushButton("Analyze frames", analysisGroupBox_);
    analyzeFramesPushButton_->setMinimumSize(0, 30);

    analysisProgressEdit_ = new QLabel("", inputGroupBox);

    analysisLayout->addWidget(analyzeFramesPushButton_, 0, 0, 1, 2);
    analysisLayout->addWidget(new QLabel("Progress:", analysisGroupBox_), 1, 0);
    analysisLayout->addWidget(analysisProgressEdit_, 1, 1);

    // Add to the main layout
    mainLayout->addWidget(analysisGroupBox_, 0, 1);

    // 'ALIGNMENT' GROUP BOX
    alignmentGroupBox_ = new QGroupBox("Alignment", this);
    alignmentGroupBox_->setEnabled(false); // Disable by default
    const auto alignmentLayout = new QVBoxLayout(alignmentGroupBox_);

    localAlignmentCheckBox_ = new QCheckBox("Local alignment", alignmentGroupBox_);
    alignmentLayout->addWidget(localAlignmentCheckBox_);

    localAlignmentOptionsFrame_ = new QFrame(alignmentGroupBox_);
    const auto localAlignmentOptionsLayout = new QGridLayout(localAlignmentOptionsFrame_);
    localAlignmentOptionsLayout->setContentsMargins(0, 0, 0, 0);

    apSizeSpinBox_ = new QSpinBox(alignmentGroupBox_);
    apSizeSpinBox_->setMinimumSize(0, 30);
    apSizeSpinBox_->setRange(8, 256);
    apSizeSpinBox_->setValue(16);

    featureBasedAlignmentCheckBox_ = new QCheckBox("Feature-based AP placement", alignmentGroupBox_);
    featureBasedAlignmentCheckBox_->setChecked(true); // Check by default
    showApsCheckBox_ = new QCheckBox("Show APs", alignmentGroupBox_);
    showApsCheckBox_->setChecked(true); // Check by default

    localAlignmentOptionsLayout->addWidget(new QLabel("AP size:", alignmentGroupBox_), 0, 0);
    localAlignmentOptionsLayout->addWidget(apSizeSpinBox_, 0, 1);
    localAlignmentOptionsLayout->addWidget(featureBasedAlignmentCheckBox_, 1, 0, 1, 2);
    localAlignmentOptionsLayout->addWidget(showApsCheckBox_, 2, 0, 1, 2);

    alignmentLayout->addWidget(localAlignmentOptionsFrame_);

    // Add to the main layout
    mainLayout->addWidget(alignmentGroupBox_, 1, 0);

    // 'OUTPUT OPTIONS' GROUP BOX
    outputOptionsGroupBox_ = new QGroupBox("Output options", this);
    outputOptionsGroupBox_->setEnabled(false); // Disable by default
    const auto outputOptionsLayout = new QGridLayout(outputOptionsGroupBox_);

    cropWidthSpinBox_ = new QSpinBox(outputOptionsGroupBox_);
    cropWidthSpinBox_->setMinimumSize(0, 30);
    cropHeightSpinBox_ = new QSpinBox(outputOptionsGroupBox_);
    cropHeightSpinBox_->setMinimumSize(0, 30);

    upsampleCheckBox_ = new QCheckBox("Upsample", outputOptionsGroupBox_);

    upsampleFactorDoubleSpinBox_ = new QDoubleSpinBox(outputOptionsGroupBox_);
    upsampleFactorDoubleSpinBox_->setRange(1.0, 2.0);
    upsampleFactorDoubleSpinBox_->setSingleStep(0.10);
    upsampleFactorDoubleSpinBox_->setMinimumSize(0, 30);

    outputOptionsLayout->addWidget(new QLabel("Crop width:", outputOptionsGroupBox_), 0, 0);
    outputOptionsLayout->addWidget(cropWidthSpinBox_, 0, 1);
    outputOptionsLayout->addWidget(new QLabel("Crop height:", outputOptionsGroupBox_), 1, 0);
    outputOptionsLayout->addWidget(cropHeightSpinBox_, 1, 1);
    outputOptionsLayout->addWidget(upsampleCheckBox_, 2, 0);
    outputOptionsLayout->addWidget(new QLabel("Factor:", outputOptionsGroupBox_), 3, 0);
    outputOptionsLayout->addWidget(upsampleFactorDoubleSpinBox_, 3, 1);

    // Add to the main layout
    mainLayout->addWidget(outputOptionsGroupBox_, 1, 1);

    // 'STACKING OPTIONS' GROUP BOX
    stackingOptionsGroupBox_ = new QGroupBox("Stacking options", this);
    stackingOptionsGroupBox_->setEnabled(false); // Disable by default
    const auto stackingOptionsLayout = new QGridLayout(stackingOptionsGroupBox_);

    stackingOptionsLayout->addWidget(new QLabel("% of frames to stack:", stackingOptionsGroupBox_), 0, 0, 1, 2);
    for (int i = 0; i < percentageSpinBoxes_.size(); ++i) {
        percentageSpinBoxes_[i] = new QSpinBox();
        percentageSpinBoxes_[i]->setRange(0, 100);
        percentageSpinBoxes_[i]->setMinimumSize(0, 30);
        stackingOptionsLayout->addWidget(percentageSpinBoxes_[i], i / 2 + 1, i % 2);
    }

    // Add to the main layout
    mainLayout->addWidget(stackingOptionsGroupBox_, 2, 0);

    // 'STACK' GROUP BOX
    stackGroupBox_ = new QGroupBox("Stack", this);
    stackGroupBox_->setEnabled(false); // Disable by default
    const auto outputLayout = new QGridLayout(stackGroupBox_);

    addToWorkspaceCheckBox_ = new QCheckBox("Add to workspace", stackGroupBox_);

    stackPushButton_ = new QPushButton("Stack", stackGroupBox_);
    stackPushButton_->setMinimumSize(0, 30);

    stackProgressEdit_ = new QLabel("", stackGroupBox_);

    outputLayout->addWidget(addToWorkspaceCheckBox_, 0, 0, 1, 2);
    outputLayout->addWidget(stackPushButton_, 1, 0, 1, 2);
    outputLayout->addWidget(new QLabel("Progress:", stackGroupBox_), 2, 0);
    outputLayout->addWidget(stackProgressEdit_, 2, 1);

    // Add to the main layout
    mainLayout->addWidget(stackGroupBox_, 2, 1);
}
