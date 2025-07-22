#include "display.h"

void Display::show(const cv::Mat &mat, Qt::AspectRatioMode mode) {
    cv::Mat displayMat;

    if (mat.depth() == CV_32F) {
        mat.convertTo(displayMat, CV_8U, 255.0);
    }
    else {
        displayMat = mat.clone();
    }

    if (displayMat.channels() == 3) {
        cv::cvtColor(displayMat, displayMat, cv::COLOR_BGR2RGB);
    }

    if (displayMat.channels() == 3) {
        originalImage = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_RGB888).copy();
    }
    else if (displayMat.channels() == 1) {
        originalImage = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_Grayscale8).copy();
    }
    else {
        originalImage = QImage();
    }

    setPixmap(QPixmap::fromImage(originalImage).scaled(size(), mode, Qt::SmoothTransformation));
}

void Display::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    if (!pixmap() || pixmap().isNull()) {
        return;
    }
    setPixmap(pixmap().scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}
