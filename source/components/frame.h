#ifndef FRAME_H
#define FRAME_H

#include <opencv2/opencv.hpp>

// Represents a single frame and allows some processing to it
class Frame {
public:
    Frame(cv::Mat mat) : image(mat.clone()) {}

    cv::Rect findObject(int minSize = 0);

    cv::Rect getObjectCrop(cv::Rect object, int width, int height);

    cv::Mat crop(cv::Rect rect);

    void convertColor(int code);

    cv::Mat &mat() { return image; }

private:
    cv::Mat image;
};

#endif // FRAME_H
