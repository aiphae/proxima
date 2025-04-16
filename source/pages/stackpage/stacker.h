#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "mediafile.h"
#include "alignmentpoint.h"

class Stacker {
public:
    cv::Mat stack();

    int framesToStack;
    int width;
    int height;

    cv::Mat reference;
    std::vector<MediaFile> files;
    std::vector<std::pair<int, double>> sorted;
    std::vector<AlignmentPoint> aps;

private:
    cv::Mat stackGlobal();
    cv::Mat stackLocal();
};

#endif // STACKER_H
