#ifndef MEDIA_COLLECTION_H
#define MEDIA_COLLECTION_H

#include "data/media_file.h"

class MediaCollection {
public:
    MediaCollection() = default;
    ~MediaCollection() = default;

    cv::Mat matAtFrame(int frame);
    std::vector<cv::Mat> matsAtFrames(std::vector<int> &frames) { return {}; }
    bool allDimensionsEqual() const { return true; }
    int totalFrames() const { return _totalFrames; }
    int fileCount() const { return static_cast<int>(_files.size()); }

    void addFile(MediaFile *file);
    void removeFile(MediaFile *file);

    MediaFile &operator[](int index) {
        return *_files[index];
    }

private:
    std::vector<MediaFile *> _files;
    int _totalFrames = 0;
};

#endif // MEDIA_COLLECTION_H
