#include "media_viewer.h"
#include <QVBoxLayout>

MediaViewer::MediaViewer(QWidget *parent)
    : QWidget{parent}
{
    display = new Display(this);
    display->setAlignment(Qt::AlignCenter);
    slider = new QSlider(Qt::Horizontal, this);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(slider);
    layout->addWidget(display);

    setLayout(layout);

    connect(slider, &QSlider::valueChanged, this, [this](int value) {
        showFrame(value);
    });
}

void MediaViewer::show(std::variant<MediaFile *, MediaCollection> source,
                       std::optional<std::vector<int>> &&map) {
    this->source = source;
    if (map.has_value()) {
        this->map = map.value();
    }

    int totalFrames;
    if (std::holds_alternative<MediaFile *>(source)) {
        totalFrames = std::get<MediaFile *>(source)->frames();
    }
    else {
        totalFrames = std::get<MediaCollection>(source).totalFrames();
    }

    slider->setMaximum(totalFrames - 1);
    slider->setValue(0);
    slider->setEnabled(totalFrames > 1);

    showFrame(0);
}

void MediaViewer::clear() {
    display->clear();
    slider->setValue(0);
    source = std::variant<MediaFile *, MediaCollection>();
    map.clear();
}

void MediaViewer::showFrame(int frame) {
    int mapped = frame;
    if (!map.empty() && frame < map.size()) {
        mapped = map[frame];
    }

    if (std::holds_alternative<MediaFile *>(source)) {
        display->show(std::get<MediaFile *>(source)->matAtFrame(mapped));
    }
    else {
        display->show(std::get<MediaCollection>(source).matAtFrame(mapped));
    }
}
