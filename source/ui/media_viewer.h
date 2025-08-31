#ifndef MEDIA_VIEWER_H
#define MEDIA_VIEWER_H

#include <QSlider>
#include "data/media_collection.h"
#include "components/display.h"

using ModifyingFunction = std::function<void(cv::Mat &)>;

class MediaViewer : public QWidget {
    Q_OBJECT

public:
    explicit MediaViewer(QWidget *parent = nullptr);
    void show(const std::variant<MediaFile *, MediaCollection> &source,
              const std::optional<std::vector<int>> &map = std::nullopt,
              const ModifyingFunction & = nullptr);
    void setModifyingFunction(const ModifyingFunction &);
    void clear();

private:
    Display *display_;
    QSlider *slider_;

    std::variant<MediaFile *, MediaCollection> source_;
    std::vector<int> map_;
    std::function<void(cv::Mat &)> func_;

    int currentFrame_ = 0;
    void showFrame_(int frame);
};

#endif // MEDIA_VIEWER_H
