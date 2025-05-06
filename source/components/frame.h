#ifndef FRAME_H
#define FRAME_H

#include "stacking/alignment.h"
#include <opencv2/opencv.hpp>

class Frame {
public:
    static cv::Mat centerObject(cv::Mat frame, int width, int height);
    static cv::Mat expandBorders(cv::Mat frame, int width, int height);
    static double estimateQuality(cv::Mat frame);
    static cv::Point2f computeShift(cv::Mat reference, cv::Mat target);
};

#endif // FRAME_H
