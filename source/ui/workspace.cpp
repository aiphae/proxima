#include "workspace.h"
#include <QHBoxLayout>
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

    // Insert into map
    auto [it, inserted] = mediaFiles.emplace(path, std::move(file));
    if (!inserted) {
        return false;
    }

    // Create item and UI
    auto *item = new WorkspaceItem(&it->second, this);
    workspaceItems.emplace_back(item);

    containerLayout->addWidget(item);
    containerWidget->update();

    connect(item, &WorkspaceItem::clicked, this, [this](std::string filePath) {
        emit itemClicked(&mediaFiles.at(filePath));
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

    auto layout = new QVBoxLayout;
    layout->addWidget(display);
    layout->addWidget(fileNameLabel);
    layout->addLayout(metadataLayout);
    layout->setContentsMargins(6, 6, 6, 6);

    setLayout(layout);
    setFrameShape(QFrame::StyledPanel);
}

void Workspace::WorkspaceItem::mousePressEvent(QMouseEvent *event) {
    emit clicked(filePath);
    QFrame::mousePressEvent(event);
}
