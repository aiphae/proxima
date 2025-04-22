#ifndef FRAME_H
#define FRAME_H

#include <opencv2/opencv.hpp>
#include "alignmentpoint.h"

enum class APPlacement {
    Uniform,
    FeatureBased
};

class Frame {
public:
    static cv::Mat centerObject(cv::Mat frame, int width, int height);
    static double estimateQuality(cv::Mat frame);
    static std::vector<AlignmentPoint> getAps(cv::Mat frame, int apSize, APPlacement placement);
    static cv::Point2f computeShift(cv::Mat reference, cv::Mat target);
};

#endif // FRAME_H
