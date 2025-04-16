#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include "mediafile.h"
#include "display.h"
#include "stacker.h"

namespace Ui {
class StackPage;
}

class StackPage : public QWidget {
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

private slots:
    void on_selectFilesPushButton_clicked();
    void on_estimateAPGridPushButton_clicked();
    void on_stackPushButton_clicked();

private:
    // UI
    Ui::StackPage *ui;
    void connectUI();
    void updateUI();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    // Media files
    std::vector<MediaFile> mediaFiles;
    std::vector<std::pair<int, double>> sortedFrames;
    int totalFrames = 0;

    // Stacking
    Stacker stacker;
};

#endif // STACKPAGE_H
