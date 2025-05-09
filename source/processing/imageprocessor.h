#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include "components/image.h"

class ImageProcessor {
public:
    ImageProcessor(Image &image) : image(image) {}
    void setBrightness(int value);
    void setContrast(int value);
    void setSaturation(int value);
    void apply();
    cv::Mat mat() const { return image.mat(); }

private:
    Image image;

    int brightness = 0;
    int contrast = 0;
    int saturation = 0;

    cv::Matx33f M = cv::Matx33f::eye();
    cv::Vec3f bias = cv::Vec3f{0, 0, 0};

    void updateMatrix();
};

#endif // IMAGEPROCESSOR_H
