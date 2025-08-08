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

    connect(ui->analyzeFramesPushButton, &QPushButton::clicked, this, &StackingDialog::_analyzeFiles);
}

StackingDialog::~StackingDialog() {
    delete ui;
}

void StackingDialog::includeFile(MediaFile *file, bool flag) {
    if (flag) {
        _files.addFile(file);
    }
    else {
        _files.removeFile(file);
    }
    ui->selectedFilesEdit->setText(QString::number(_files.fileCount()));
    ui->totalFramesEdit->setText(QString::number(_files.totalFrames()));

    if (_files.fileCount() > 0) {
        ui->analyzeGroupBox->setEnabled(true);
    }
}

void StackingDialog::_analyzeFiles() {
    _frameQualities.clear();
    _frameQualities.resize(_files.totalFrames());

    // Block UI while processing
    _enableStackingOptions(false);
    this->setEnabled(false);

    // Background thread is needed to not block the main GUI
    std::thread([this] {
        // Save one thread for the GUI, and one for the outer thread
        asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

        // Progress counter
        auto counter = std::make_shared<std::atomic<int>>(0);

        for (int i = 0; i < _files.totalFrames(); ++i) {
            asio::post(pool, [this, counter, i]() {
                cv::Mat frame = _files.matAtFrame(i);
                _frameQualities[i].first = i;
                _frameQualities[i].second = Frame::estimateQuality(frame);

                // Update progress from the MAIN thread
                QMetaObject::invokeMethod(this, [this, counter] {
                    ui->analyzingProgressEdit->setText(QString::number(++(*counter)) + "/" + QString::number(_files.totalFrames()));
                });
            });
        }

        // Wait until all frames are processed
        pool.join();

        // Sort frames by quality (pair.second)
        std::stable_sort(_frameQualities.begin(), _frameQualities.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        for (const auto &[id, quality] : _frameQualities) {
            qDebug() << id << quality;
        }

        // Extract only indices from the vector of pairs
        std::vector<int> map;
        map.reserve(_frameQualities.size());
        for (const auto &[id, quality] : _frameQualities) {
            map.push_back(id);
        }

        // When finished, invoke this from the MAIN thread:
        QMetaObject::invokeMethod(this, [this, map] {
            ui->analyzingProgressEdit->setText("Completed!");
            _enableStackingOptions(true);
            this->setEnabled(true);

            // Send a signal to the MainWindow to update the GUI
            emit analyzeFinished(&_files, map);
        });
    }).detach(); // Detach to keep the app responsive while processing
}

void StackingDialog::_enableStackingOptions(bool flag) {
    ui->alignmentGroupBox->setEnabled(flag);
    ui->optionsGroupBox->setEnabled(flag);
    ui->stackGroupBox->setEnabled(flag);
    ui->outputGroupBox->setEnabled(flag);
}
