#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/opencv.hpp>

class Image {
public:
    Image(cv::Mat image) : original(image), clone(image.clone()) {}
    Image &applyColorTransform(const cv::Matx33f &transform);
    Image &applyBias(const cv::Vec3f &bias);
    void reset() { clone = original.clone(); }
    cv::Mat mat() const { return clone; }

private:
    cv::Mat original;
    cv::Mat clone;
};

#endif // IMAGE_H
