#include "alignpage.h"
#include "ui_alignpage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <opencv2/opencv.hpp>

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
    QStringList files = QFileDialog::getOpenFileNames(this, QDir::homePath());
    if (files.isEmpty()) {
        return;
    }
    selectedFiles = files;

    totalFrames = processingConfig.cropWidth = processingConfig.cropHeight = 0;
    mediaFiles.clear();

    for (const QString &file : files) {
        MediaFile media(file);
        if (!media.isValid()) continue;
        mediaFiles.push_back(media);
        totalFrames += mediaFiles.back().frames();
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

    int currentFrame = 0;
    // Iterate through media files to find which one contains the requested frame
    for (auto &media : mediaFiles) {
        if (frameNumber < currentFrame + media.frames()) {
            int localFrame = frameNumber - currentFrame;
            cv::Mat frame = media.matAtFrame(localFrame);
            if (!frame.empty()) {
                display->show(previewProcessing(frame));
            }
            return;
        }
        currentFrame += media.frames();
    }
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
    if (currentObject.x && currentObject.y) {
        cv::rectangle(frame, currentCrop, cv::Scalar(0, 255, 0));

        cv::Point cropTextPos(currentCrop.x, currentCrop.y - 5);
        cv::putText(frame, "Crop", cropTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0));

        cv::Point cropCenter(currentObject.x + currentObject.width / 2, currentObject.y + currentObject.height / 2);
        cv::drawMarker(frame, cropCenter, cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 15, 1);
    }
}

void AlignPage::previewObject(const cv::Mat &frame) {
    if (currentObject.x && currentObject.y) {
        cv::rectangle(frame, currentObject, cv::Scalar(0, 0, 255));

        cv::Point objectTextPos(currentObject.x, currentObject.y - 5);
        cv::putText(frame, "Object", objectTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));

        cv::Point objectCenter(currentObject.x + currentObject.width / 2, currentObject.y + currentObject.height / 2);
        cv::drawMarker(frame, objectCenter, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 10, 1);
    }
}

cv::Mat AlignPage::processFrame(cv::Mat &frame) {
    cv::Mat processedFrame = frame.clone();

    // Convert to monochrome and then back to keep the image 3-channel
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
    // Make a cropping rectangle around 'object'
    cv::Point center = (object.br() + object.tl()) / 2;
    cv::Point tl(center.x - processingConfig.cropWidth / 2, center.y - processingConfig.cropHeight / 2);
    cv::Point br(center.x + processingConfig.cropWidth / 2, center.y + processingConfig.cropHeight / 2);
    cv::Rect crop(tl, br);
    return crop;
}

void AlignPage::estimateParameters() {
    // To maximize the chance that precomputed cropping values are based on a fully visible object,
    // make the calculations on a frame that is somewhere before the middle of the whole range of frames
    int mediaIndex = mediaFiles.size() / 3;
    int frameIndex = mediaFiles[mediaIndex].isVideo() ? mediaFiles[mediaIndex].frames() / 3 : 1;

    cv::Rect object = findObject(mediaFiles[mediaIndex].matAtFrame(frameIndex));

    // Default cropping is always a square
    objectSide = std::max(object.width, object.height);
    processingConfig.cropHeight = processingConfig.cropWidth = objectSide * ui->scaleSpinBox->value();

    // Set the minimum object size as 95% of possible fully visible object
    processingConfig.minObjectSize = std::min(object.width, object.height) * 0.95;
}

cv::Rect AlignPage::findObject(const cv::Mat &frame) {
    cv::Mat proccessed = frame.clone();

    cv::cvtColor(proccessed, proccessed, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(proccessed, proccessed, cv::Size(5, 5), 0);
    cv::threshold(proccessed, proccessed, 0, 255, cv::THRESH_OTSU | cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(proccessed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

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
                              "Unable to process in join mode because some media files have different dimensions."
                              "Consider applying cropping or using another files.");
        return;
    }

    cv::Size fileDimensions = processingConfig.crop
                                  ? cv::Size(processingConfig.cropWidth, processingConfig.cropHeight)
                                  : mediaFiles[0].dimensions();

    cv::VideoWriter videoOutput;
    QString filename;

    if (processingConfig.joinMode) {
        filename = "proxima-aligned-" + QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm-ss") + ".avi";
        videoOutput = cv::VideoWriter(filename.toStdString(),
                                 0, // Codec. '0' for uncompressed
                                 30.0, // FPS
                                 fileDimensions, // Dimensions
                                 true); // Keep color

        // Check for errors
        if (!videoOutput.isOpened()) {
            QMessageBox::critical(this,
                                  "Processing error",
                                  "Could not create the output file.");
            return;
        }
    }

    auto writeFrame = [](bool toVideo, cv::VideoWriter &video, std::string filename, cv::Mat &frame) {
        if (toVideo) {
            video.write(frame);
        }
        else {
            cv::imwrite(filename, frame);
        }
    };

    int counter = 1; // Counter of processed frames
    for (auto &media : mediaFiles) {
        if (!processingConfig.joinMode) {
            QString extension = media.isVideo() ? ".avi" : ".tif";
            filename = "proxima-aligned-" + QString::fromStdString(media.filename()) + extension;
            if (media.isVideo()) {
                videoOutput = cv::VideoWriter(filename.toStdString(),
                                         0, // Codec. '0' for uncompressed
                                         30.0, // FPS
                                         fileDimensions, // Dimensions
                                         true); // Keep color
            }
        }

        for (int i = 0; i < media.frames(); ++i) {
            // Update UI
            ui->currentProcessingEdit->setText(QString::number(counter++) + '/' + QString::number(totalFrames));
            // Call 'repaint' to update the QLabel correctly
            ui->currentProcessingEdit->repaint();

            cv::Mat frame = media.matAtFrame(i);
            cv::Mat processed = processFrame(frame);

            // Check if current frame has no detected object
            if (processingConfig.rejectFrames && currentObject.width < 1) {
                continue;
            }

            // Check if crop is not enabled
            if (!processingConfig.crop) {
                writeFrame(processingConfig.joinMode, videoOutput, filename.toStdString(), processed);
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

            writeFrame(processingConfig.joinMode, videoOutput, filename.toStdString(), processed);
        }
    }

    videoOutput.release();
}

bool AlignPage::allDimensionsEqual() {
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
