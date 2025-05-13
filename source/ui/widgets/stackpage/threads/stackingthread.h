#ifndef STACKINGTHREAD_H
#define STACKINGTHREAD_H

#include "threading/thread.h"
#include "stacking/stacker.h"
#include "data/mediamanager.h"
#include <QObject>

class StackingThread : public Thread {
    Q_OBJECT

public:
    explicit StackingThread(
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

#endif // STACKINGTHREAD_H
