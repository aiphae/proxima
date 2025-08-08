#include "sort_thread.h"
#include "threading/thread_pool.h"
#include "components/frame.h"

SortThread::SortThread(
    MediaCollection &manager,
    std::vector<std::pair<int, double>> &sorted,
    QObject *parent
) : Thread(parent), manager(manager), sorted(sorted) {}

void SortThread::run() {
    auto framesDone = std::make_shared<std::atomic<int>>(0);
    {
        ThreadPool pool(std::thread::hardware_concurrency() - 2);
        int totalFrames = manager.totalFrames();
        for (int i = 0; i < totalFrames; ++i) {
            pool.enqueue([this, i, framesDone]() {
                cv::Mat frame = manager.matAtFrame(i);
                sorted[i].first = i;
                sorted[i].second = Frame::estimateQuality(frame);
                emit progressUpdated(++(*framesDone));
            });
        }
    }

    // Normalize to [0, 1]
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::min();

    for (const auto &pair : sorted) {
        minVal = std::min(minVal, pair.second);
        maxVal = std::max(maxVal, pair.second);
    }

    double range = maxVal - minVal;
    if (range > 0.0) {
        for (auto &pair : sorted) {
            pair.second = (pair.second - minVal) / range;
        }
    }
    else {
        for (auto &pair : sorted) {
            pair.second = 1.0;
        }
    }

    // Sort from highest quality to lowest
    std::stable_sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    emit finished();
    running = false;
}
