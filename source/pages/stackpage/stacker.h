#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>

class Stacker {
public:
    cv::Mat reference;
    std::vector<cv::Point> aps;

    cv::Mat stack(std::vector<cv::Mat> &frames);

private:
    // Performs stacking using global alignment
    cv::Mat stackGlobal(std::vector<cv::Mat> &frames);
};

#endif // STACKER_H
