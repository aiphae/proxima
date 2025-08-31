#ifndef WORKSPACE_H
#define WORKSPACE_H

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
    int itemCount() const { return static_cast<int>(workspaceItems_.size()); }
    void clear();
    void enableMultipleSelection(bool flag) const;
    void resetMultipleSelection() const;

signals:
    void itemClicked(MediaFile *);
    void itemChecked(MediaFile *, bool flag);

private:
    class WorkspaceItem_;
    std::vector<WorkspaceItem_ *> workspaceItems_;
    std::map<std::string, MediaFile> mediaFiles_;

    QWidget *containerWidget_;
    QVBoxLayout *containerLayout_;
};

// A single item in the workspace
class Workspace::WorkspaceItem_ : public QFrame {
    Q_OBJECT

public:
    explicit WorkspaceItem_(MediaFile *file, QWidget *parent = nullptr);
    void showCheckBox(const bool flag) const { checkBox_->setHidden(!flag); }
    void resetCheckBox() const { checkBox_->setCheckState(Qt::Unchecked); }

signals:
    void clicked(const std::string &filePath);
    void checked(const std::string &filePath, bool flag);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    const std::string filePath_;
    Display *display_;
    QLabel *fileNameLabel_;
    QLabel *framesLabel_;
    QLabel *fileSizeLabel_;
    QCheckBox *checkBox_;
};

#endif // WORKSPACE_H
