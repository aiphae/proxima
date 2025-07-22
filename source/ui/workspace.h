#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "data/media_file.h"
#include "components/display.h"

// Represents the whole workspace
class Workspace : public QWidget {
    Q_OBJECT

public:
    explicit Workspace(QWidget *parent = nullptr);
    bool addItem(const std::string &path);
    unsigned int count() { return static_cast<unsigned int>(workspaceItems.size()); }
    void clear();

signals:
    void itemClicked(MediaFile *);

private:
    class WorkspaceItem;
    std::vector<WorkspaceItem *> workspaceItems;
    std::map<std::string, MediaFile> mediaFiles;

    QWidget *containerWidget;
    QVBoxLayout *containerLayout;
};

// A single item in the workspace
class Workspace::WorkspaceItem : public QFrame {
    Q_OBJECT

public:
    explicit WorkspaceItem(MediaFile *file, QWidget *parent = nullptr);

signals:
    void clicked(std::string filePath);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    std::string filePath;
    Display *display;
    QLabel *fileNameLabel;
    QLabel *framesLabel;
    QLabel *fileSizeLabel;
};

#endif // WORKSPACE_H
