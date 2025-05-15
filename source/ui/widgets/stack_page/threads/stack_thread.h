#ifndef STACK_THREAD_H
#define STACK_THREAD_H

#include "threading/thread.h"
#include "stacking/stacker.h"
#include "data/media_manager.h"
#include <QObject>

class StackThread : public Thread {
    Q_OBJECT

public:
    explicit StackThread(
        MediaManager &manager,
        StackConfig &config,
        std::vector<int> &percentages,
        QString &outputDir,
        QObject *parent = nullptr
    );

signals:
    void statusUpdated(QString status);
    void frameProcessed(QString status);
    void finished();

protected:
    void run() override;

private:
    Stacker stacker;
    MediaManager &manager;
    StackConfig &config;
    std::vector<int> &percentages;
    QString &outputDir;
};

#endif // STACK_THREAD_H
