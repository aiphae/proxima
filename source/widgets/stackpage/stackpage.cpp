#include "stackpage.h"
#include "ui_stackpage.h"
#include "components/frame.h"
#include "concurrency/threadpool.h"
#include <QFileDialog>
#include <QMessageBox>

// A helper function to build a filter with supported media files for user input
inline QString fileFilters();

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StackPage)
    , source(manager)
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
    source.sorted.clear();

    // Load new files and check for validity
    manager.load(files);
    if (manager.totalFrames() == 0) {
        return;
    }
    if (!manager.allDimensionsEqual()) {
        QMessageBox::critical(this, "Invalid input", "All files must have equal dimensions.");
        return;
    }

    // Initialize 'sorted' array with empty values
    source.sorted.resize(manager.totalFrames(), {0, 0.0});
    for (int i = 0; i < manager.totalFrames(); ++i) {
        source.sorted[i].first = i;
    }

    // Set output dimensions as first media's dimensions
    config.outputWidth = manager[0].dimensions().width;
    config.outputHeight = manager[0].dimensions().height;
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

    cv::Mat frame = manager.matAtFrame(source.sorted[frameNumber].first);
    if (frame.empty()) {
        return;
    }

    frame = Frame::centerObject(frame, config.outputWidth, config.outputHeight);
    display->show(frame);
}

void StackPage::estimateAPGrid() {
    cv::Mat reference = manager.matAtFrame(source.sorted[0].first);
    reference = Frame::centerObject(reference, config.outputWidth, config.outputHeight);

    int apSize = ui->apSizeSpinBox->value();
    APPlacement placement = ui->featureBasedApPlacement->isChecked() ? APPlacement::FeatureBased : APPlacement::Uniform;
    config.aps = Frame::getAps(reference, apSize, placement);

    if (config.aps.empty()) {
        ui->apAmountEdit->setText("<font color='red'>No APs detected!</font>");
    }
    else {
        ui->apAmountEdit->setText(QString::number(config.aps.size()));
    }

    // Preview alignment points
    cv::Mat preview = reference.clone();
    for (const auto &ap : config.aps) {
        cv::rectangle(preview, ap.rect(), cv::Scalar(0, 255, 0), 1);
    }
    display->show(preview);
}

void StackPage::stack() {
    if (source.sorted.empty() || manager.totalFrames() == 0) {
        return;
    }

    std::string outputDir = QFileDialog::getExistingDirectory().toStdString();
    if (outputDir.empty()) {
        return;
    }

    std::vector<QSpinBox *> spinBoxes {
        ui->percentToStackSpinBox1, ui->percentToStackSpinBox2,
        ui->percentToStackSpinBox3, ui->percentToStackSpinBox4,
        ui->percentToStackSpinBox5
    };

    std::thread([this, spinBoxes, outputDir]() {
        for (auto &spinBox : spinBoxes) {
            int percent = spinBox->value();
            if (percent < 1) {
                continue;
            }

            config.framesToStack = manager.totalFrames() * percent / 100;

            QMetaObject::invokeMethod(this, [this, percent]() {
                ui->statusEdit->setText(QString("Stacking %1%").arg(percent));
            });

            cv::Mat stacked = stacker.stack(source, config);
            std::string filename = outputDir + "/proxima-stacked-" + std::to_string(percent) + ".tif";
            cv::imwrite(filename, stacked);
        }

        QMetaObject::invokeMethod(this, [this]() {
            ui->statusEdit->setText("Done!");
        });
    }).detach();
}

void StackPage::analyzeFrames() {
    if (manager.totalFrames() == 0) {
        return;
    }

    // Counter of processed frames
    // 'shared_ptr' because otherwise it would be destroyed after 'thread.detach()'
    std::shared_ptr<std::atomic<int>> framesAnalyzed = std::make_shared<std::atomic<int>>(0);

    // Analyze frames concurrently
    std::thread([this, framesAnalyzed]() {
        // Producer thread
        {
            ThreadPool pool(std::thread::hardware_concurrency() - 2); // Leave 1 for GUI and 1 for producer
            for (int i = 0; i < manager.totalFrames(); ++i) {
                pool.enqueue([i, this, framesAnalyzed]() {
                    cv::Mat frame = manager.matAtFrame(i);
                    source.sorted[i].first = i;
                    source.sorted[i].second = Frame::estimateQuality(frame);
                    int done = ++(*framesAnalyzed);
                    // Emit signal to update progress bar from the main thread
                    emit sortingProgressUpdated(done);
                });
            }
            // Thread pool's destructor is called here,
            // waiting for workers to complete their work
        }
        // Emit the signal to the main thread
        emit analyzingCompleted();
    }).detach();
}

void StackPage::enableConfigEdit() {
    ui->alignmentGroupBox->setEnabled(true);
    ui->stackOptionsGroupBox->setEnabled(true);
    ui->outputOptionsGroupBox->setEnabled(true);
    ui->advancedOptionsGroupBox->setEnabled(true);
    ui->processingGroupBox->setEnabled(true);
}

void StackPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    connect(ui->widthSpinBox, &QSpinBox::editingFinished, this, [this]() {
        config.outputWidth = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->heightSpinBox, &QSpinBox::editingFinished, this, [this]() {
        config.outputHeight = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        analyzeFrames();
    });

    connect(this, &StackPage::sortingProgressUpdated, this, [this](int current) {
        ui->analyzingProgressEdit->setText(QString::number(current) + "/" + QString::number(manager.totalFrames()));
    });

    connect(this, &StackPage::analyzingCompleted, this, [this]() {
        std::sort(source.sorted.begin(), source.sorted.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; }
        );
        enableConfigEdit();
        ui->frameSlider->setValue(0);
        displayFrame(0);
    });

    connect(ui->localAlignmentCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->localAlignmentFrame->setEnabled(state == Qt::Checked);
        config.localAlign = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->drizzleCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        bool checked = (state == Qt::Checked);

        ui->drizzle1_5xRadioButton->setEnabled(checked);
        ui->drizzle2_0xRadioButton->setEnabled(checked);

        if (!checked) {
            config.drizzle = 1.0;
        }
        else if (ui->drizzle1_5xRadioButton->isChecked()) {
            config.drizzle = 1.5;
        }
        else {
            config.drizzle = 2.0;
        }
    });

    connect(&stacker, &Stacker::progressUpdated, this, [this](QString status) {
        ui->stackingProgressEdit->setText(status);
    });

    connect(ui->selectFilesPushButton, &QPushButton::clicked, this, &StackPage::selectFiles);
    connect(ui->estimateAPGridPushButton, &QPushButton::clicked, this, &StackPage::estimateAPGrid);
    connect(ui->stackPushButton, &QPushButton::clicked, this, &StackPage::stack);
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
