#include "imageviewer.h"
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>

ImageViewer::ImageViewer(QWidget *parent)
    : QLabel(parent), zoom(1.0) {
    setMouseTracking(true);
    setBackgroundRole(QPalette::Base);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAlignment(Qt::AlignCenter);
}

void ImageViewer::loadOriginal(const cv::Mat &mat) {
    if (mat.empty()) {
        return;
    }

    if (mat.depth() == CV_32F || mat.depth() == CV_64F) {
        cv::normalize(mat, original, 0, 255, cv::NORM_MINMAX);
        original.convertTo(original, CV_8U);
    }
    else {
        original = mat.clone();
    }

    if (original.channels() == 1) {
        cv::cvtColor(original, original, cv::COLOR_GRAY2BGR);
    }

    cv::Mat rgbMat;
    cv::cvtColor(original, rgbMat, cv::COLOR_BGR2RGB);

    QImage qimg(rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step, QImage::Format_RGB888);
    image = QPixmap::fromImage(qimg.copy());

    offset = QPoint(0, 0);
    zoom = 1.0;

    updateView();
}

void ImageViewer::show(const cv::Mat &mat) {
    if (mat.empty()) {
        return;
    }

    cv::Mat displayMat;

    if (mat.depth() == CV_32F || mat.depth() == CV_64F) {
        cv::normalize(mat, displayMat, 0, 255, cv::NORM_MINMAX);
        displayMat.convertTo(displayMat, CV_8U);
    } else {
        displayMat = mat.clone();
    }

    QImage qimg;
    if (displayMat.channels() == 1) {
        qimg = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_Grayscale8);
    }
    else {
        cv::cvtColor(displayMat, displayMat, cv::COLOR_BGR2RGB);
        qimg = QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_RGB888);
    }

    QPixmap stretchedPixmap = QPixmap::fromImage(qimg).scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    setPixmap(stretchedPixmap);
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragStart = event->pos();
        dragging = true;
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && !image.isNull()) {
        QPoint delta = event->pos() - dragStart;
        dragStart = event->pos();

        offset -= delta;

        // Clamp offset
        int maxX = std::max(0, (int) (image.width() * zoom - width()));
        int maxY = std::max(0, (int) (image.height() * zoom - height()));
        offset.setX(qBound(0, offset.x(), maxX));
        offset.setY(qBound(0, offset.y(), maxY));

        updateView();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    dragging = false;
}

void ImageViewer::wheelEvent(QWheelEvent *event) {
    if (image.isNull()) {
        return;
    }

    double scaleFactor = 1.15;

    QPointF cursorPos = event->position();
    QPointF imagePos = (cursorPos + QPointF(offset)) / zoom;

    double minZoom = std::max(
        static_cast<double>(width()) / image.width(),
        static_cast<double>(height()) / image.height()
    );
    double maxZoom = 4.0;

    if (event->angleDelta().y() > 0) {
        zoom *= scaleFactor;
    }
    else {
        zoom /= scaleFactor;
    }

    zoom = std::clamp(zoom, minZoom, maxZoom);

    // Compute new offset
    offset = (imagePos * zoom - cursorPos).toPoint();

    // Clamp or center offset if image is smaller than viewport
    QSize scaledSize = image.size() * zoom;
    int maxX = std::max(0, scaledSize.width() - width());
    int maxY = std::max(0, scaledSize.height() - height());

    offset.setX(qBound(0, offset.x(), maxX));
    offset.setY(qBound(0, offset.y(), maxY));

    updateView();
}

cv::Mat ImageViewer::current() {
    if (original.empty()) {
        return cv::Mat();
    }

    // Map QLabel view size to original image using zoom
    QSize viewSize = size();
    QSize scaledSize = (image.size() * zoom);

    // Compute visible region in original image space
    double scaleX = static_cast<double>(original.cols) / scaledSize.width();
    double scaleY = static_cast<double>(original.rows) / scaledSize.height();

    int x = static_cast<int>(offset.x() * scaleX);
    int y = static_cast<int>(offset.y() * scaleY);

    int w = static_cast<int>(viewSize.width() * scaleX);
    int h = static_cast<int>(viewSize.height() * scaleY);

    x = std::clamp(x, 0, original.cols - 1);
    y = std::clamp(y, 0, original.rows - 1);
    w = std::min(w, original.cols - x);
    h = std::min(h, original.rows - y);

    return original(cv::Rect(x, y, w, h)).clone();
}

void ImageViewer::updateView() {
    if (image.isNull()) {
        return;
    }

    QSize scaledSize = image.size() * zoom;
    QPixmap scaledImage = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap view(size());
    view.fill(Qt::black);

    QPainter painter(&view);
    QRect srcRect(offset, size());
    srcRect = srcRect & scaledImage.rect(); // Clamp to bounds
    painter.drawPixmap(0, 0, scaledImage.copy(srcRect));
    painter.end();

    setPixmap(view);
}
