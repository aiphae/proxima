#ifndef MEDIA_VIEWER_H
#define MEDIA_VIEWER_H

#include <QWidget>
#include <QSlider>
#include "data/media_collection.h"
#include "components/display.h"

class MediaViewer : public QWidget {
    Q_OBJECT

public:
    explicit MediaViewer(QWidget *parent = nullptr);
    void show(std::variant<MediaFile *, MediaCollection> source,
              std::optional<std::vector<int>> &&map = std::nullopt);
    void clear();

private:
    Display *display;
    QSlider *slider;
    std::variant<MediaFile *, MediaCollection> source;
    std::vector<int> map;

    void showFrame(int frame);
};

#endif // MEDIA_VIEWER_H
