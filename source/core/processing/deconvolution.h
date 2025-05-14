#ifndef DECONVOLUTION_H
#define DECONVOLUTION_H

#include <opencv2/opencv.hpp>

cv::Mat computePSF(double sigma, double kurtosis);

class Deconvolution {
public:
    enum Method {
        LucyRichardson,
        Blind
    };

    struct Config {
        int iterations;
        cv::Mat psf;
    };

    using IterationCallback = std::function<void(int, int)>;

    static cv::Mat deconvolve(cv::Mat &mat, Method method, Config &config, IterationCallback = nullptr);

private:
    static cv::Mat deconvolveLR(cv::Mat &mat, Config &config, IterationCallback = nullptr);
    static cv::Mat deconvolveBlind();
};

#endif // DECONVOLUTION_H
