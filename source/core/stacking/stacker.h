#ifndef STACKER_H
#define STACKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "stacking/alignment.h"
#include "data/media_collection.h"

struct StackConfig {
    // Sorted frames as index-quality pair
    std::vector<std::pair<int, double>> sorted;

    int outputWidth;
    int outputHeight;

    bool localAlign;
    AlignmentPointSet aps;

    double drizzle = 1.0;
};

struct _StackConfig {
    int outputWidth;
    int outputHeight;
    AlignmentPointSet *aps = nullptr;
    double upsample;
};

class Stacker : public QObject {
public:
    using IterationCallback = std::function<void(int, int)>;
    static cv::Mat stack(MediaCollection &manager, StackConfig &config, IterationCallback callback = nullptr);

    void initialize(cv::Mat reference, const _StackConfig &config);
    void add(cv::Mat mat, double weight);
    cv::Mat average();

    void reset();

private:
    cv::Mat _reference;
    _StackConfig _config;

    cv::Mat _localAccumulator, _localWeights;
    cv::Mat _globalAccumulator, _globalWeights;
};

#endif // STACKER_H
