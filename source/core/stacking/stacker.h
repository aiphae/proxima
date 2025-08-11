#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "stacking/alignment.h"

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
    double upsample = 1.0;
};

class Stacker{
public:
    void initialize(const cv::Mat reference, const _StackConfig &config);
    void add(cv::Mat mat, double weight);
    cv::Mat average();

private:
    cv::Mat _reference;
    _StackConfig _config;

    cv::Mat _localAccumulator, _localWeights;
    cv::Mat _globalAccumulator, _globalWeights;

    std::mutex _mtx;

    void _reset();
};

#endif // STACKER_H
