#include "media_viewer.h"
#include <QVBoxLayout>
#include <utility>

MediaViewer::MediaViewer(QWidget *parent)
    : QWidget{parent}
{
    display_ = new Display(this);
    display_->setAlignment(Qt::AlignCenter);
    slider_ = new QSlider(Qt::Horizontal, this);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(slider_);
    layout->addWidget(display_);

    setLayout(layout);

    connect(slider_, &QSlider::valueChanged, this, [this](const int value) {
        showFrame_(value);
    });
}

void MediaViewer::show(const std::variant<MediaFile *, MediaCollection> &source,
                       const std::optional<std::vector<int>> &map,
                       ModifyingFunction func) {
    source_ = source;
    if (map.has_value()) {
        map_ = map.value();
    }

    int totalFrames;
    if (std::holds_alternative<MediaFile *>(source)) {
        totalFrames = std::get<MediaFile *>(source)->frames();
    }
    else {
        totalFrames = std::get<MediaCollection>(source).totalFrames();
    }

    func_ = func;

    slider_->setMaximum(totalFrames - 1);
    slider_->setValue(0);
    slider_->setEnabled(totalFrames > 1);

    showFrame_(0);
}

void MediaViewer::clear() {
    display_->clear();
    slider_->setValue(0);
    source_ = std::variant<MediaFile *, MediaCollection>();
    map_.clear();
}

void MediaViewer::showFrame_(const int frame) {
    currentFrame_ = frame;

    int mapped = frame;
    if (!map_.empty() && frame < map_.size()) {
        mapped = map_[frame];
    }

    cv::Mat mat;
    if (std::holds_alternative<MediaFile *>(source_)) {
        mat = std::get<MediaFile *>(source_)->matAtFrame(mapped);
    }
    else {
        mat = std::get<MediaCollection>(source_).matAtFrame(mapped);
    }

    if (func_) {
        func_(mat);
    }

    display_->show(mat);
}

void MediaViewer::setModifyingFunction(ModifyingFunction func) {
    func_ = std::move(func);
    showFrame_(currentFrame_);
}
