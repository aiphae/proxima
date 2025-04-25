#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "mediafile.h"
#include "alignmentpoint.h"

class Stacker {
public:
    struct Source {
        std::vector<MediaFile> files;
        std::vector<std::pair<int, double>> sorted;
    };

    struct Config {
        int framesToStack;
        int outputWidth;
        int outputHeight;

        bool localAlign;
        std::vector<AlignmentPoint> aps;
    };

    static cv::Mat stack(Source &source, Config &config);

private:
    static cv::Mat stackGlobal(Source &source, Config &config);
    static cv::Mat stackLocal(Source &source, Config &config);
};

#endif // STACKER_H
