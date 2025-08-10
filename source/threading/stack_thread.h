#ifndef _STACK_THREAD_H
#define _STACK_THREAD_H

#include "threading/thread.h"
#include "data/media_collection.h"
#include "stacking/stacker.h"

class _StackThread : public Thread {
    Q_OBJECT

public:
    explicit _StackThread(
        MediaCollection &collection,
        _StackConfig &config,
        std::array<int, 4> &percentages,
        std::vector<std::pair<int, double>> &frameQualities,
        std::string &outputDir,
        QObject *parent = nullptr
    );

signals:
    void statusUpdated(QString status);
    void frameProcessed(QString status);
    void finished();

protected:
    void run() override;

private:
    MediaCollection &_collection;
    _StackConfig &_config;
    std::array<int, 4> &_percentages;
    std::vector<std::pair<int, double>> &_frameQualities;
    std::string &_outputDir;

    Stacker _stacker;
};

#endif // STACK_THREAD_H
