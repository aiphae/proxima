#ifndef ALIGNPAGE_H
#define ALIGNPAGE_H

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "mediafile.h"
#include "display.h"

namespace Ui {
class AlignPage;
}

class AlignPage : public QWidget {
    Q_OBJECT

public:
    explicit AlignPage(QWidget *parent = nullptr);
    ~AlignPage();

private slots:
    void on_openFilesPushButton_clicked();

private:
    // UI and display
    Ui::AlignPage *ui;
    std::unique_ptr<Display> display;
    void connectUI(); // Connects UI elements
    void updateUI(); // Updates current UI state

    // Selected files
    QStringList selectedFiles;
    QVector<MediaFile> mediaFiles;
    int totalFrames = 0;

    // Processing configuration
    struct {
        bool rejectFrames = false;
        int minObjectSize = 0;
        bool crop = false;
        int cropWidth = 0;
        int cropHeight = 0;
        bool toMonochrome = false;
        bool joinMode = true;
    } processingConfig;

    // Processing states
    cv::Rect currentObject;
    cv::Rect currentCrop;
    int objectSide = 0;
    int currentFrame = 0;

    // Processing methods
    cv::Mat processFrame(cv::Mat &frame); // Preprocesses a single frame
    cv::Rect findObject(const cv::Mat &frame);
    cv::Rect getCrop(const cv::Mat &frame, const cv::Rect &object);
    void estimateParameters(); // Estimates cropping side and object detection threshold

    // Processing operations
    void process(); // Processes all files
    bool allDimensionsEqual() const;

    // Display operations
    void displayFrame(const int frameNumber);
    std::tuple<int, int> findMediaFrame(const int frameNumber);
    cv::Mat previewProcessing(cv::Mat &frame);
    void previewCrop(const cv::Mat &frame);
    void previewObject(const cv::Mat &frame);
};

#endif // ALIGNPAGE_H
