#include "preprocesspage.h"
#include "ui_preprocesspage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <opencv2/opencv.hpp>
#include <QSet>
#include "helpers.h"
#include "frame.h"

// Some constants for clarity
namespace {
    const double DEFAULT_OUTPUT_FPS = 30.0;
    const int CODEC_UNCOMPRESSED = 0;
    const double MIN_OBJECT_SIZE_FACTOR = 0.95;
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
    totalFrames = config.cropWidth = config.cropHeight = 0;
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

    displayFrame(0);
}

void AlignPage::displayFrame(const int frameNumber) {
    if (mediaFiles.empty()) {
        return;
    }

    if (cv::Mat frame = getMatAtFrame(mediaFiles, frameNumber); !frame.empty()) {
        display->show(Preprocessor::preview(frame, config));
    }
}

// To maximize the chance that precomputed cropping values are based on a fully visible object,
// make the calculations on a frame that is somewhere before the middle of the whole range of frames.
void AlignPage::estimateParameters() {
    int mediaIndex = mediaFiles.size() / 3;
    int frameIndex = mediaFiles[mediaIndex].isVideo() ? mediaFiles[mediaIndex].frames() / 3 : 0;

    Frame frame(mediaFiles[mediaIndex].matAtFrame(frameIndex));
    cv::Rect object = frame.findObject();

    objectSide = std::max(object.width, object.height);
    config.cropHeight = config.cropWidth = static_cast<int>(objectSide * ui->scaleSpinBox->value());
    config.minObjectSize = static_cast<int>(std::min(object.width, object.height) * MIN_OBJECT_SIZE_FACTOR);
}

void AlignPage::process() {
    const bool joinMode = ui->joinModeRadioButton->isChecked();

    // Check for equality of media files dimensions if combining into one file
    if (mediaFiles.size() > 1 && joinMode && !allDimensionsEqual() && !config.crop) {
        QMessageBox::critical(this,
                              "Processing error",
                              "Unable to process in join mode because some media files have different dimensions. "
                              "Consider applying cropping or using another files.");
        return;
    }

    std::string directory = QFileDialog::getExistingDirectory().toStdString(); // Directory to store output files
    std::variant<std::string, cv::VideoWriter> output; // Output for either image or video
    cv::Size fileDimensions = config.crop
                                  ? cv::Size(config.cropWidth, config.cropHeight)
                                  : mediaFiles[0].dimensions();

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
        // Create a separate output file if processing separately
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
            cv::Mat processed = Preprocessor::process(frame, config);

            writeFrame(processed, output);
        }
    }

    if (std::holds_alternative<cv::VideoWriter>(output)) {
        std::get<cv::VideoWriter>(output).release();
    }
}

// Checks if all media files have equal dimensions (width and height).
// Necessary before combining into a single file.
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
        config.cropWidth = config.cropHeight = objectSide * ui->scaleSpinBox->value();
        updateUI();
        displayFrame(currentFrame);
    });

    connect(ui->cropXSpinBox, &QSpinBox::valueChanged, this, [this]() {
        config.cropWidth = ui->cropXSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->cropYSpinBox, &QSpinBox::valueChanged, this, [this]() {
        config.cropHeight = ui->cropYSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->minObjectSizeSpinBox, &QSpinBox::valueChanged, this, [this]() {
        config.minObjectSize = ui->minObjectSizeSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->rejectFramesCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->objectDetectionFrame->setEnabled(state == Qt::Checked);
        config.rejectFrames = (state == Qt::Checked);
        if (!config.minObjectSize) {
            estimateParameters();
            updateUI();
        }
        displayFrame(currentFrame);
    });

    connect(ui->cropCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        ui->cropValuesFrame->setEnabled(state == Qt::Checked);
        config.crop = (state == Qt::Checked);
        if (!config.cropWidth || !config.cropHeight) {
            estimateParameters();
            updateUI();
        }
        displayFrame(currentFrame);
    });

    connect(ui->monochromeCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        config.toMonochrome = (state == Qt::Checked);
        displayFrame(currentFrame);
    });

    connect(ui->doProcessingPushButton, &QPushButton::clicked, this, &AlignPage::process);
}

// Updates spin boxes without triggering their signals.
void AlignPage::updateUI() {
    ui->cropXSpinBox->blockSignals(true);
    ui->cropYSpinBox->blockSignals(true);
    ui->cropXSpinBox->setValue(config.cropWidth);
    ui->cropYSpinBox->setValue(config.cropHeight);
    ui->cropXSpinBox->blockSignals(false);
    ui->cropYSpinBox->blockSignals(false);

    ui->minObjectSizeSpinBox->blockSignals(true);
    ui->minObjectSizeSpinBox->setValue(config.minObjectSize);
    ui->minObjectSizeSpinBox->blockSignals(false);
}
