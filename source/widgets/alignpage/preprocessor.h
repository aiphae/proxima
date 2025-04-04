#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <opencv2/opencv.hpp>

struct PreprocessingConfig {
    bool crop = false;
    int cropWidth = 0;
    int cropHeight = 0;
    bool rejectFrames = false;
    int minObjectSize = 0;
    bool toMonochrome = false;
};

class Preprocessor {
public:
    static cv::Mat process(cv::Mat &frame, PreprocessingConfig &config);
    static cv::Mat preview(cv::Mat &frame, PreprocessingConfig &config);
};

#endif // PREPROCESSOR_H
