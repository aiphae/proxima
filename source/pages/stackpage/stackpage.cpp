#include "stackpage.h"
#include "ui_stackpage.h"
#include "helpers.h"
#include "frame.h"
#include <QFileDialog>
#include "threadpool.h"

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
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath(), fileFilters());
    if (files.isEmpty()) {
        return;
    }

    totalFrames = 0;
    source.files.clear();
    source.sorted.clear();

    for (const QString &file : files) {
        if (MediaFile media(file); media.isValid()) {
            source.files.emplace_back(file);
            totalFrames += source.files.back().frames();
        }
    }

    if (source.files.empty()) {
        return;
    }

    updateUI();

    config.outputWidth = source.files[0].dimensions().width;
    config.outputHeight = source.files[0].dimensions().height;

    ui->totalFramesEdit->setText(QString::number(totalFrames));
    ui->frameSlider->setMinimum(0);
    ui->frameSlider->setMaximum(totalFrames - 1);
    ui->frameSlider->setValue(0);

    source.sorted.resize(totalFrames);
    for (int i = 0; i < totalFrames; ++i) {
        source.sorted[i] = {i, 0.0};
    }

    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (source.files.empty()) {
        return;
    }

    int index = source.sorted[frameNumber].first;
    cv::Mat frame = getMatAtFrame(source.files, index);
    if (frame.empty()) {
        return;
    }

    frame = Frame::centerObject(frame, config.outputWidth, config.outputHeight);
    display->show(frame);
}

void StackPage::on_estimateAPGridPushButton_clicked() {
    cv::Mat reference = getMatAtFrame(source.files, source.sorted[0].first);
    reference = Frame::centerObject(reference, config.outputWidth, config.outputHeight);

    int apSize = ui->apSixeSpinBox->value();
    APPlacement placement = ui->featureBasedApPlacement->isChecked() ? APPlacement::FeatureBased : APPlacement::Uniform;
    config.aps = Frame::getAps(reference, apSize, placement);

    // Preview alignment points
    cv::Mat preview = reference.clone();
    for (const auto &ap : config.aps) {
        cv::rectangle(preview, ap.rect(), cv::Scalar(0, 255, 0), 1);
    }
    display->show(preview);
}

void StackPage::on_stackPushButton_clicked() {
    if (source.files.empty() || source.sorted.empty()) {
        return;
    }

    // Stack 20% of the best frames
    config.framesToStack = totalFrames * 0.20;

    cv::Mat stacked = Stacker::stack(source, config);

    display->show(stacked);
    cv::imwrite("output.tif", stacked);
}

void StackPage::analyzeFrames() {
    if (source.files.empty() || source.sorted.empty()) {
        return;
    }

    std::shared_ptr<std::atomic<int>> framesAnalyzed = std::make_shared<std::atomic<int>>();

    std::thread([this, framesAnalyzed]() {
        // Producer thread
        {
            ThreadPool pool(std::thread::hardware_concurrency() - 2); // Leave 1 for GUI and 1 for producer
            *framesAnalyzed = 0;
            for (int i = 0; i < totalFrames; ++i) {
                int index = i;
                pool.enqueue([index, this, framesAnalyzed]() {
                    cv::Mat frame = getMatAtFrame(source.files, index);
                    double quality = Frame::estimateQuality(frame);
                    source.sorted[index].first = index;
                    source.sorted[index].second = quality;
                    int done = ++(*framesAnalyzed);
                    emit sortingProgressUpdated(done);
                });
            }
            // Thread pool's destructor is called here,
            // waiting for workers to complete their work
        }
        emit analyzingComplete();
    }).detach();
}

void StackPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    connect(ui->widthSpinBox, &QSpinBox::editingFinished, [this]() {
        config.outputWidth = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->heightSpinBox, &QSpinBox::editingFinished, [this]() {
        config.outputHeight = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        analyzeFrames();
    });

    connect(this, &StackPage::sortingProgressUpdated, this, [this](int current) {
        ui->analyzingProgressEdit->setText(QString::number(current) + "/" + QString::number(totalFrames));
    });

    connect(this, &StackPage::analyzingComplete, this, [this]() {
        std::sort(source.sorted.begin(), source.sorted.end(),
                  [](const auto &a, const auto &b) { return a.second > b.second; }
                  );
        ui->frameSlider->setValue(0);
        displayFrame(0);
    });
}

void StackPage::updateUI() {
    ui->widthSpinBox->blockSignals(true);
    ui->widthSpinBox->setValue(source.files[0].dimensions().width);
    ui->widthSpinBox->blockSignals(false);

    ui->heightSpinBox->blockSignals(true);
    ui->heightSpinBox->setValue(source.files[0].dimensions().height);
    ui->heightSpinBox->blockSignals(false);
}
