#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include "processing/colorcorrection.h"
#include "processing/deconvolution.h"
#include "processing/wavelets.h"

class ImageProcessor {
public:
    ImageProcessor() {}
    void load(cv::Mat mat);
    void apply();
    cv::Mat mat() const { return clone; }
    cv::Mat& orig() { return original; }
    void reset();

    void setBrightness(int value);
    void setContrast(int value);
    void setSaturation(int value);

private:
    cv::Mat original;
    cv::Mat clone;

    ColorCorrection colorCorrection;
    Deconvolution deconvolution;
    Wavelets wavelets;
};

#endif // IMAGEPROCESSOR_H
