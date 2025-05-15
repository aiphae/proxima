#ifndef SORT_THREAD_H
#define SORT_THREAD_H

#include "threading/thread.h"
#include "data/media_manager.h"

class SortingThread : public Thread {
    Q_OBJECT

public:
    explicit SortingThread(
        MediaManager &manager,
        std::vector<std::pair<int, double>> &sorted,
        QObject *parent = nullptr
    );

signals:
    void progressUpdated(int current);
    void finished();

protected:
    void run() override;

private:
    MediaManager &manager;
    std::vector<std::pair<int, double>> &sorted;
};

#endif // SORT_THREAD_H
