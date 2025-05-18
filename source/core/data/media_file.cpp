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
        const size_t blockIndex = frame / BLOCK_SIZE;
        std::lock_guard<std::mutex> lock(videoMutex);

        if (blockIndex != currentBlockIndex) {
            loadBlock(blockIndex);
        }

        const size_t localIndex = frame % BLOCK_SIZE;
        if (localIndex < currentBlock.size()) {
            return currentBlock[localIndex];
        }
        return cv::Mat();
    }

    return image;
}

void MediaFile::loadBlock(const size_t blockIndex) {
    currentBlock.clear();

    size_t startFrame = blockIndex * BLOCK_SIZE;
    video.set(cv::CAP_PROP_POS_FRAMES, startFrame);

    for (size_t i = 0; i < BLOCK_SIZE && startFrame + i < _frames; ++i) {
        cv::Mat mat;
        if (!video.read(mat))
            break;

        currentBlock.emplace_back(std::move(mat));
    }

    currentBlockIndex = blockIndex;
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
