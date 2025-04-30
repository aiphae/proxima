#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include "components/mediafile.h"
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

#endif // MEDIAMANAGER_H
