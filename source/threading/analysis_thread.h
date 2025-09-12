#ifndef ANALYZE_THREAD_H
#define ANALYZE_THREAD_H

#include "threading/thread.h"
#include "data/media_collection.h"
#include <QObject>

class AnalysisThread : public Thread {
    Q_OBJECT

public:
    explicit AnalysisThread(
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
    MediaCollection &files_;
    std::vector<std::pair<int, double>> &output_;
};

#endif // ANALYZE_THREAD_H
