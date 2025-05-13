#ifndef STACKER_H
#define STACKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "stacking/alignment.h"
#include "data/mediamanager.h"

struct StackConfig {
    // Sorted frames as index-quality pair
    std::vector<std::pair<int, double>> sorted;

    int outputWidth;
    int outputHeight;

    bool localAlign;
    AlignmentPointSet aps;

    double drizzle = 1.0;
};

class Stacker : public QObject {
    Q_OBJECT

public:
    explicit Stacker(QObject *parent = nullptr) : QObject(parent) {}
    cv::Mat stack(MediaManager &manager, StackConfig &config);

signals:
    // Signal to the main thread to update the GUI
    void progressUpdated(QString status);

private:
    cv::Mat stackGlobal(MediaManager &manager, StackConfig &config, bool emitSignals, bool crop);
    cv::Mat stackLocal(MediaManager &manager, StackConfig &config, bool emitSignals);
};

#endif // STACKER_H
