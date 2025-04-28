#include "stackpage.h"
#include "ui_stackpage.h"
#include "components/helpers.h"
#include "components/frame.h"
#include "concurrency/threadpool.h"
#include <QFileDialog>

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StackPage)
{
    ui->setupUi(this);
    display = std::make_unique<Display>(ui->imageDisplay);
    connectUI();
}

StackPage::~StackPage() {
    delete ui;
}

void StackPage::on_selectFilesPushButton_clicked() {
    const QStringList files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath(), fileFilters());
    if (files.isEmpty()) {
        return;
    }

    stacker.source.files.clear();
    stacker.source.sorted.clear();
    totalFrames = 0;

    for (const QString &file : files) {
        MediaFile media(file);
        if (media.isValid()) {
            stacker.source.files.push_back(file);
            totalFrames += media.frames();
        }
    }

    if (stacker.source.files.empty()) {
        return;
    }

    stacker.config.outputWidth = stacker.source.files[0].dimensions().width;
    stacker.config.outputHeight = stacker.source.files[0].dimensions().height;
    stacker.source.sorted.resize(totalFrames, {0, 0.0});
    for (int i = 0; i < totalFrames; ++i) {
        stacker.source.sorted[i].first = i;
    }

    ui->totalFramesEdit->setText(QString::number(totalFrames));
    ui->frameSlider->setRange(0, totalFrames - 1);
    ui->frameSlider->setValue(0);
    ui->analyzingGroupBox->setEnabled(true);

    updateUI();
    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (stacker.source.files.empty()) {
        return;
    }

    int index = stacker.source.sorted[frameNumber].first;
    cv::Mat frame = getMatAtFrame(stacker.source.files, index);
    if (frame.empty()) {
        return;
    }

    frame = Frame::centerObject(frame, stacker.config.outputWidth, stacker.config.outputHeight);
    display->show(frame);
}

void StackPage::on_estimateAPGridPushButton_clicked() {
    cv::Mat reference = getMatAtFrame(stacker.source.files, stacker.source.sorted[0].first);
    reference = Frame::centerObject(reference, stacker.config.outputWidth, stacker.config.outputHeight);

    int apSize = ui->apSizeSpinBox->value();
    APPlacement placement = ui->featureBasedApPlacement->isChecked() ? APPlacement::FeatureBased : APPlacement::Uniform;
    stacker.config.aps = Frame::getAps(reference, apSize, placement);

    if (stacker.config.aps.empty()) {
        ui->apAmountEdit->setText("<font color='red'>No APs detected!</font>");
    }
    else {
        ui->apAmountEdit->setText(QString::number(stacker.config.aps.size()));
    }

    // Preview alignment points
    cv::Mat preview = reference.clone();
    for (const auto &ap : stacker.config.aps) {
        cv::rectangle(preview, ap.rect(), cv::Scalar(0, 255, 0), 1);
    }
    display->show(preview);
}

void StackPage::on_stackPushButton_clicked() {
    if (stacker.source.files.empty() || stacker.source.sorted.empty()) {
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

            stacker.config.framesToStack = totalFrames * percent / 100;

            QMetaObject::invokeMethod(this, [this, percent]() {
                ui->statusEdit->setText(QString("Stacking %1%").arg(percent));
            });

            cv::Mat stacked = stacker.stack();
            std::string filename = outputDir + "/proxima-stacked-" + std::to_string(percent) + ".tif";
            cv::imwrite(filename, stacked);
        }

        QMetaObject::invokeMethod(this, [this]() {
            ui->statusEdit->setText("Done!");
        });
    }).detach();
}

void StackPage::analyzeFrames() {
    if (stacker.source.files.empty() || stacker.source.sorted.empty()) {
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
            for (int i = 0; i < totalFrames; ++i) {
                pool.enqueue([i, this, framesAnalyzed]() {
                    cv::Mat frame = getMatAtFrame(stacker.source.files, i);
                    stacker.source.sorted[i].first = i;
                    stacker.source.sorted[i].second = Frame::estimateQuality(frame);
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

    connect(ui->widthSpinBox, &QSpinBox::editingFinished, [this]() {
        stacker.config.outputWidth = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->heightSpinBox, &QSpinBox::editingFinished, [this]() {
        stacker.config.outputHeight = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        analyzeFrames();
    });

    connect(this, &StackPage::sortingProgressUpdated, this, [this](int current) {
        ui->analyzingProgressEdit->setText(QString::number(current) + "/" + QString::number(totalFrames));
    });

    connect(this, &StackPage::analyzingCompleted, this, [this]() {
        std::sort(stacker.source.sorted.begin(), stacker.source.sorted.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; }
        );
        enableConfigEdit();
        ui->frameSlider->setValue(0);
        displayFrame(0);
    });

    connect(ui->localAlignmentCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->localAlignmentFrame->setEnabled(state == Qt::Checked);
        stacker.config.localAlign = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->drizzleCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        bool checked = (state == Qt::Checked);

        ui->drizzle1_5xRadioButton->setEnabled(checked);
        ui->drizzle2_0xRadioButton->setEnabled(checked);

        if (!checked) {
            stacker.config.drizzle = 1.0;
        }
        else if (ui->drizzle1_5xRadioButton->isChecked()) {
            stacker.config.drizzle = 1.5;
        }
        else {
            stacker.config.drizzle = 2.0;
        }
    });

    connect(&stacker, &Stacker::progressUpdated, this, [this](QString status) {
        ui->stackingProgressEdit->setText(status);
    });
}

void StackPage::updateUI() {
    ui->widthSpinBox->blockSignals(true);
    ui->widthSpinBox->setValue(stacker.source.files[0].dimensions().width);
    ui->widthSpinBox->blockSignals(false);

    ui->heightSpinBox->blockSignals(true);
    ui->heightSpinBox->setValue(stacker.source.files[0].dimensions().height);
    ui->heightSpinBox->blockSignals(false);
}
