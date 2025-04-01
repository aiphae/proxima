#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <opencv2/opencv.hpp>
#include <QPixmap>
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
// and provides ability to extract frames from it
class MediaFile {
public:
    MediaFile(const QString &filename);
    ~MediaFile();

    bool isValid() const { return _isValid; }
    bool isVideo() const { return _isVideo; };
    unsigned int frames() const { return _frames; };
    cv::Size dimensions() const { return _dimensions; }
    cv::Mat matAtFrame(unsigned int frame);
    std::string extension() const { return _extension; }
    std::string filename() const { return _filename; }

private:
    cv::Mat image;
    cv::VideoCapture video;

    bool _isValid = false;
    bool _isVideo = false;
    unsigned long _frames = 0;
    cv::Size _dimensions = {0, 0};
    std::string _extension;
    std::string _filename;
};

#endif // MEDIAFILE_H
