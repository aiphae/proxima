#ifndef STACKER_H
#define STACKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "components/mediafile.h"
#include "components/alignmentpoint.h"

class Stacker : public QObject {
    Q_OBJECT

public:
    explicit Stacker(QObject *parent = nullptr) : QObject(parent) {}

    struct {
        std::vector<MediaFile> files;
        std::vector<std::pair<int, double>> sorted;
    } source;

    struct {
        int framesToStack;
        int outputWidth;
        int outputHeight;
        bool localAlign;
        std::vector<AlignmentPoint> aps;
        double drizzle = 1.0;
    } config;

    cv::Mat stack();

signals:
    void progressUpdated(int current, int total);

private:
    cv::Mat stackGlobal(bool crop);
    cv::Mat stackLocal();
};

#endif // STACKER_H
