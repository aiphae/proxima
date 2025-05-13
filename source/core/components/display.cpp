#include "display.h"

void Display::show(const cv::Mat &mat) {
    cv::Mat displayMat;

    if (mat.type() == CV_32F || mat.type() == CV_64F) {
        cv::normalize(mat, displayMat, 0, 255, cv::NORM_MINMAX);
        displayMat.convertTo(displayMat, CV_8U);
    }
    else {
        displayMat = mat.clone();
    }

    QImage image;
    if (displayMat.channels() != 1) {
        cv::cvtColor(displayMat, displayMat, cv::COLOR_BGR2RGB);
        image = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_RGB888);
    }
    else {
        image = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_Grayscale8);
    }

    label->setPixmap(QPixmap::fromImage(image).scaled(label->size(), Qt::KeepAspectRatio));
}
