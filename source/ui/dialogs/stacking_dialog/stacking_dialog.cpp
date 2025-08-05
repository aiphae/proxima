#include "stacking_dialog.h"
#include "ui_stacking_dialog.h"
#include "components/frame.h"
#include "boost/asio/thread_pool.hpp"
#include "boost/asio/post.hpp"

StackingDialog::StackingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StackingDialog)
{
    ui->setupUi(this);

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, &StackingDialog::analyzeFiles);
}

StackingDialog::~StackingDialog() {
    delete ui;
}

void StackingDialog::includeFile(MediaFile *file, bool flag) {
    if (flag) {
        files.addFile(file);
    }
    else {
        files.removeFile(file);
    }
    ui->selectedFilesEdit->setText(QString::number(files.fileCount()));
    ui->totalFramesEdit->setText(QString::number(files.totalFrames()));

    if (files.fileCount() > 0) {
        ui->analyzeGroupBox->setEnabled(true);
    }
}

void StackingDialog::analyzeFiles() {
    frameQualities.clear();
    frameQualities.resize(files.totalFrames());

    enableStackingOptions(false);

    // Background thread is needed to not block the main GUI
    std::thread([this] {
        // '- 1' for the outer thread
        asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

        // Progress counter
        auto counter = std::make_shared<std::atomic<int>>(0);

        for (int i = 0; i < files.totalFrames(); ++i) {
            asio::post(pool, [this, counter, i]() {
                cv::Mat frame = files.matAtFrame(i);
                frameQualities[i].first = i;
                frameQualities[i].second = Frame::estimateQuality(frame);

                // Update progress
                QMetaObject::invokeMethod(this, [this, counter] {
                    ui->analyzingProgressEdit->setText(QString::number(++(*counter)) + "/" + QString::number(files.totalFrames()));
                });
            });
        }

        pool.join();

        std::stable_sort(frameQualities.begin(), frameQualities.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        QMetaObject::invokeMethod(this, [this] {
            ui->analyzingProgressEdit->setText("Completed!");
            enableStackingOptions(true);
        });
    }).detach();
}

void StackingDialog::enableStackingOptions(bool flag) {
    ui->alignmentGroupBox->setEnabled(flag);
    ui->optionsGroupBox->setEnabled(flag);
    ui->stackGroupBox->setEnabled(flag);
    ui->outputGroupBox->setEnabled(flag);
}
