#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "data/media_file.h"
#include "stacking_dialog/stack_dialog.h"
#include "ui/workspace.h"
#include "ui/media_viewer.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUI_();
    void connectUI_();

    MediaFile *currentFile_ = nullptr;
    void showFile_();

    StackDialog *initializeStackDialog_();

    // UI elements
    QFrame *workspaceFrame_;
    QFrame *displayFrame_;
    QFrame *toolsFrame_;

    Workspace *workspace_;
    MediaViewer *mediaViewer_;

    QLabel *totalFilesEdit_;

    QPushButton *addItemsPushButton_;
    QPushButton *clearWorkspacePushButton_;
    QPushButton *stackingPushButton_;
    QPushButton *saveButton_;
};

#endif // MAIN_WINDOW_H
