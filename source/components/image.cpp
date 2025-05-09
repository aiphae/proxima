#include "image.h"

Image &Image::applyColorTransform(const cv::Matx33f &transform) {
    clone.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *) {
        cv::Vec3f input(pixel[0], pixel[1], pixel[2]);
        cv::Vec3f output = transform * input;
        for (int i = 0; i < 3; ++i) {
            pixel[i] = cv::saturate_cast<uchar>(output[i]);
        }
    });
    return *this;
}

Image &Image::applyBias(const cv::Vec3f &bias) {
    clone.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *) {
        for (int c = 0; c < 3; ++c) {
            int val = static_cast<int>(pixel[c]) + static_cast<int>(bias[c]);
            pixel[c] = cv::saturate_cast<uchar>(val);
        }
    });
    return *this;
}
