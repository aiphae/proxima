#ifndef COLOR_CORRECTION_H
#define COLOR_CORRECTION_H

#include <opencv2/opencv.hpp>

class ColorCorrection {
public:
    ColorCorrection() = default;

    int brightness = 0;
    int contrast = 0;
    int saturation = 0;

    void apply(cv::Mat &mat);
    void reset();

private:
    cv::Matx33f M = cv::Matx33f::eye();
    cv::Vec3f bias = cv::Vec3f{0, 0, 0};
    void computeMatrices();
};

#endif // COLOR_CORRECTION_H
