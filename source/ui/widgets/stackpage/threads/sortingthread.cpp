#include "sortingthread.h"
#include "threading/threadpool.h"
#include "components/frame.h"

SortingThread::SortingThread(
    MediaManager &manager,
    std::vector<std::pair<int, double>> &sorted,
    QObject *parent
) : Thread(parent), manager(manager), sorted(sorted) {}

void SortingThread::run() {
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

    std::stable_sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    emit finished();
    running = false;
}
