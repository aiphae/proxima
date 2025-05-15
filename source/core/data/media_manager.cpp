#include "media_manager.h"

MediaManager::~MediaManager() {

}

void MediaManager::clear() {
    _files.clear();
    _totalFrames = 0;
}

void MediaManager::load(const QVector<QString> &filenames) {
    for (const auto &filename : filenames) {
        if (MediaFile file(filename); file.isValid()) {
            _files.push_back(std::move(file));
            _totalFrames += _files.back().frames();
        }
    }
}

cv::Mat MediaManager::matAtFrame(int frame) {
    int currentFrame = 0;
    for (int i = 0; i < _files.size(); ++i) {
        if (frame < currentFrame + _files[i].frames()) {
            return _files[i].matAtFrame(frame - currentFrame);
        }
        currentFrame += _files[i].frames();
    }
    return cv::Mat{};
}

bool MediaManager::allDimensionsEqual() const {
    if (_files.size() < 1) {
        return false;
    }

    auto dimensions = _files[0].dimensions();
    for (const auto &file : _files) {
        if (file.dimensions() != dimensions) {
            return false;
        }
    }

    return true;
}
