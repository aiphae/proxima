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
    int itemCount() { return static_cast<int>(_workspaceItems.size()); }
    void clear();
    void enableMultipleSelection(bool flag);
    void resetMultipleSelection();

signals:
    void itemClicked(MediaFile *);
    void itemChecked(MediaFile *, bool flag);

private:
    class _WorkspaceItem;
    std::vector<_WorkspaceItem *> _workspaceItems;
    std::map<std::string, MediaFile> _mediaFiles;

    QWidget *_containerWidget;
    QVBoxLayout *_containerLayout;
};

// A single item in the workspace
class Workspace::_WorkspaceItem : public QFrame {
    Q_OBJECT

public:
    explicit _WorkspaceItem(MediaFile *file, QWidget *parent = nullptr);
    void showCheckBox(bool flag) { _checkBox->setHidden(!flag); }
    void resetCheckBox() { _checkBox->setCheckState(Qt::Unchecked); }

signals:
    void clicked(const std::string &filePath);
    void checked(const std::string &filePath, bool flag);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    const std::string _filePath;
    Display *_display;
    QLabel *_fileNameLabel;
    QLabel *_framesLabel;
    QLabel *_fileSizeLabel;
    QCheckBox *_checkBox;
};

#endif // WORKSPACE_H
