#ifndef SORT_THREAD_H
#define SORT_THREAD_H

#include "threading/thread.h"
#include "data/media_collection.h"

class SortThread : public Thread {
    Q_OBJECT

public:
    explicit SortThread(
        MediaCollection &manager,
        std::vector<std::pair<int, double>> &sorted,
        QObject *parent = nullptr
    );

signals:
    void progressUpdated(int current);
    void finished();

protected:
    void run() override;

private:
    MediaCollection &manager;
    std::vector<std::pair<int, double>> &sorted;
};

#endif // SORT_THREAD_H
