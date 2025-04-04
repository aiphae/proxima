#include "alignpage.h"
#include "ui_alignpage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <opencv2/opencv.hpp>
#include <QSet>

// Some constants for clarity
namespace {
    const double DEFAULT_OUTPUT_FPS = 30.0;
    const int CODEC_UNCOMPRESSED = 0;
    const double MIN_OBJECT_SIZE_FACTOR = 0.95;

    const cv::Scalar COLOR_GREEN(0, 255, 0);
    const cv::Scalar COLOR_BLUE(255, 0, 0);
}

AlignPage::AlignPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlignPage)
{
    ui->setupUi(this);
    display = std::make_unique<Display>(ui->imageDisplay);
    connectUI();
}

AlignPage::~AlignPage() {
    delete ui;
}

void AlignPage::on_openFilesPushButton_clicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath(), fileFilters());
    if (files.isEmpty()) {
        return;
    }

    selectedFiles = files;
    totalFrames = processingConfig.cropWidth = processingConfig.cropHeight = 0;
    mediaFiles.clear();

    for (const QString &file : files) {
        if (MediaFile media(file); media.isValid()) {
            mediaFiles.emplace_back(std::move(media));
            totalFrames += mediaFiles.back().frames();
        }
    }

    ui->totalFramesEdit->setText(QString::number(totalFrames));
    ui->frameSlider->setMinimum(0);
    ui->frameSlider->setMaximum(totalFrames);
    ui->frameSlider->setValue(0);

    displayFrame(0);
}

void AlignPage::displayFrame(const int frameNumber) {
    if (mediaFiles.empty()) {
        return;
    }

    auto [mediaIndex, localFrame] = findMediaFrame(frameNumber);
    if (mediaIndex >= mediaFiles.size()) {
        return;
    }

    if (cv::Mat frame = mediaFiles[mediaIndex].matAtFrame(localFrame); !frame.empty()) {
        display->show(previewProcessing(frame));
    }
}

std::tuple<int, int> AlignPage::findMediaFrame(const int frameNumber) {
    int currentFrame = 0;
    for (int i = 0; i < mediaFiles.size(); ++i) {
        if (frameNumber < currentFrame + mediaFiles[i].frames()) {
            return {i, frameNumber - currentFrame};
        }
        currentFrame += mediaFiles[i].frames();
    }
    return {mediaFiles.size(), 0};
}

cv::Mat AlignPage::previewProcessing(cv::Mat &frame) {
    cv::Mat processed = processFrame(frame);
    if (processingConfig.crop) {
        previewCrop(processed);
    }
    if (processingConfig.rejectFrames) {
        previewObject(processed);
    }
    return processed;
}

void AlignPage::previewCrop(const cv::Mat &frame) {
    if (currentObject.x <= 0 || currentObject.y <= 0) {
        return;
    }

    cv::rectangle(frame, currentCrop, COLOR_GREEN);

    cv::Point cropTextPos(currentCrop.x, currentCrop.y - 5);
    cv::putText(frame, "Crop", cropTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR_GREEN);

    cv::Point cropCenter(currentObject.x + currentObject.width / 2, currentObject.y + currentObject.height / 2);
    cv::drawMarker(frame, cropCenter, COLOR_GREEN, cv::MARKER_CROSS, 15, 1);
}

void AlignPage::previewObject(const cv::Mat &frame) {
    if (currentObject.x <= 0 || currentObject.y <= 0) {
        return;
    }

    cv::rectangle(frame, currentObject, COLOR_BLUE);

    cv::Point objectTextPos(currentObject.x, currentObject.y - 5);
    cv::putText(frame, "Object", objectTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR_BLUE);

    cv::Point objectCenter(currentObject.x + currentObject.width / 2, currentObject.y + currentObject.height / 2);
    cv::drawMarker(frame, objectCenter, COLOR_BLUE, cv::MARKER_CROSS, 10, 1);
}

cv::Mat AlignPage::processFrame(cv::Mat &frame) {
    cv::Mat processedFrame = frame.clone();

    if (processingConfig.toMonochrome) {
        cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2GRAY);
        cv::cvtColor(processedFrame, processedFrame, cv::COLOR_GRAY2BGR);
    }

    if (processingConfig.rejectFrames || processingConfig.crop) {
        currentObject = findObject(frame);

        if (!processingConfig.minObjectSize || !processingConfig.cropWidth || !processingConfig.cropHeight) {
            estimateParameters();
            updateUI();
        }

        if (processingConfig.crop) {
            currentCrop = getCrop(frame, currentObject);
        }
    }

    return processedFrame;
}

cv::Rect AlignPage::getCrop(const cv::Mat &frame, const cv::Rect &object) {
    cv::Point center = (object.br() + object.tl()) / 2;
    cv::Point tl(center.x - processingConfig.cropWidth / 2, center.y - processingConfig.cropHeight / 2);
    cv::Point br(center.x + processingConfig.cropWidth / 2, center.y + processingConfig.cropHeight / 2);
    return cv::Rect(tl, br);
}

// To maximize the chance that precomputed cropping values are based on a fully visible object,
// make the calculations on a frame that is somewhere before the middle of the whole range of frames
void AlignPage::estimateParameters() {
    int mediaIndex = mediaFiles.size() / 3;
    int frameIndex = mediaFiles[mediaIndex].isVideo() ? mediaFiles[mediaIndex].frames() / 3 : 0;

    cv::Rect object = findObject(mediaFiles[mediaIndex].matAtFrame(frameIndex));

    // Default cropping is always a square
    objectSide = std::max(object.width, object.height);
    double scale = ui->scaleSpinBox->value();
    processingConfig.cropHeight = processingConfig.cropWidth = static_cast<int>(objectSide * scale);

    // Set the minimum object size as 95% of possible fully visible object
    processingConfig.minObjectSize = static_cast<int>(std::min(object.width, object.height) * MIN_OBJECT_SIZE_FACTOR);
}

cv::Rect AlignPage::findObject(const cv::Mat &frame) {
    cv::Mat proccessed = frame.clone();

    cv::cvtColor(proccessed, proccessed, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(proccessed, proccessed, cv::Size(5, 5), 0);
    cv::threshold(proccessed, proccessed, 0, 255, cv::THRESH_OTSU | cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(proccessed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return cv::Rect();
    }

    std::vector<cv::Point> allPoints;
    for (const auto &contour : contours) {
        allPoints.insert(allPoints.end(), contour.begin(), contour.end());
    }

    cv::Rect boundingRect;
    if (!allPoints.empty()) {
        boundingRect = cv::boundingRect(allPoints);
        if (std::min(boundingRect.width, boundingRect.height) < processingConfig.minObjectSize) {
            boundingRect = cv::Rect();
        }
    }

    return boundingRect;
}

void AlignPage::process() {
    // Check for equality of media files dimensions if combining into one file
    if (mediaFiles.size() > 1 && processingConfig.joinMode && !allDimensionsEqual() && !processingConfig.crop) {
        QMessageBox::critical(this,
                              "Processing error",
                              "Unable to process in join mode because some media files have different dimensions. "
                              "Consider applying cropping or using another files.");
        return;
    }

    // Directory to store output files
    std::string directory = QFileDialog::getExistingDirectory().toStdString();

    cv::Size fileDimensions = processingConfig.crop
                                  ? cv::Size(processingConfig.cropWidth, processingConfig.cropHeight)
                                  : mediaFiles[0].dimensions();
    std::variant<std::string, cv::VideoWriter> output;

    // Lambdas for convenience
    auto constructVideoWriter = [&fileDimensions](const std::string &filename) {
        return cv::VideoWriter(filename, CODEC_UNCOMPRESSED, DEFAULT_OUTPUT_FPS, fileDimensions, true);
    };
    auto writeFrame = [](cv::Mat &frame, std::variant<std::string, cv::VideoWriter> &output) {
        if (std::holds_alternative<std::string>(output)) {
            cv::imwrite(std::get<std::string>(output), frame);
        }
        else {
            std::get<cv::VideoWriter>(output).write(frame);
        }
    };

    const bool joinMode = processingConfig.joinMode;

    // Create a single .avi output file if join mode is selected
    if (joinMode) {
        std::string filename = "proxima-aligned-" + QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm-ss").toStdString() + ".avi";
        filename = directory + '/' + filename;
        output = constructVideoWriter(filename);
        if (!std::get<cv::VideoWriter>(output).isOpened()) {
            QMessageBox::critical(this, "File error", "Could not create the file: " + QString::fromStdString(filename));
            return;
        }
    }

    int counter = 1; // Counter of processed frames
    for (auto &media : mediaFiles) {
        // Create a separate output file if join mode is not selected
        if (!joinMode) {
            std::string extension = media.isVideo() ? ".avi" : ".tif";
            std::string filename = "proxima-aligned-" + media.filename() + extension;
            filename = directory + '/' + filename;
            if (media.isVideo()) {
                output = constructVideoWriter(filename);
            }
            else {
                output = filename;
            }
        }

        for (int i = 0; i < media.frames(); ++i) {
            // Update UI
            ui->currentProcessingEdit->setText(QString::number(counter++) + '/' + QString::number(totalFrames));
            QCoreApplication::processEvents(); // TEMPORARY

            cv::Mat frame = media.matAtFrame(i);
            cv::Mat processed = processFrame(frame);

            // Check if current frame has no detected object
            if (processingConfig.rejectFrames && currentObject.width < 1) {
                continue;
            }

            // Check if crop is not enabled
            if (!processingConfig.crop) {
                writeFrame(processed, output);
                continue;
            }

            // Cropping rectangle that fits into frame
            cv::Rect alignedCrop = currentCrop & cv::Rect(0, 0, processed.cols, processed.rows);
            // Check if 'alignedCrop' is smaller than required
            if (alignedCrop.width != processingConfig.cropWidth || alignedCrop.height != processingConfig.cropHeight) {
                // Fill the out-of-bounds region of currentCrop with black
                cv::Mat black = cv::Mat::zeros(cv::Size(processingConfig.cropWidth, processingConfig.cropHeight), processed.type());
                auto imageRect = cv::Rect({}, processed.size());
                auto intersection = imageRect & currentCrop;
                auto interROI = intersection - currentCrop.tl();
                processed(intersection).copyTo(black(interROI));
                processed = black;
            }
            else {
                processed = processed(currentCrop);
            }

            writeFrame(processed, output);
        }
    }

    if (std::holds_alternative<cv::VideoWriter>(output)) {
        std::get<cv::VideoWriter>(output).release();
    }
}

bool AlignPage::allDimensionsEqual() const {
    auto dimensions = mediaFiles[0].dimensions();
    for (int i = 1; i < mediaFiles.size(); ++i) {
        if (mediaFiles[i].dimensions() != dimensions) {
            return false;
        }
    }
    return true;
}

void AlignPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this]() {
        currentFrame = ui->frameSlider->value();
        displayFrame(currentFrame);
    });

    connect(ui->scaleSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {
        processingConfig.cropWidth = processingConfig.cropHeight = objectSide * ui->scaleSpinBox->value();
        updateUI();
        displayFrame(currentFrame);
    });

    connect(ui->cropXSpinBox, &QSpinBox::valueChanged, this, [this]() {
        processingConfig.cropWidth = ui->cropXSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->cropYSpinBox, &QSpinBox::valueChanged, this, [this]() {
        processingConfig.cropHeight = ui->cropYSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->minObjectSizeSpinBox, &QSpinBox::valueChanged, this, [this]() {
        processingConfig.minObjectSize = ui->minObjectSizeSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->rejectFramesCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->objectDetectionFrame->setEnabled(state == Qt::Checked);
        processingConfig.rejectFrames = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->cropCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->cropValuesFrame->setEnabled(state == Qt::Checked);
        processingConfig.crop = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->monochromeCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        processingConfig.toMonochrome = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->joinModeRadioButton, &QRadioButton::clicked, this, [this]() {
        processingConfig.joinMode = ui->joinModeRadioButton->isChecked();
    });

    connect(ui->batchModeRadioButton, &QRadioButton::clicked, this, [this]() {
        processingConfig.joinMode = !ui->batchModeRadioButton->isChecked();
    });

    connect(ui->doProcessingPushButton, &QPushButton::clicked, this, &AlignPage::process);
}

void AlignPage::updateUI() {
    ui->cropXSpinBox->blockSignals(true);
    ui->cropYSpinBox->blockSignals(true);
    ui->cropXSpinBox->setValue(processingConfig.cropWidth);
    ui->cropYSpinBox->setValue(processingConfig.cropHeight);
    ui->cropXSpinBox->blockSignals(false);
    ui->cropYSpinBox->blockSignals(false);

    ui->minObjectSizeSpinBox->blockSignals(true);
    ui->minObjectSizeSpinBox->setValue(processingConfig.minObjectSize);
    ui->minObjectSizeSpinBox->blockSignals(false);
}

QString AlignPage::fileFilters() {
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
