#ifndef FRAME_H
#define FRAME_H

#include <opencv2/opencv.hpp>

// Represents a single frame and allows some processing to it
class Frame {
public:
    static cv::Mat cropOnObject(cv::Mat frame, int width, int height);
    static double estimateQuality(cv::Mat frame);

private:
    static cv::Mat crop(cv::Mat frame, cv::Rect rect); // Crops 'frame' on 'rect'
    static cv::Rect getObjectCrop(cv::Mat frame, cv::Rect object, int width, int height); // Returns a crop rectangle centered on the object
    static cv::Rect findObject(cv::Mat frame); // Finds an object and returns its bounding rectangle
};

#endif // FRAME_H
