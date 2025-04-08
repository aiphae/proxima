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

    ui->widthSpinBox->blockSignals(true);
    ui->heightSpinBox->blockSignals(true);

    ui->widthSpinBox->setValue(mediaFiles[0].dimensions().width);
    config.width = mediaFiles[0].dimensions().width;
    ui->heightSpinBox->setValue(mediaFiles[0].dimensions().height);
    config.height = mediaFiles[0].dimensions().height;

    ui->widthSpinBox->blockSignals(false);
    ui->heightSpinBox->blockSignals(false);

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
    if (cv::Mat frame = getMatAtFrame(mediaFiles, index); !frame.empty()) {
        Frame _frame(frame);
        int width = ui->widthSpinBox->value();
        int height = ui->heightSpinBox->value();
        cv::Mat crop = _frame.cropOnObject(width, height);
        display->show(crop);
    }
}

void StackPage::on_estimateAPGridPushButton_clicked() {

}

void StackPage::on_stackPushButton_clicked() {
    if (sortedFrames.empty()) {
        return;
    }

    Frame reference(getMatAtFrame(mediaFiles, sortedFrames[0].first));
    cv::Mat referenceFrame = reference.crop(reference.getObjectCrop(reference.findObject(), config.width, config.height));

    StackingSource source{referenceFrame, mediaFiles, sortedFrames};
    config.frames = totalFrames * 0.2;

    cv::Mat stacked = stacker.stack(source, config);
    display->show(stacked);
    cv::imwrite("output.tif", stacked);
}

void StackPage::connectUI() {
    connect(ui->frameSlider, &QSlider::valueChanged, this, [this](int value) {
        currentFrame = value;
        displayFrame(currentFrame);
    });

    connect(ui->widthSpinBox, &QSpinBox::editingFinished, [this]() {
        config.width = ui->widthSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->heightSpinBox, &QSpinBox::editingFinished, [this]() {
        config.height = ui->heightSpinBox->value();
        displayFrame(currentFrame);
    });

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, [this]() {
        if (mediaFiles.empty()) {
            return;
        }

        for (int frame = 0; frame < totalFrames; ++frame) {
            cv::Mat mat = getMatAtFrame(mediaFiles, frame);
            if (!mat.empty()) {
                sortedFrames[frame].second = Frame(mat).estimateQuality();
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
