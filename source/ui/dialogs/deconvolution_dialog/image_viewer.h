#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <QLabel>
#include <QPixmap>
#include <QPoint>
#include <opencv2/opencv.hpp>

class ImageViewer : public QLabel {
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    void loadOriginal(const cv::Mat &mat);
    void show(const cv::Mat &mat);
    cv::Mat current();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updateView();
    cv::Mat original;

    QPixmap image;
    QPoint offset;
    QPoint dragStart;
    bool dragging = false;
    double zoom = 1.0;
};

#endif // IMAGE_VIEWER_H
