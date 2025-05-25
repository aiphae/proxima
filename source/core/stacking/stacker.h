#ifndef STACKER_H
#define STACKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "stacking/alignment.h"
#include "data/media_manager.h"

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
public:
    using IterationCallback = std::function<void(int, int)>;
    static cv::Mat stack(MediaManager &manager, StackConfig &config, IterationCallback callback = nullptr);
};

#endif // STACKER_H
