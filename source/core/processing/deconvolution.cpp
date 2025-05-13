#include "deconvolution.h"

cv::Mat Deconvolution::deconvolve(cv::Mat &mat, Method method, Config &config) {
    cv::Mat result;
    switch (method) {
    case LucyRichardson:
        result = deconvolveLR(mat, config);
        break;
    case Blind:
        result = deconvolveBlind();
        break;
    }
    return result;
}

cv::Mat Deconvolution::deconvolveLR(cv::Mat &mat, Config &config) {
    cv::Mat psf = config.psf.psf;
    cv::Mat psfFlipped;
    cv::flip(psf, psfFlipped, -1);

    int channels = mat.channels();
    if (channels == 1) {
        cv::Mat blurred;
        mat.convertTo(blurred, CV_32F);

        cv::Mat estimate = blurred.clone();

        for (int i = 0; i < config.iterations; ++i) {
            cv::Mat estimateConv;
            cv::filter2D(estimate, estimateConv, CV_32F, psf, {-1, -1}, 0, cv::BORDER_REPLICATE);

            cv::Mat ratio;
            cv::divide(blurred, estimateConv + 1e-6f, ratio);

            cv::Mat correction;
            cv::filter2D(ratio, correction, CV_32F, psfFlipped, {-1, -1}, 0, cv::BORDER_REPLICATE);

            estimate = estimate.mul(correction);

            cv::threshold(estimate, estimate, 0, 0, cv::THRESH_TOZERO);
        }

        cv::Mat output;
        cv::normalize(estimate, estimate, 0, 1, cv::NORM_MINMAX);
        estimate.convertTo(output, CV_8U, 255.0);
        return output;
    }
    else {
        std::vector<cv::Mat> inputChannels;
        cv::split(mat, inputChannels);

        std::vector<cv::Mat> outputChannels(channels);
        for (int c = 0; c < channels; ++c) {
            cv::Mat blurred;
            inputChannels[c].convertTo(blurred, CV_32F);

            cv::Mat estimate = blurred.clone();

            for (int i = 0; i < config.iterations; ++i) {
                cv::Mat estimateConv;
                cv::filter2D(estimate, estimateConv, CV_32F, psf, {-1, -1}, 0, cv::BORDER_REPLICATE);

                cv::Mat ratio;
                cv::divide(blurred, estimateConv + 1e-6f, ratio);

                cv::Mat correction;
                cv::filter2D(ratio, correction, CV_32F, psfFlipped, {-1, -1}, 0, cv::BORDER_REPLICATE);

                estimate = estimate.mul(correction);

                cv::threshold(estimate, estimate, 0, 0, cv::THRESH_TOZERO);
            }

            cv::Mat output;
            cv::normalize(estimate, estimate, 0, 1, cv::NORM_MINMAX);
            estimate.convertTo(output, CV_8U, 255.0);
            outputChannels[c] = output;
        }

        cv::Mat result;
        cv::merge(outputChannels, result);
        return result;
    }
}

cv::Mat Deconvolution::deconvolveBlind() {
    return {};
}
