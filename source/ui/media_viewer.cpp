#include "media_viewer.h"
#include <QVBoxLayout>

MediaViewer::MediaViewer(QWidget *parent)
    : QWidget{parent}
{
    _display = new Display(this);
    _display->setAlignment(Qt::AlignCenter);
    _slider = new QSlider(Qt::Horizontal, this);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(_slider);
    layout->addWidget(_display);

    setLayout(layout);

    connect(_slider, &QSlider::valueChanged, this, [this](int value) {
        _showFrame(value);
    });
}

void MediaViewer::show(std::variant<MediaFile *, MediaCollection> source,
                       const std::optional<std::vector<int>> &map,
                       ModifyingFunction func) {
    _source = source;
    if (map.has_value()) {
        _map = map.value();
    }

    int totalFrames;
    if (std::holds_alternative<MediaFile *>(source)) {
        totalFrames = std::get<MediaFile *>(source)->frames();
    }
    else {
        totalFrames = std::get<MediaCollection>(source).totalFrames();
    }

    _func = func;

    _slider->setMaximum(totalFrames - 1);
    _slider->setValue(0);
    _slider->setEnabled(totalFrames > 1);

    _showFrame(0);
}

void MediaViewer::clear() {
    _display->clear();
    _slider->setValue(0);
    _source = std::variant<MediaFile *, MediaCollection>();
    _map.clear();
}

void MediaViewer::_showFrame(int frame) {
    _currentFrame = frame;

    int mapped = frame;
    if (!_map.empty() && frame < _map.size()) {
        mapped = _map[frame];
    }

    cv::Mat mat;
    if (std::holds_alternative<MediaFile *>(_source)) {
        mat = std::get<MediaFile *>(_source)->matAtFrame(mapped);
    }
    else {
        mat = std::get<MediaCollection>(_source).matAtFrame(mapped);
    }

    if (_func) {
        _func(mat);
    }

    _display->show(mat);
}

void MediaViewer::setModifyingFunction(ModifyingFunction func) {
    _func = func;
    _showFrame(_currentFrame);
}
