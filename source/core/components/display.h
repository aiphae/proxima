#ifndef DISPLAY_H
#define DISPLAY_H

#include <QLabel>
#include <opencv2/opencv.hpp>

class Display : public QLabel {
public:
    Display(QWidget *parent = nullptr) : QLabel(parent) {}
    void show(const cv::Mat &mat, Qt::AspectRatioMode mode = Qt::KeepAspectRatio);
    void resizeEvent(QResizeEvent *event) override;

private:
    QImage originalImage;
};

#endif // DISPLAY_H
