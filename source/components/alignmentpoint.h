#ifndef ALIGNMENTPOINT_H
#define ALIGNMENTPOINT_H

#include <opencv2/opencv.hpp>

class AlignmentPoint {
public:
    AlignmentPoint(int x, int y, int size);
    cv::Rect rect() const { return roi; }

private:
    cv::Rect roi;
};

#endif // ALIGNMENTPOINT_H
