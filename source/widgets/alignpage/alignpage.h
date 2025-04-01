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
    Ui::AlignPage *ui;

    // To display images
    std::unique_ptr<Display> display;

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

    // Preprocesses a frame
    cv::Mat processFrame(cv::Mat &frame);
    // To store data about an object on current frame
    cv::Rect currentObject, currentCrop;

    // Finds an object on 'frame' and returns a bounding rectangle
    cv::Rect findObject(const cv::Mat &frame);
    // Stores the longest side of an object. Used for estimating crop and scaling it
    int objectSide = 0;
    // Returns a cropping rectangle around 'object'
    cv::Rect getCrop(const cv::Mat &frame, const cv::Rect &object);
    // Estimates the optimal cropping side (always a square)
    void estimateParameters();

    // Processes 'selectedFiles' and saves the result
    void process();
    // Returns true if all files in 'mediaFiles' have equal dimensions
    bool allDimensionsEqual();

    // For displaying
    void displayFrame(const int frameNumber);
    cv::Mat previewProcessing(cv::Mat &frame);
    void previewCrop(const cv::Mat &frame);
    void previewObject(const cv::Mat &frame);
    int currentFrame = 0;

    // Connects UI elemets
    void connectUI();
    // Updates UI edits without triggering any signals
    void updateUI();
};

#endif // ALIGNPAGE_H
