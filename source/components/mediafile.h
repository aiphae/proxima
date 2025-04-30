#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <opencv2/opencv.hpp>
#include <QSet>

// Allowed image extensions
const QSet<QString> imageExtensions = {
    ".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff"
};

// Allowed video extensions
const QSet<QString> videoExtensions = {
    ".mp4", ".avi", ".mkv", ".mov"
};

// Represents a single media file (video or image)
// and provides ability to extract specific frames from it
class MediaFile {
public:
    MediaFile(const QString &filename);
    ~MediaFile();

    bool isValid() const { return _isValid; }
    bool isVideo() const { return _isVideo; };
    int frames() const { return _frames; };
    cv::Size dimensions() const { return _dimensions; }
    std::string extension() const { return _extension; }
    std::string filename() const { return _filename; }
    cv::Mat matAtFrame(int frame);

    MediaFile(MediaFile&& other) noexcept;
    MediaFile& operator=(MediaFile&& other) noexcept;

    MediaFile(const MediaFile&) = delete;
    MediaFile& operator=(const MediaFile&) = delete;

private:
    cv::Mat image;
    cv::VideoCapture video;

    bool _isValid = false;
    bool _isVideo = false;
    int _frames = 0;
    cv::Size _dimensions = {0, 0};
    std::string _extension;
    std::string _filename;

    std::mutex videoMutex;
};

#endif // MEDIAFILE_H
