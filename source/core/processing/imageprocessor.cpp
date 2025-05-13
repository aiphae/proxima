#include "imageprocessor.h"

void ImageProcessor::load(cv::Mat mat) {
    original = mat;
    clone = original.clone();
}

void ImageProcessor::reset() {
    clone = original.clone();
    colorCorrection.reset();
    // wavelets.reset();
}

void ImageProcessor::apply() {
    clone = original.clone();

    // Wavelets

    // Color correction
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
