#ifndef DECONVOLUTION_H
#define DECONVOLUTION_H

#include <opencv2/opencv.hpp>

struct PSF {
    PSF() = default;
    PSF(double sigma, double kurtosis) {
        int ksize = std::max(3, static_cast<int>(6 * sigma) | 1);
        psf = cv::Mat(ksize, ksize, CV_32F);

        float sum = 0.0f;
        int center = ksize / 2;
        float sigma2 = sigma * sigma;
        float kurtosisFactor = 1.0f + 0.1f * kurtosis;

        for (int y = 0; y < ksize; ++y) {
            for (int x = 0; x < ksize; ++x) {
                float dx = x - center;
                float dy = y - center;
                float r2 = (dx * dx + dy * dy) / sigma2;
                float value = std::exp(-std::pow(r2, kurtosisFactor));
                psf.at<float>(y, x) = value;
                sum += value;
            }
        }

        psf /= sum;
    }

    cv::Mat psf;
};

class Deconvolution {
public:
    enum Method {
        LucyRichardson,
        Blind
    };

    struct Config {
        int iterations;
        PSF psf;
    };

    static cv::Mat deconvolve(cv::Mat &mat, Method method, Config &config);

private:
    static cv::Mat deconvolveLR(cv::Mat &mat, Config &config);
    static cv::Mat deconvolveBlind();
};

#endif // DECONVOLUTION_H
