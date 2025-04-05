#ifndef PREPROCESSPAGE_H
#define PREPROCESSPAGE_H

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "mediafile.h"
#include "display.h"
#include "preprocessor.h"

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
    // UI
    Ui::AlignPage *ui;
    void connectUI();
    void updateUI();

    // Display
    std::unique_ptr<Display> display;
    int currentFrame = 0;
    void displayFrame(const int frameNumber);
    std::tuple<int, int> findMediaFrame(const int frameNumber);

    // Selected files
    QStringList selectedFiles;
    std::vector<MediaFile> mediaFiles;
    int totalFrames = 0;

    // Processing
    PreprocessingConfig config;
    int objectSide = 0;
    void estimateParameters();
    void process();
    bool allDimensionsEqual();
};

#endif // PREPROCESSPAGE_H
