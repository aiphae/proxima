#include "analyze_thread.h"
#include "components/frame.h"
#include "asio/thread_pool.hpp"
#include "asio/post.hpp"
#include <QDebug>

AnalyzeThread::AnalyzeThread(
    MediaCollection &files,
    std::vector<std::pair<int, double>> &output,
    QObject *parent)
: Thread(parent), _files(files), _output(output) {}

void AnalyzeThread::run() {
    // Save one thread for the GUI, and one for the outer thread
    asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

    // Progress counter
    auto counter = std::make_shared<std::atomic<int>>(0);

    for (int i = 0; i < _files.totalFrames(); ++i) {
        asio::post(pool, [this, counter, i]() {
            cv::Mat frame = _files.matAtFrame(i);
            _output[i].first = i;
            _output[i].second = Frame::estimateQuality(frame);
            emit progressUpdated(++(*counter));
        });
    }

    // Wait until all frames are processed
    pool.join();

    // Normalize to [0, 1]
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::min();

    for (const auto &pair : _output) {
        minVal = std::min(minVal, pair.second);
        maxVal = std::max(maxVal, pair.second);
    }

    double range = maxVal - minVal;
    if (range > 0.0) {
        for (auto &pair : _output) {
            pair.second = (pair.second - minVal) / range;
        }
    }
    else {
        for (auto &pair : _output) {
            pair.second = 1.0;
        }
    }

    // Sort frames by quality (pair.second)
    std::stable_sort(_output.begin(), _output.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    emit finished();
    running = false;
}
