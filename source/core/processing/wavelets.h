#ifndef WAVELETS_H
#define WAVELETS_H

#include <opencv2/opencv.hpp>

class Wavelets {
public:
    Wavelets() = default;
    void apply(cv::Mat &mat);
    void reset();
    std::vector<float> gains;
    std::vector<float> thresholds;

private:
    std::vector<cv::Mat> layers;
    void decompose(cv::Mat &mat);
    void reconstruct(cv::Mat &mat);
};

#endif // WAVELETS_H
