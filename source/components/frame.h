#ifndef FRAME_H
#define FRAME_H

#include <opencv2/opencv.hpp>

// Represents a single frame and allows some processing to it
class Frame {
public:
    Frame(cv::Mat mat) : image(mat) {}

    cv::Mat cropOnObject(int width, int height);

    cv::Rect findObject(int minSize = 0); // Finds an object and returns its bounding rectangle
    cv::Rect getObjectCrop(cv::Rect object, int width, int height); // Returns a crop rectangle centered on the object
    cv::Mat crop(cv::Rect rect); // Crops the image on 'rect'
    void convertToGray();
    double estimateQuality();

    cv::Mat &mat() { return image; }

private:
    cv::Mat image;
};

#endif // FRAME_H
