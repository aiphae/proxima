#include "threading/stack_thread.h"
#include "boost/asio/thread_pool.hpp"
#include "boost/asio/post.hpp"
#include <QDateTime>

StackThread::StackThread(
    MediaCollection &files,
    StackConfig &config,
    std::array<int, 4> &percentages,
    std::vector<std::pair<int, double>> &frameQualities,
    std::string &outputDir,
    QObject *parent
) : Thread(parent), files_(files), config_(config),
    percentages_(percentages), frameQualities_(frameQualities), outputDir_(outputDir)
{}

void StackThread::run() {
    for (int i = 0; i < percentages_.size(); ++i) {
        if (percentages_[i] < 1) {
            continue;
        }

        asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

        stacker_.initialize(files_.matAtFrame(0), config_);

        int framesToStack = static_cast<int>(percentages_[i] / 100.0 * files_.totalFrames());
        framesToStack = std::clamp(framesToStack, 1, files_.totalFrames());

        // Extract the necessary part of frames and sort them by their index.
        // This is needed because consequent access to frames is much faster than random one.
        std::vector<std::pair<int, double>> currentStack{frameQualities_.begin(), frameQualities_.begin() + framesToStack};
        std::sort(currentStack.begin(), currentStack.end(), [](const auto &a, const auto &b) {
            return a.first < b.first;
        });

        auto counter = std::make_shared<std::atomic<int>>(0);

        for (const auto &[index, quality] : currentStack) {
            asio::post(pool, [this, index, quality, counter, framesToStack] {
                stacker_.add(files_.matAtFrame(index), quality);
                emit frameProcessed(QString::number(++(*counter)) + "/" + QString::number(framesToStack));
            });
        }

        // Wait until all frames are processed
        pool.join();

        // Save current result
        cv::Mat result = stacker_.average();
        result.convertTo(result, CV_16UC3, 65535.0 / 255.0);

        QString parameters = QString("%1-%2-%3")
            .arg(percentages_[i])
            .arg(config_.aps ? "local-" + QString::number(config_.aps->size()) : "global")
            .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm-ss"));

        std::string filePath = outputDir_ + "/proxima-stacked" + parameters.toStdString() + ".tif";
        cv::imwrite(filePath, result, {cv::IMWRITE_TIFF_COMPRESSION, 1});

        emit singleStackCompleted(filePath);
    }

    running = false;
}
