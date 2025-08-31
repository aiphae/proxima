#include "workspace.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileInfo>

Workspace::Workspace(QWidget *parent)
    : QWidget(parent)
{
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    const auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    containerWidget_ = new QWidget;
    containerLayout_ = new QVBoxLayout(containerWidget_);
    containerLayout_->setContentsMargins(0, 0, 0, 0);
    containerLayout_->setAlignment(Qt::AlignTop);

    containerWidget_->setLayout(containerLayout_);
    scrollArea->setWidget(containerWidget_);

    mainLayout->addWidget(scrollArea);
}

bool Workspace::addItem(const std::string &path) {
    MediaFile file(QString::fromStdString(path));
    if (!file.isValid()) {
        return false;
    }

    auto [it, inserted] = mediaFiles_.emplace(path, std::move(file));
    if (!inserted) {
        return false;
    }

    auto *item = new WorkspaceItem_(&it->second, this);
    workspaceItems_.emplace_back(item);

    containerLayout_->addWidget(item);
    containerWidget_->update();

    connect(item, &WorkspaceItem_::clicked, this, [this](const std::string &filePath) {
        emit itemClicked(&mediaFiles_.at(filePath));
    });
    connect(item, &WorkspaceItem_::checked, this, [this](const std::string &filePath, const bool flag) {
        emit itemChecked(&mediaFiles_.at(filePath), flag);
    });

    return true;
}

void Workspace::clear() {
    for (const auto &itemPtr : workspaceItems_) {
        containerLayout_->removeWidget(itemPtr);
        itemPtr->deleteLater();
    }
    workspaceItems_.clear();
    mediaFiles_.clear();
    containerWidget_->update();
}

void Workspace::enableMultipleSelection(bool flag) const {
    for (const auto item : workspaceItems_) {
        item->showCheckBox(flag);
    }
}

void Workspace::resetMultipleSelection() const {
    for (const auto item : workspaceItems_) {
        item->resetCheckBox();
    }
}

Workspace::WorkspaceItem_::WorkspaceItem_(MediaFile *file, QWidget *parent)
    : QFrame(parent)
    , filePath_(file->path())
{
    setFixedWidth(288);

    display_ = new Display(this);
    display_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    display_->setMinimumHeight(100);
    display_->setMaximumHeight(100);
    display_->show(file->matAtFrame(0), Qt::KeepAspectRatioByExpanding);

    fileNameLabel_ = new QLabel(this);
    fileNameLabel_->setText(QString::fromStdString(file->filename() + file->extension()));
    fileNameLabel_->setAlignment(Qt::AlignCenter);

    QFileInfo fileInfo(QString::fromStdString(file->path()));
    qint64 size = fileInfo.size();
    QString sizeText;
    if (size < 1024) {
        sizeText = QString::number(size) + " bytes";
    }
    else if (size < 1024 * 1024) {
        sizeText = QString::number(size / 1024.0, 'f', 2) + " KB";
    }
    else if (size < 1024 * 1024 * 1024) {
        sizeText = QString::number(size / (1024.0 * 1024), 'f', 2) + " MB";
    }
    else {
        sizeText = QString::number(size / (1024.0 * 1024 * 1024), 'f', 2) + " GB";
    }

    fileSizeLabel_ = new QLabel(this);
    fileSizeLabel_->setText("Size: " + sizeText);
    fileSizeLabel_->setAlignment(Qt::AlignLeft);

    framesLabel_ = new QLabel(this);
    framesLabel_->setText("Frames: " + QString::number(file->frames()));
    framesLabel_->setAlignment(Qt::AlignRight);

    auto metadataLayout = new QHBoxLayout;
    metadataLayout->addWidget(fileSizeLabel_);
    metadataLayout->addWidget(framesLabel_);

    checkBox_ = new QCheckBox(this);
    checkBox_->setHidden(true); // Hide check box by default
    checkBox_->setMinimumWidth(18);
    checkBox_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(checkBox_, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState state) {
        emit checked(filePath_, state == Qt::Checked);
    });

    auto layout = new QGridLayout;

    layout->addWidget(checkBox_, 0, 0, Qt::AlignTop);
    layout->addWidget(display_, 0, 1, 1, 2);
    layout->addWidget(fileNameLabel_, 1, 1, 1, 2);
    layout->addWidget(fileSizeLabel_, 2, 1);
    layout->addWidget(framesLabel_, 2, 2);
    layout->setContentsMargins(6, 6, 6, 6);

    setLayout(layout);
    setFrameShape(StyledPanel);
}

void Workspace::WorkspaceItem_::mousePressEvent(QMouseEvent *event) {
    emit clicked(filePath_);
    QFrame::mousePressEvent(event);
}
