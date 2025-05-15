#include "stack_thread.h"

StackThread::StackThread(
    MediaManager &manager,
    StackConfig &config,
    std::vector<int> &percentages,
    QString &outputDir,
    QObject *parent
) : Thread(parent), manager(manager), config(config), percentages(percentages), outputDir(outputDir)
{
    connect(&stacker, &Stacker::progressUpdated, this, [this](QString status) {
        emit frameProcessed(status);
    });
}

void StackThread::run() {
    for (auto &percent : percentages) {
        emit statusUpdated(QString("Stacking %1...").arg(percent));

        int framesToStack = percent * manager.totalFrames() / 100;

        StackConfig currentConfig = config;
        currentConfig.sorted = std::vector(config.sorted.begin(), config.sorted.begin() + framesToStack);

        cv::Mat stacked = stacker.stack(manager, currentConfig);
        QString filePath = outputDir + QString("/proxima-stacked-%1.tif").arg(percent);
        cv::imwrite(filePath.toStdString(), stacked);
    }

    emit statusUpdated("Done!");
    emit finished();
    running = false;
}
