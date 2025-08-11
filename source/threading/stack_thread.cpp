#include "threading/stack_thread.h"
#include "boost/asio/thread_pool.hpp"
#include "boost/asio/post.hpp"
#include <QDateTime>

_StackThread::_StackThread(
    MediaCollection &collection,
    _StackConfig &config,
    std::array<int, 4> &percentages,
    std::vector<std::pair<int, double>> &frameQualities,
    std::string &outputDir,
    QObject *parent
) : Thread(parent), _collection(collection), _config(config),
    _percentages(percentages), _frameQualities(frameQualities), _outputDir(outputDir)
{}

void _StackThread::run() {
    // Output paths
    std::vector<std::string> paths(_percentages.size());

    for (int i = 0; i < _percentages.size(); ++i) {
        if (_percentages[i] < 1) {
            continue;
        }

        asio::thread_pool pool(std::thread::hardware_concurrency() - 2);

        emit statusUpdated(QString("Stacking %1%...").arg(_percentages[i]));

        _stacker.initialize(_collection.matAtFrame(0), _config);

        int framesToStack = static_cast<int>(_percentages[i] / 100.0 * _collection.totalFrames());
        qDebug() << "framesToStack: " << framesToStack << "\n";

        // Extract the necessary part of frames and sort them by their index.
        // This is needed because consequent access to frames is much faster than random one.
        std::vector<std::pair<int, double>> currentStack{_frameQualities.begin(), _frameQualities.begin() + framesToStack};
        std::sort(currentStack.begin(), currentStack.end(), [](const auto &a, const auto &b) {
            return a.first < b.first;
        });

        auto counter = std::make_shared<std::atomic<int>>(0);

        for (const auto &[index, quality] : currentStack) {
            asio::post(pool, [this, index, quality, counter] {
                qDebug() << "Adding " << index << "\n";
                _stacker.add(_collection.matAtFrame(index), quality);
                emit frameProcessed(QString::number(++(*counter)) + "/" + QString::number(_collection.totalFrames()));
            });
        }

        // Wait until all frames are processed
        pool.join();

        // Save current result
        cv::Mat result = _stacker.average();
        result.convertTo(result, CV_16UC3, 65535.0 / 255.0);

        QString parameters = QString("%1-%2-%3")
            .arg(_percentages[i])
            .arg(_config.aps ? "local-" + QString::number(_config.aps->size()) : "global")
            .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy-HH-mm-ss"));

        std::string filePath = _outputDir + "/proxima-stacked" + parameters.toStdString() + ".tif";
        cv::imwrite(filePath, result, {cv::IMWRITE_TIFF_COMPRESSION, 1});

        paths[i] = filePath;
    }

    emit statusUpdated("Done!");
    emit finished(paths);
    running = false;
}
