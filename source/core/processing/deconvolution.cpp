#include "deconvolution.h"

cv::Mat computePSF(double sigma, double kurtosis) {
    // Ensure sigma is positive and not too small
    sigma = std::max(sigma, 1e-5);

    // Kernel size: large enough to capture PSF spread; always odd
    int ksize = std::max(3, static_cast<int>(std::ceil(6 * sigma)) | 1);
    cv::Mat psf(ksize, ksize, CV_32F);

    float sum = 0.0f;
    int center = ksize / 2;
    sigma *= sigma;

    // Kurtosis directly controls shape; lower values = flatter
    // Clamp to a small positive value to avoid undefined pow behavior at 0
    float shape = std::max(0.05f, static_cast<float>(kurtosis));

    for (int y = 0; y < ksize; ++y) {
        for (int x = 0; x < ksize; ++x) {
            float dx = x - center;
            float dy = y - center;
            float r2 = (dx * dx + dy * dy) / sigma;

            // Shape-controlled exponential decay
            float value = std::exp(-std::pow(r2, shape));
            psf.at<float>(y, x) = value;
            sum += value;
        }
    }

    // Normalize to ensure PSF sums to 1
    if (sum > 0.0f) {
        psf /= sum;
    }

    return psf;
}

cv::Mat Deconvolution::deconvolve(cv::Mat &mat, Method method, Config &config, IterationCallback callback) {
    cv::Mat result;
    switch (method) {
    case LucyRichardson:
        result = deconvolveLR(mat, config, callback);
        break;
    case Blind:
        result = deconvolveBlind();
        break;
    }
    return result;
}

cv::Mat Deconvolution::deconvolveLR(cv::Mat &mat, Config &config, IterationCallback callback) {
    cv::Mat psf = config.psf;
    cv::Mat psfFlipped;
    cv::flip(psf, psfFlipped, -1);

    int channels = mat.channels();
    std::vector<cv::Mat> inputChannels(channels);
    std::vector<cv::Mat> estimates(channels);

    // Split input and convert to float
    cv::split(mat, inputChannels);
    for (int c = 0; c < channels; ++c) {
        inputChannels[c].convertTo(inputChannels[c], CV_32F);
        estimates[c] = inputChannels[c].clone();
    }

    for (int i = 0; i < config.iterations; ++i) {
        for (int c = 0; c < channels; ++c) {
            cv::Mat estimateConv;
            cv::filter2D(estimates[c], estimateConv, CV_32F, psf, {-1, -1}, 0, cv::BORDER_REPLICATE);

            cv::Mat ratio;
            cv::divide(inputChannels[c], estimateConv + 1e-6f, ratio);

            cv::Mat correction;
            cv::filter2D(ratio, correction, CV_32F, psfFlipped, {-1, -1}, 0, cv::BORDER_REPLICATE);

            estimates[c] = estimates[c].mul(correction);
            cv::threshold(estimates[c], estimates[c], 0, 0, cv::THRESH_TOZERO);
        }

        if (callback) {
            callback(i + 1, config.iterations);
        }
    }

    // Merge and normalize output
    cv::Mat result;
    cv::merge(estimates, result);
    cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);
    result.convertTo(result, CV_8U, 255.0);
    return result;
}

cv::Mat Deconvolution::deconvolveBlind() {
    return {};
}
