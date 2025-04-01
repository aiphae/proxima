#include "display.h"

void Display::show(const cv::Mat &mat) {
    cv::Mat cloned = mat.clone();
    cv::cvtColor(cloned, cloned, cv::COLOR_BGR2RGB);
    QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    label->setPixmap(QPixmap::fromImage(image).scaled(label->size(), Qt::KeepAspectRatio));
}
