#include "analysis_thread.h"
#include "components/frame.h"
#include "asio/thread_pool.hpp"
#include "asio/post.hpp"

AnalysisThread::AnalysisThread(
    MediaCollection &files,
    std::vector<std::pair<int, double>> &output,
    QObject *parent)
: Thread(parent), files_(files), output_(output) {}

void AnalysisThread::run() {
    // Save one thread for the GUI, and one for the outer thread
    asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

    // Progress counter
    auto counter = std::make_shared<std::atomic<int>>(0);

    for (int i = 0; i < files_.totalFrames(); ++i) {
        asio::post(pool, [this, counter, i]() {
            const cv::Mat frame = files_.matAtFrame(i);
            output_[i].first = i;
            output_[i].second = Frame::estimateQuality(frame);
            emit progressUpdated(++*counter);
        });
    }

    // Wait until all frames are processed
    pool.join();

    // Normalize to [0, 1]
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::min();

    for (const auto &pair : output_) {
        minVal = std::min(minVal, pair.second);
        maxVal = std::max(maxVal, pair.second);
    }

    double range = maxVal - minVal;
    if (range > 0.0) {
        for (auto &pair : output_) {
            pair.second = (pair.second - minVal) / range;
        }
    }
    else {
        for (auto &pair : output_) {
            pair.second = 1.0;
        }
    }

    // Sort frames by quality (pair.second)
    std::stable_sort(output_.begin(), output_.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    emit finished();
    running = false;
}
