#ifndef DISPLAY_H
#define DISPLAY_H

#include <QLabel>
#include <opencv2/opencv.hpp>

// Class for displaying images in a QLabel
class Display {
public:
    Display(QLabel *label) : label(label) {}
    void show(const cv::Mat &mat);

private:
    QLabel *label;
};

#endif // DISPLAY_H
