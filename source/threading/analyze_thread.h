#ifndef ANALYZE_THREAD_H
#define ANALYZE_THREAD_H

#include "threading/thread.h"
#include "data/media_collection.h"

class AnalyzeThread : public Thread {
    Q_OBJECT

public:
    explicit AnalyzeThread(
        MediaCollection &files,
        std::vector<std::pair<int, double>> &output,
        QObject *parent = nullptr
    );

signals:
    void progressUpdated(int current);
    void finished();

protected:
    void run() override;

private:
    MediaCollection &_files;
    std::vector<std::pair<int, double>> &_output;
};

#endif // ANALYZE_THREAD_H
