#ifndef SORTINGTHREAD_H
#define SORTINGTHREAD_H

#include "threading/thread.h"
#include "data/mediamanager.h"

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

#endif // SORTINGTHREAD_H
