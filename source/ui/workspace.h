#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QCheckBox>
#include "data/media_file.h"
#include "components/display.h"

// Represents the whole workspace
class Workspace : public QWidget {
    Q_OBJECT

public:
    explicit Workspace(QWidget *parent = nullptr);
    bool addItem(const std::string &path);
    int itemCount() { return static_cast<unsigned int>(workspaceItems.size()); }
    void clear();
    void enableMultipleSelection(bool flag);
    void resetMultipleSelection();

signals:
    void itemClicked(MediaFile *);
    void itemChecked(MediaFile *, bool flag);

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
    void showCheckBox(bool flag) { checkBox->setHidden(!flag); }
    void resetCheckBox() { checkBox->setCheckState(Qt::Unchecked); }

signals:
    void clicked(const std::string &filePath);
    void checked(const std::string &filePath, bool flag);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    const std::string filePath;
    Display *display;
    QLabel *fileNameLabel;
    QLabel *framesLabel;
    QLabel *fileSizeLabel;
    QCheckBox *checkBox;
};

#endif // WORKSPACE_H
