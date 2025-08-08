#include "media_collection.h"

void MediaCollection::addFile(MediaFile *file) {
    _files.push_back(file);
    _totalFrames += file->frames();
}

void MediaCollection::removeFile(MediaFile *file) {
    for (auto it = _files.begin(); it != _files.end(); ++it) {
        if (*it == file) {
            _totalFrames -= (*it)->frames();
            _files.erase(it);
            return;
        }
    }
}

cv::Mat MediaCollection::matAtFrame(int frame) {
    int currentFrame = 0;
    for (int i = 0; i < _files.size(); ++i) {
        if (frame < currentFrame + _files[i]->frames()) {
            return _files[i]->matAtFrame(frame - currentFrame);
        }
        currentFrame += _files[i]->frames();
    }
    return {};
}
