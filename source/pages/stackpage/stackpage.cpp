#include "stackpage.h"
#include "ui_stackpage.h"
#include <QFileDialog>
#include "helpers.h"
#include "frame.h"

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
    mediaFiles.clear();
    sortedFrames.clear();

    for (const QString &file : files) {
        if (MediaFile media(file); media.isValid()) {
            mediaFiles.emplace_back(std::move(media));
            totalFrames += mediaFiles.back().frames();
        }
    }

    if (mediaFiles.empty()) {
        return;
    }

    updateUI();

    stacker.width = mediaFiles[0].dimensions().width;
    stacker.height = mediaFiles[0].dimensions().height;

    ui->totalFramesEdit->setText(QString::number(totalFrames));
    ui->frameSlider->setMinimum(0);
    ui->frameSlider->setMaximum(totalFrames - 1);
    ui->frameSlider->setValue(0);

    for (int i = 0; i < totalFrames; ++i) {
        sortedFrames.emplace_back(i, 0.0);
    }

    stacker.files = mediaFiles;

    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (mediaFiles.empty()) {
        return;
    }

    int index = sortedFrames[frameNumber].first;
    cv::Mat frame = getMatAtFrame(mediaFiles, index);
    if (frame.empty()) {
        return;
    }

    frame = Frame::centerObject(frame, stacker.width, stacker.height);
    display->show(frame);
}

void StackPage::on_estimateAPGridPushButton_clicked() {
    cv::Mat reference = getMatAtFrame(mediaFiles, sortedFrames[0].first);
    reference = Frame::centerObject(reference, stacker.width, stacker.height);
    stacker.reference = reference;

    int apSize = ui->apSixeSpinBox->value();
    stacker.aps = Frame::getAps(reference, apSize);

    // Preview alignment points
    cv::Mat preview = reference.clone();
    for (const auto &ap : stacker.aps) {
        cv::rectangle(preview, ap.rect(), cv::Scalar(0, 255, 0), 1);
    }
    display->show(preview);
}

void StackPage::on_stackPushButton_clicked() {
    if (mediaFiles.empty() || sortedFrames.empty()) {
        return;
    }

    // Stack 20% of the best frames
    stacker.framesToStack = totalFrames * 0.20;

    cv::Mat stacked = stacker.stack();

    display->show(stacked);
    cv::imwrite("output.tif", stacked);
}

void StackPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    connect(ui->widthSpinBox, &QSpinBox::editingFinished, [this]() {
        stacker.width = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->heightSpinBox, &QSpinBox::editingFinished, [this]() {
        stacker.height = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        if (mediaFiles.empty()) {
            return;
        }

        for (int frame = 0; frame < totalFrames; ++frame) {
            cv::Mat mat = getMatAtFrame(mediaFiles, frame);
            sortedFrames[frame].second = mat.empty() ? 0.0 : Frame::estimateQuality(mat);

            ui->analyzingProgressEdit->setText(QString::number(frame + 1) + "/" + QString::number(totalFrames));
            QCoreApplication::processEvents();
        }

        std::stable_sort(sortedFrames.begin(), sortedFrames.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        stacker.reference = getMatAtFrame(mediaFiles, sortedFrames[0].first);
        stacker.sorted = sortedFrames;

        ui->frameSlider->setValue(0);
        displayFrame(0);
    });
}

void StackPage::updateUI() {
    ui->widthSpinBox->blockSignals(true);
    ui->widthSpinBox->setValue(mediaFiles[0].dimensions().width);
    ui->widthSpinBox->blockSignals(false);

    ui->heightSpinBox->blockSignals(true);
    ui->heightSpinBox->setValue(mediaFiles[0].dimensions().height);
    ui->heightSpinBox->blockSignals(false);
}
