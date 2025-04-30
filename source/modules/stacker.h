#ifndef STACKER_H
#define STACKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "components/alignmentpoint.h"
#include "modules/mediamanager.h"

struct StackSource {
    StackSource(MediaManager &manager) : manager(manager) {}
    MediaManager &manager;
    std::vector<std::pair<int, double>> sorted;
};

struct StackConfig {
    int framesToStack;
    int outputWidth;
    int outputHeight;
    bool localAlign;
    std::vector<AlignmentPoint> aps;
    double drizzle = 1.0;
};

class Stacker : public QObject {
    Q_OBJECT

public:
    explicit Stacker(QObject *parent = nullptr) : QObject(parent) {}
    cv::Mat stack(StackSource &source, StackConfig &config);

signals:
    // Signal to the main thread to update the GUI
    void progressUpdated(QString status);

private:
    cv::Mat stackGlobal(StackSource &source, StackConfig &config, bool emitSignals, bool crop);
    cv::Mat stackLocal(StackSource &source, StackConfig &config, bool emitSignals);
};

#endif // STACKER_H
