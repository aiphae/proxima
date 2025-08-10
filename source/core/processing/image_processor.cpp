#include "image_processor.h"

void ImageProcessor::load(cv::Mat mat) {
    // Convert to CV_32F for better precision
    if (mat.depth() != CV_32F) {
        mat.convertTo(original, CV_32F, 1.0 / 255.0);
    }
    else {
        original = mat;
    }
    clone = original.clone();
}

void ImageProcessor::reset() {
    clone = original.clone();
    colorCorrection.reset();
    wavelets.reset();
}

void ImageProcessor::apply() {
    clone = original.clone();
    wavelets.apply(clone);
    colorCorrection.apply(clone);
}

void ImageProcessor::setBrightness(int value) {
    colorCorrection.brightness = value;
}

void ImageProcessor::setContrast(int value) {
    colorCorrection.contrast = value;
}

void ImageProcessor::setSaturation(int value) {
    colorCorrection.saturation = value;
}

void ImageProcessor::setWaveletsGains(std::vector<float> &gains) {
    wavelets.gains = gains;
}
