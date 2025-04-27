#ifndef STACKER_H
#define STACKER_H

#include <opencv2/opencv.hpp>
#include "components/mediafile.h"
#include "components/alignmentpoint.h"

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

        double drizzle = 1.0;
    };

    static cv::Mat stack(Source &source, Config &config);

private:
    static cv::Mat stackGlobal(Source &source, Config &config, bool crop);
    static cv::Mat stackLocal(Source &source, Config &config);
};

#endif // STACKER_H
