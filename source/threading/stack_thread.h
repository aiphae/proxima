#ifndef STACK_THREAD_H
#define STACK_THREAD_H

#include "threading/thread.h"
#include "data/media_collection.h"
#include "stacking/stacker.h"

class StackThread : public Thread {
    Q_OBJECT

public:
    explicit StackThread(
        MediaCollection &files,
        StackConfig &config,
        std::array<int, 4> &percentages,
        std::vector<std::pair<int, double>> &frameQualities,
        std::string &outputDir,
        QObject *parent = nullptr
    );

signals:
    void frameProcessed(const QString &);
    void singleStackCompleted(const std::string &);
    void finished();

protected:
    void run() override;

private:
    // Store references to:
    MediaCollection &files_;
    std::vector<std::pair<int, double>> &frameQualities_;

    // Store copies of:
    StackConfig config_;
    std::array<int, 4> percentages_;
    std::string outputDir_;

    Stacker stacker_;
};

#endif // STACK_THREAD_H
