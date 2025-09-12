#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "stacking/alignment.h"

struct StackConfig {
    int outputWidth;
    int outputHeight;
    const AlignmentPointSet *aps = nullptr;
    double upsample = 1.0;
};

class Stacker {
public:
    void initialize(const cv::Mat &reference, const StackConfig &config);
    void add(cv::Mat mat, double weight);
    cv::Mat average();

private:
    cv::Mat reference_;
    StackConfig config_;

    cv::Mat localAccumulator_, localWeights_;
    cv::Mat globalAccumulator_, globalWeights_;

    std::mutex mtx_;

    void reset_();
};

#endif // STACKER_H
