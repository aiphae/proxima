#include "stackpage.h"
#include "ui_stackpage.h"
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
    ui->frameSlider->setMaximum(totalFrames - 1);  // Fixed: subtract 1 since we start at 0
    ui->frameSlider->setValue(0);

    sortedFrames.clear();
    for (int i = 0; i < totalFrames; ++i) {
        sortedFrames.emplace_back(i, 0.0);  // Initialize with original frame index
    }

    displayFrame(0);
}

void StackPage::displayFrame(const int frameNumber) {
    if (mediaFiles.empty() || frameNumber < 0 || frameNumber >= totalFrames) {
        return;
    }

    // Use the original frame index from sortedFrames
    int originalFrameIdx = sortedFrames[frameNumber].first;
    auto [mediaIndex, localFrame] = findMediaFrame(originalFrameIdx);
    if (mediaIndex >= mediaFiles.size()) {
        return;
    }

    if (cv::Mat frame = mediaFiles[mediaIndex].matAtFrame(localFrame); !frame.empty()) {
        display->show(frame);
    }
}

std::tuple<int, int> StackPage::findMediaFrame(const int frameNumber) {
    int currentFrame = 0;
    for (int i = 0; i < mediaFiles.size(); ++i) {
        if (frameNumber < currentFrame + mediaFiles[i].frames()) {
            return {i, frameNumber - currentFrame};
        }
        currentFrame += mediaFiles[i].frames();
    }
    return {mediaFiles.size(), 0};
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

        // Calculate quality scores for all frames
        for (int frameIdx = 0; frameIdx < totalFrames; ++frameIdx) {
            auto [mediaIndex, localFrame] = findMediaFrame(frameIdx);
            if (mediaIndex >= mediaFiles.size()) {
                continue;
            }

            cv::Mat mat = mediaFiles[mediaIndex].matAtFrame(localFrame);
            if (!mat.empty()) {
                double qualityScore = calculateFrameQuality(mat);
                sortedFrames[frameIdx].second = qualityScore;
            }
        }

        // Sort while preserving original indices
        std::stable_sort(sortedFrames.begin(), sortedFrames.end(),
                         [](const auto& a, const auto& b) {
                             return a.second > b.second;  // Higher quality first
                         });

        // Update UI
        ui->frameSlider->setMinimum(0);
        ui->frameSlider->setMaximum(totalFrames - 1);
        ui->frameSlider->setValue(0);
        displayFrame(0);
    });
}

double StackPage::calculateFrameQuality(const cv::Mat& mat) {
    cv::Mat gray, lap;
    if (mat.channels() > 1) {
        cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = mat.clone();
    }

    cv::Laplacian(gray, lap, CV_64F);
    cv::Scalar mu, sigma;
    cv::meanStdDev(lap, mu, sigma);
    return sigma[0] * sigma[0];  // Return variance as quality metric
}

QString StackPage::fileFilters() {
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
