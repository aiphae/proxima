#include "imageprocessor.h"

void ImageProcessor::apply() {
    image.applyColorTransform(M).applyBias(bias);
}

void ImageProcessor::setBrightness(int value) {
    brightness = value;
    updateMatrix();
}

void ImageProcessor::setContrast(int value) {
    contrast = value;
    updateMatrix();
}

void ImageProcessor::setSaturation(int value) {
    saturation = value;
    updateMatrix();
}

void ImageProcessor::updateMatrix() {
    // Luminance weights
    const float lr = 0.2126f, lg = 0.7152f, lb = 0.0722f;

    // Saturation matrix
    int s = saturation;
    cv::Matx33f satMat {
        (1 - s) * lr + s, (1 - s) * lg    , (1 - s) * lb,
        (1 - s) * lr    , (1 - s) * lg + s, (1 - s) * lb,
        (1 - s) * lr    , (1 - s) * lg    , (1 - s) * lb + s
    };

    // Contrast matrix
    int c = contrast;
    cv::Matx33f contrastMat = cv::Matx33f::eye() * c;
    cv::Vec3f contrastBias = (1.0f - c) * 0.5f * 255.0f * cv::Vec3f(1, 1, 1);

    // Brightness bias
    cv::Vec3f brightnessBias = brightness * 255.0f * cv::Vec3f(1, 1, 1);

    // Combine
    M = satMat * contrastMat;
    bias = satMat * contrastBias + brightnessBias;
}
