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

    for (const QString &file : files) {
        if (MediaFile media(file); media.isValid()) {
            mediaFiles.emplace_back(std::move(media));
            totalFrames += mediaFiles.back().frames();
        }
    }

    ui->totalFramesEdit->setText(QString::number(totalFrames));
    ui->frameSlider->setMinimum(0);
    ui->frameSlider->setMaximum(totalFrames - 1);
    ui->frameSlider->setValue(0);

    sortedFrames.clear();
    for (int i = 0; i < totalFrames; ++i) {
        sortedFrames.emplace_back(i, 0.0);
    }

    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (mediaFiles.empty()) {
        return;
    }

    int index = sortedFrames[frameNumber].first;
    auto [mediaIndex, localFrame] = findMediaFrame(mediaFiles, index);

    if (cv::Mat frame = mediaFiles[mediaIndex].matAtFrame(localFrame); !frame.empty()) {
        display->show(frame);
    }
}

void StackPage::on_estimateAPGridPushButton_clicked() {

}

void StackPage::on_stackPushButton_clicked() {
    if (sortedFrames.empty()) {
        return;
    }

    auto [media, index] = findMediaFrame(mediaFiles, sortedFrames[0].first);
    stacker.reference = mediaFiles[media].matAtFrame(index);

    std::vector<cv::Mat> frames;
    for (int i = 0; i < 0.2 * totalFrames; ++i) {
        auto [media, index] = findMediaFrame(mediaFiles, i);
        frames.emplace_back(mediaFiles[media].matAtFrame(index));
    }

    cv::Mat stacked = stacker.stack(frames);
    display->show(stacked);
    cv::imwrite("output.tif", stacked);
}

void StackPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        if (mediaFiles.empty()) {
            return;
        }

        for (int frame = 0; frame < totalFrames; ++frame) {
            auto [mediaIndex, localFrame] = findMediaFrame(mediaFiles, frame);
            cv::Mat mat = mediaFiles[mediaIndex].matAtFrame(localFrame);
            if (!mat.empty()) {
                sortedFrames[frame].second = Frame(mat).estimateQuality();;
            }
        }

        std::stable_sort(sortedFrames.begin(), sortedFrames.end(),
                         [](const auto& a, const auto& b) {
                             return a.second > b.second;
                         });

        ui->frameSlider->setMinimum(0);
        ui->frameSlider->setMaximum(totalFrames - 1);
        ui->frameSlider->setValue(0);

        displayFrame(0);
    });
}
