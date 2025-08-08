#include "workspace.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileInfo>

Workspace::Workspace(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    _containerWidget = new QWidget;
    _containerLayout = new QVBoxLayout(_containerWidget);
    _containerLayout->setContentsMargins(0, 0, 0, 0);
    _containerLayout->setAlignment(Qt::AlignTop);

    _containerWidget->setLayout(_containerLayout);
    scrollArea->setWidget(_containerWidget);

    mainLayout->addWidget(scrollArea);
}

bool Workspace::addItem(const std::string &path) {
    MediaFile file(QString::fromStdString(path));
    if (!file.isValid()) {
        return false;
    }

    auto [it, inserted] = _mediaFiles.emplace(path, std::move(file));
    if (!inserted) {
        return false;
    }

    auto *item = new _WorkspaceItem(&it->second, this);
    _workspaceItems.emplace_back(item);

    _containerLayout->addWidget(item);
    _containerWidget->update();

    connect(item, &_WorkspaceItem::clicked, this, [this](const std::string &filePath) {
        emit itemClicked(&_mediaFiles.at(filePath));
    });
    connect(item, &_WorkspaceItem::checked, this, [this](const std::string &filePath, bool flag) {
        emit itemChecked(&_mediaFiles.at(filePath), flag);
    });

    return true;
}

void Workspace::clear() {
    for (auto &itemPtr : _workspaceItems) {
        _containerLayout->removeWidget(itemPtr);
        itemPtr->deleteLater();
    }
    _workspaceItems.clear();
    _mediaFiles.clear();
    _containerWidget->update();
}

void Workspace::enableMultipleSelection(bool flag) {
    for (auto item : _workspaceItems) {
        item->showCheckBox(flag);
    }
}

void Workspace::resetMultipleSelection() {
    for (auto item : _workspaceItems) {
        item->resetCheckBox();
    }
}

Workspace::_WorkspaceItem::_WorkspaceItem(MediaFile *file, QWidget *parent)
    : QFrame(parent)
    , _filePath(file->path())
{
    setFixedWidth(288);

    _display = new Display(this);
    _display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _display->setMinimumHeight(100);
    _display->setMaximumHeight(100);
    _display->show(file->matAtFrame(0), Qt::KeepAspectRatioByExpanding);

    _fileNameLabel = new QLabel(this);
    _fileNameLabel->setText(QString::fromStdString(file->filename() + file->extension()));
    _fileNameLabel->setAlignment(Qt::AlignCenter);

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

    _fileSizeLabel = new QLabel(this);
    _fileSizeLabel->setText("Size: " + sizeText);
    _fileSizeLabel->setAlignment(Qt::AlignLeft);

    _framesLabel = new QLabel(this);
    _framesLabel->setText("Frames: " + QString::number(file->frames()));
    _framesLabel->setAlignment(Qt::AlignRight);

    auto metadataLayout = new QHBoxLayout;
    metadataLayout->addWidget(_fileSizeLabel);
    metadataLayout->addWidget(_framesLabel);

    _checkBox = new QCheckBox(this);
    _checkBox->setHidden(true); // Hide check box by default
    _checkBox->setMinimumWidth(18);
    _checkBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(_checkBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        emit checked(_filePath, state == Qt::Checked);
    });

    auto layout = new QVBoxLayout;
    layout->addWidget(_checkBox);
    layout->addWidget(_display);
    layout->addWidget(_fileNameLabel);
    layout->addLayout(metadataLayout);
    layout->setContentsMargins(6, 6, 6, 6);

    auto layout_ = new QGridLayout;

    layout_->addWidget(_checkBox, 0, 0, Qt::AlignTop);
    layout_->addWidget(_display, 0, 1, 1, 2);
    layout_->addWidget(_fileNameLabel, 1, 1, 1, 2);
    layout_->addWidget(_fileSizeLabel, 2, 1);
    layout_->addWidget(_framesLabel, 2, 2);
    layout_->setContentsMargins(6, 6, 6, 6);

    setLayout(layout_);
    setFrameShape(QFrame::StyledPanel);
}

void Workspace::_WorkspaceItem::mousePressEvent(QMouseEvent *event) {
    emit clicked(_filePath);
    QFrame::mousePressEvent(event);
}
