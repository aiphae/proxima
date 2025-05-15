#include "media_file.h"
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
        std::lock_guard<std::mutex> lock(videoMutex);
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

MediaFile::MediaFile(MediaFile&& other) noexcept {
    image = std::move(other.image);
    video = std::move(other.video);
    _isValid = other._isValid;
    _isVideo = other._isVideo;
    _frames = other._frames;
    _dimensions = other._dimensions;
    _extension = std::move(other._extension);
    _filename = std::move(other._filename);
    // Note: don't move std::mutex — construct a new one
}

MediaFile& MediaFile::operator=(MediaFile&& other) noexcept {
    if (this != &other) {
        image = std::move(other.image);
        video = std::move(other.video);
        _isValid = other._isValid;
        _isVideo = other._isVideo;
        _frames = other._frames;
        _dimensions = other._dimensions;
        _extension = std::move(other._extension);
        _filename = std::move(other._filename);
        // mutex — construct a new one
    }
    return *this;
}
