#include "stackpage.h"
#include "stacking/stackingthread.h"
#include "ui_stackpage.h"
#include "components/frame.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

// A helper function to build a filter with supported media files for user input
QString fileFilters();

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StackPage)
    , sortingThread(manager, config.sorted)
    , stackingThread(manager, config, percentages, outputDir)
{
    ui->setupUi(this);
    display = std::make_unique<Display>(ui->imageDisplay);
    connectUI();
}

StackPage::~StackPage() {
    delete ui;
}

void StackPage::selectFiles() {
    const QVector<QString> files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath(), fileFilters());
    if (files.isEmpty()) {
        return;
    }

    // Clear previous data
    manager.clear();
    config.aps.clear();

    // Load new files and check for validity
    manager.load(files);
    if (manager.totalFrames() == 0) {
        return;
    }
    if (!manager.allDimensionsEqual()) {
        QMessageBox::critical(this, "Invalid input", "All files must have equal dimensions.");
        return;
    }

    initializeConfig();
    // Update corresponding UI elements without triggering any signals
    updateOutputDimensions();

    // Update total frames counter
    ui->totalFramesEdit->setText(QString::number(manager.totalFrames()));

    // Update frame slider
    ui->frameSlider->setRange(0, manager.totalFrames() - 1);
    ui->frameSlider->setValue(0);

    // Enable analyzing group box
    ui->analyzingGroupBox->setEnabled(true);

    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (manager.totalFrames() == 0) {
        return;
    }

    cv::Mat frame = manager.matAtFrame(config.sorted[frameNumber].first);
    if (frame.empty()) {
        return;
    }

    frame = Frame::centerObject(frame, config.outputWidth, config.outputHeight);

    // Display alignment points if checked
    if (ui->showApsCheckBox->isChecked() && !config.aps.empty()) {
        for (const auto &ap : config.aps) {
            cv::rectangle(frame, ap.rect(), cv::Scalar(0, 255, 0), 1);
        }
    }

    display->show(frame);
}

void StackPage::estimateAPGrid() {
    cv::Mat reference = manager.matAtFrame(config.sorted[0].first);
    reference = Frame::centerObject(reference, config.outputWidth, config.outputHeight);

    int apSize = ui->apSizeSpinBox->value();
    AlignmentPointSet::Placement placement = ui->featureBasedApPlacement->isChecked()
        ? AlignmentPointSet::Placement::FeatureBased
        : AlignmentPointSet::Placement::Uniform;
    AlignmentPointSet::Config apConfig {placement, apSize};
    config.aps = AlignmentPointSet::estimate(reference, apConfig);

    if (config.aps.empty()) {
        ui->apAmountEdit->setText("<font color='red'>No APs detected!</font>");
    }
    else {
        ui->apAmountEdit->setText(QString::number(config.aps.size()));
    }
}

void StackPage::stack() {
    if (manager.totalFrames() == 0) {
        return;
    }

    outputDir = QFileDialog::getExistingDirectory();
    if (outputDir.isEmpty()) {
        return;
    }

    percentages.clear();

    std::vector<QSpinBox *> spinBoxes = {
        ui->percentToStackSpinBox1, ui->percentToStackSpinBox2,
        ui->percentToStackSpinBox3, ui->percentToStackSpinBox4,
        ui->percentToStackSpinBox5
    };

    for (auto &spinBox : spinBoxes) {
        if (spinBox->value() > 0) {
            percentages.push_back(spinBox->value());
        }
    }

    if (percentages.empty()) {
        return;
    }

    stackingThread.start();
}

//
// Initizizes the stacking config.
//
// Resizes the 'sorted' array to match the total frames number to avoid
// out-of-bounds indexing in 'displayFrame'.
//
// Also sets default outputWidth and outputHeight.
//
void StackPage::initializeConfig() {
    // Initialize 'sorted' array with empty values
    config.sorted.resize(manager.totalFrames(), {0, 0.0});
    for (int i = 0; i < manager.totalFrames(); ++i) {
        config.sorted[i].first = i;
    }

    // Set output dimensions as first media's dimensions
    config.outputWidth = manager[0].dimensions().width;
    config.outputHeight = manager[0].dimensions().height;
}

void StackPage::enableConfigEdit() {
    ui->alignmentGroupBox->setEnabled(true);
    ui->stackOptionsGroupBox->setEnabled(true);
    ui->outputOptionsGroupBox->setEnabled(true);
    ui->advancedOptionsGroupBox->setEnabled(true);
    ui->processingGroupBox->setEnabled(true);
}

void StackPage::connectUI() {
    // Frame slider on top
    // Updates 'currentFrame' and displays it
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    // Width and height spin boxes
    // Updates the config and displays current frame
    connect(ui->widthSpinBox, &QSpinBox::editingFinished, this, [this]() {
        config.outputWidth = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });
    connect(ui->heightSpinBox, &QSpinBox::editingFinished, this, [this]() {
        config.outputHeight = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    // Local alignment check box
    // Enables the group box to configure local alignment and displays current frame
    connect(ui->localAlignmentCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->localAlignmentFrame->setEnabled(state == Qt::Checked);
        config.localAlign = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    // Alignment point size spin box
    connect(ui->apSizeSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        estimateAPGrid();
        displayFrame(currentFrame);
    });

    // Alignment point placement radio buttons
    // Estimate new AP grid and display current frame
    connect(ui->featureBasedApPlacement, &QRadioButton::clicked, this, [this]() {
        estimateAPGrid();
        displayFrame(currentFrame);
    });
    connect(ui->uniformBasedApPlacement, &QRadioButton::clicked, this, [this]() {
        estimateAPGrid();
        displayFrame(currentFrame);
    });

    // Check box to preview alignment points
    connect(ui->showApsCheckBox, &QCheckBox::checkStateChanged, this, [this]() {
        displayFrame(currentFrame);
    });

    // Drizzle edits
    connect(ui->drizzleCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        bool checked = (state == Qt::Checked);
        ui->drizzle1_5xRadioButton->setEnabled(checked);
        ui->drizzle2_0xRadioButton->setEnabled(checked);
        if (!checked) {
            config.drizzle = 1.0;
        }
    });
    connect(ui->drizzle1_5xRadioButton, &QRadioButton::clicked, this, [this]() {
        config.drizzle = 1.5;
    });
    connect(ui->drizzle2_0xRadioButton, &QRadioButton::clicked, this, [this]() {
        config.drizzle = 2.0;
    });

    // Push buttons
    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() { sortingThread.start(); });
    connect(ui->selectFilesPushButton, &QPushButton::clicked, this, &StackPage::selectFiles);
    connect(ui->stackPushButton, &QPushButton::clicked, this, &StackPage::stack);

    // Sorting thread connections
    connect(&sortingThread, &SortingThread::progressUpdated, this, [this](int current) {
        QString status = QString("%1/%2").arg(current).arg(manager.totalFrames());
        ui->analyzingProgressEdit->setText(status);
    });
    connect(&sortingThread, &SortingThread::finished, this, [this]() {
        estimateAPGrid();
        displayFrame(0);
        enableConfigEdit();
    });

    // Stacking thread connections
    connect(&stackingThread, &StackingThread::frameProcessed, this, [this](QString status) {
        ui->stackingProgressEdit->setText(status);
    });
    connect(&stackingThread, &StackingThread::statusUpdated, this, [this](QString status) {
        ui->statusEdit->setText(status);
    });
    connect(&stackingThread, &StackingThread::finished, this, [this]() {
        // Open output directory
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
    });
}

void StackPage::updateOutputDimensions() {
    ui->widthSpinBox->blockSignals(true);
    ui->widthSpinBox->setValue(config.outputWidth);
    ui->widthSpinBox->blockSignals(false);

    ui->heightSpinBox->blockSignals(true);
    ui->heightSpinBox->setValue(config.outputHeight);
    ui->heightSpinBox->blockSignals(false);
}

QString fileFilters() {
    auto buildFilter = [](const QString &description, const QSet<QString> &extensions) {
        QStringList patterns;
        for (const QString &extension : extensions) {
            patterns << "*" + extension;
        }
        return QString("%1 (%2)").arg(description, patterns.join(' '));
    };

    QString imageFilter = buildFilter("Image Files", imageExtensions);
    QString videoFilter = buildFilter("Video Files", videoExtensions);

    QSet<QString> allExtensions = imageExtensions + videoExtensions;
    QString allFilter = buildFilter("All Supported Files", allExtensions);

    return allFilter + ";;" + imageFilter + ";;" + videoFilter;
}
