#ifndef MEDIA_MANAGER_H
#define MEDIA_MANAGER_H

#include "data/media_file.h"
#include <vector>

class MediaManager {
public:
    MediaManager() = default;
    ~MediaManager();

    void load(const QVector<QString> &filenames);
    void clear();

    cv::Mat matAtFrame(int frame);
    bool allDimensionsEqual() const;
    int totalFrames() const { return _totalFrames; }

    MediaFile &operator[](int index) {
        return _files[index];
    }

private:
    std::vector<MediaFile> _files;
    int _totalFrames = 0;
};

#endif // MEDIA_MANAGER_H
