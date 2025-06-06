#include "color_correction.h"

void ColorCorrection::apply(cv::Mat &mat) {
    computeMatrices();
    mat.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *) {
        cv::Vec3f input(pixel[0], pixel[1], pixel[2]);
        cv::Vec3f output = M * input + bias;
        for (int i = 0; i < 3; ++i) {
            pixel[i] = cv::saturate_cast<uchar>(output[i]);
        }
    });
}

void ColorCorrection::reset() {
    *this = ColorCorrection();
}

void ColorCorrection::computeMatrices() {
    const float lr = 0.2126f, lg = 0.7152f, lb = 0.0722f;

    float s;
    if (saturation <= 0) {
        s = 1.0f + saturation / 100.0f;
    }
    else {
        s = 1.0f + 2.0f * (saturation / 100.0f);
    }

    float c = contrast / 100.0f + 1.0f;
    float b = brightness / 100.0f;

    cv::Matx33f satMat{
        (1 - s) * lr + s, (1 - s) * lg    , (1 - s) * lb,
        (1 - s) * lr    , (1 - s) * lg + s, (1 - s) * lb,
        (1 - s) * lr    , (1 - s) * lg    , (1 - s) * lb + s
    };

    cv::Matx33f contrastMat = cv::Matx33f::eye() * c;
    cv::Vec3f contrastBias = (1.0f - c) * 0.5f * 255.0f * cv::Vec3f(1, 1, 1);

    cv::Vec3f brightnessBias = b * 255.0f * cv::Vec3f(1, 1, 1);

    M = satMat * contrastMat;
    bias = satMat * contrastBias + brightnessBias;
}
