#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include "mediafile.h"
#include "display.h"

namespace Ui {
class StackPage;
}

class StackPage : public QWidget {
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

private slots:
    void on_selectFilesPushButton_clicked();

private:
    Ui::StackPage *ui;
    std::unique_ptr<Display> display;
    void connectUI();
    QString fileFilters();

    std::vector<MediaFile> mediaFiles;
    std::vector<std::pair<int, double>> sortedFrames;
    int totalFrames = 0;

    void sortFrames();

    void displayFrame(const int frameNumber);
    std::tuple<int, int> findMediaFrame(const int frameNumber);
    int currentFrame = 0;

    double calculateFrameQuality(const cv::Mat &mat);
};

#endif // STACKPAGE_H
