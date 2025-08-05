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

    containerWidget = new QWidget;
    containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setAlignment(Qt::AlignTop);

    containerWidget->setLayout(containerLayout);
    scrollArea->setWidget(containerWidget);

    mainLayout->addWidget(scrollArea);
}

bool Workspace::addItem(const std::string &path) {
    MediaFile file(QString::fromStdString(path));
    if (!file.isValid()) {
        return false;
    }

    auto [it, inserted] = mediaFiles.emplace(path, std::move(file));
    if (!inserted) {
        return false;
    }

    auto *item = new WorkspaceItem(&it->second, this);
    workspaceItems.emplace_back(item);

    containerLayout->addWidget(item);
    containerWidget->update();

    connect(item, &WorkspaceItem::clicked, this, [this](const std::string &filePath) {
        emit itemClicked(&mediaFiles.at(filePath));
    });
    connect(item, &WorkspaceItem::checked, this, [this](const std::string &filePath, bool flag) {
        emit itemChecked(&mediaFiles.at(filePath), flag);
    });

    return true;
}

void Workspace::clear() {
    for (auto& itemPtr : workspaceItems) {
        containerLayout->removeWidget(itemPtr);
        itemPtr->deleteLater();
    }
    workspaceItems.clear();
    mediaFiles.clear();
    containerWidget->update();
}

void Workspace::enableMultipleSelection(bool flag) {
    for (auto item : workspaceItems) {
        item->showCheckBox(flag);
    }
}

void Workspace::resetMultipleSelection() {
    for (auto item : workspaceItems) {
        item->resetCheckBox();
    }
}

Workspace::WorkspaceItem::WorkspaceItem(MediaFile *file, QWidget *parent)
    : QFrame(parent)
    , filePath(file->path())
{
    setFixedWidth(288);

    display = new Display(this);
    display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    display->setMinimumHeight(100);
    display->setMaximumHeight(100);
    display->show(file->matAtFrame(0), Qt::KeepAspectRatioByExpanding);

    fileNameLabel = new QLabel(this);
    fileNameLabel->setText(QString::fromStdString(file->filename() + file->extension()));
    fileNameLabel->setAlignment(Qt::AlignCenter);

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

    fileSizeLabel = new QLabel(this);
    fileSizeLabel->setText("Size: " + sizeText);
    fileSizeLabel->setAlignment(Qt::AlignLeft);

    framesLabel = new QLabel(this);
    framesLabel->setText("Frames: " + QString::number(file->frames()));
    framesLabel->setAlignment(Qt::AlignRight);

    auto metadataLayout = new QHBoxLayout;
    metadataLayout->addWidget(fileSizeLabel);
    metadataLayout->addWidget(framesLabel);

    checkBox = new QCheckBox(this);
    checkBox->setHidden(true); // Hide check box by default
    checkBox->setMinimumWidth(18);
    checkBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(checkBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        emit checked(filePath, state == Qt::Checked);
    });

    auto layout = new QVBoxLayout;
    layout->addWidget(checkBox);
    layout->addWidget(display);
    layout->addWidget(fileNameLabel);
    layout->addLayout(metadataLayout);
    layout->setContentsMargins(6, 6, 6, 6);

    auto layout_ = new QGridLayout;

    layout_->addWidget(checkBox, 0, 0, Qt::AlignTop);
    layout_->addWidget(display, 0, 1, 1, 2);
    layout_->addWidget(fileNameLabel, 1, 1, 1, 2);
    layout_->addWidget(fileSizeLabel, 2, 1);
    layout_->addWidget(framesLabel, 2, 2);
    layout_->setContentsMargins(6, 6, 6, 6);

    setLayout(layout_);
    setFrameShape(QFrame::StyledPanel);
}

void Workspace::WorkspaceItem::mousePressEvent(QMouseEvent *event) {
    emit clicked(filePath);
    QFrame::mousePressEvent(event);
}
