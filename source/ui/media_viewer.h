#ifndef MEDIA_VIEWER_H
#define MEDIA_VIEWER_H

#include <QWidget>
#include <QSlider>
#include "data/media_collection.h"
#include "components/display.h"

using ModifyingFunction = std::function<void(cv::Mat &)>;

class MediaViewer : public QWidget {
    Q_OBJECT

public:
    explicit MediaViewer(QWidget *parent = nullptr);
    void show(std::variant<MediaFile *, MediaCollection> source,
              const std::optional<std::vector<int>> &map = std::nullopt,
              ModifyingFunction = nullptr);
    void setModifyingFunction(ModifyingFunction);
    void clear();

private:
    Display *_display;
    QSlider *_slider;

    std::variant<MediaFile *, MediaCollection> _source;
    std::vector<int> _map;
    std::function<void(cv::Mat &)> _func;

    void _showFrame(int frame);
};

#endif // MEDIA_VIEWER_H
