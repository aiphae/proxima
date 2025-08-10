#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "data/media_file.h"
#include "stacking_dialog/stacking_dialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void _connectUI();
    void _setupUI();

    MediaFile *_currentFile;
    void _showFile();

    StackingDialog *_initializeStackingDialog();
};

#endif // MAIN_WINDOW_H
