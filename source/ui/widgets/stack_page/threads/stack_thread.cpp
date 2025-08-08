#include "stack_thread.h"
#include <QDateTime>

StackThread::StackThread(
    MediaCollection &manager,
    StackConfig &config,
    std::vector<int> &percentages,
    QString &outputDir,
    QObject *parent
) : Thread(parent), manager(manager), config(config), percentages(percentages), outputDir(outputDir)
{}

void StackThread::run() {
    for (auto &percent : percentages) {
        emit statusUpdated(QString("Stacking %1%...").arg(percent));

        int framesToStack = percent * manager.totalFrames() / 100;

        StackConfig currentConfig = config;
        currentConfig.sorted = std::vector(config.sorted.begin(), config.sorted.begin() + framesToStack);

        auto updateProgress = [this](int current, int total) {
            QMetaObject::invokeMethod(this, [this, current, total] {
                QString progress = QString("%1/%2").arg(current).arg(total);
                emit frameProcessed(progress);
            });
        };

        cv::Mat stacked = Stacker::stack(manager, currentConfig, updateProgress);
        stacked.convertTo(stacked, CV_16UC3, 65535.0 / 255.0);

        QString parameters = QString("%1-%2-%3")
            .arg(percent)
            .arg(currentConfig.localAlign ? "local-" + QString::number(config.aps.size()) + "aps" : "global")
            .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm-ss"));

        QString filePath = outputDir + "/proxima-stacked-" + parameters + ".tif";

        cv::imwrite(filePath.toStdString(), stacked, {cv::IMWRITE_TIFF_COMPRESSION, 1});
    }

    emit statusUpdated("Done!");
    emit finished();
    running = false;
}
