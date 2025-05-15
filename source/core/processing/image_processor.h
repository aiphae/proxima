#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "processing/color_correction.h"
#include "processing/deconvolution.h"
#include "processing/wavelets.h"

class ImageProcessor {
public:
    ImageProcessor() {}
    void load(cv::Mat mat);
    void apply();
    cv::Mat mat() const { return clone; }
    cv::Mat orig() const { return original; }
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

#endif // IMAGE_PROCESSOR_H
