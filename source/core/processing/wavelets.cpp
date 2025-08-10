#include "wavelets.h"

void Wavelets::reset() {
    layers.clear();
    gains.clear();
}

void Wavelets::decompose(cv::Mat &mat) {
    if (mat.empty()) {
        return;
    }

    cv::Mat input;
    if (mat.type() != CV_32F) {
        mat.convertTo(input, CV_32F);
    }
    else {
        input = mat.clone();
    }

    layers.clear();
    cv::Mat current = input.clone();

    // B3-spline kernel coefficients for a trous
    float filterData[] = {1.f / 16, 4.f / 16, 6.f / 16, 4.f / 16, 1.f / 16};

    int numLayers = 5;

    for (int scale = 0; scale < numLayers; ++scale) {
        int step = 1 << scale;

        // Create dilated kernel
        int kernelSize = 1 + 4 * step; // size for 5-tap kernel with dilation
        cv::Mat kernel = cv::Mat::zeros(1, kernelSize, CV_32F);

        // Place kernel coefficients at dilated positions
        for (int i = 0; i < 5; ++i) {
            int pos = i * step;
            if (pos < kernelSize) {
                kernel.at<float>(0, pos) = filterData[i];
            }
        }

        // Apply separable filter (horizontal then vertical)
        cv::Mat smoothed;
        cv::sepFilter2D(current, smoothed, CV_32F, kernel, kernel.t(), cv::Point(-1, -1), 0, cv::BORDER_REFLECT);

        // Compute detail layer (wavelet coefficients)
        cv::Mat detail = current - smoothed;
        layers.push_back(detail.clone());

        // Update current for next iteration
        current = smoothed.clone();
    }

    // Store the final low-resolution approximation
    layers.push_back(current.clone());
}

void Wavelets::reconstruct(cv::Mat &mat) {
    if (layers.empty()) {
        return;
    }

    // Start with the residual (low-frequency) layer
    cv::Mat result = layers.back().clone();

    // Apply enhancement coefficients to each detail layer
    for (int i = 0; i < layers.size() - 1; ++i) {
        cv::Mat enhanced_layer;

        // Apply gain coefficient
        float gain = (gains.size() > i) ? gains[i] : 1.0f;
        enhanced_layer = layers[i] * gain;

        // Add enhanced layer to result
        result += enhanced_layer;
    }

    if (mat.type() != CV_32F) {
        result.convertTo(mat, mat.type());
    }
    else {
        mat = result.clone();
    }
}

void Wavelets::apply(cv::Mat &mat) {
    if (layers.empty()) {
        decompose(mat);
    }

    reconstruct(mat);
}
