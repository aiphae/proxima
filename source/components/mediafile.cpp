#include "mediafile.h"
#include <QFileInfo>

MediaFile::MediaFile(const QString &filename) {
    QFileInfo file(filename);
    QString extension = filename.mid(filename.lastIndexOf('.'));
    _filename = file.completeBaseName().toStdString();

    if (imageExtensions.contains(extension)) {
        image = cv::imread(filename.toStdString());
        if (!image.empty()) {
            _frames = 1;
            _isValid = true;
            _dimensions = image.size();
        }
    }
    else {
        video = cv::VideoCapture(filename.toStdString());
        if (video.isOpened()) {
            _frames = video.get(cv::CAP_PROP_FRAME_COUNT);
            _isVideo = true;
            _isValid = true;
            _dimensions = cv::Size(video.get(cv::CAP_PROP_FRAME_WIDTH), video.get(cv::CAP_PROP_FRAME_HEIGHT));
        }
    }
    _extension = extension.toStdString();
}

cv::Mat MediaFile::matAtFrame(int frame) {
    if (_isVideo) {
        video.set(cv::CAP_PROP_POS_FRAMES, frame);
        cv::Mat mat;
        if (video.read(mat)) {
            return mat;
        }
        else {
            return cv::Mat();
        }
    }

    return image;
}

MediaFile::~MediaFile() {
    if (_isVideo) {
        video.release();
    }
}
