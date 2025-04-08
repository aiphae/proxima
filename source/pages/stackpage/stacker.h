#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "mediafile.h"

struct StackingConfiguration {
    int frames;
    int width;
    int height;
};

struct StackingSource {
    cv::Mat reference;
    std::vector<MediaFile> &files;
    std::vector<std::pair<int, double>> &sorted;
};

class Stacker {
public:
    std::vector<cv::Point> aps;
    cv::Mat stack(const StackingSource &source, const StackingConfiguration &config);

private:
    // Performs stacking using global alignment
    cv::Mat stackGlobal(const StackingSource &source, const StackingConfiguration &config);
};

#endif // STACKER_H
