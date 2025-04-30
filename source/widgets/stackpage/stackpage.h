#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include <QFutureWatcher>
#include "components/display.h"
#include "modules/stacker.h"
#include "modules/mediamanager.h"

namespace Ui {
class StackPage;
}

class StackPage : public QWidget {
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

private slots:
    void selectFiles();
    void estimateAPGrid();
    void stack();

signals:
    void sortingProgressUpdated(int current);
    void analyzingCompleted();

private:
    // UI
    Ui::StackPage *ui;
    void connectUI();
    void updateOutputDimensions();
    void enableConfigEdit();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    MediaManager manager;

    StackSource source;
    StackConfig config;
    Stacker stacker;

    void analyzeFrames();
};

#endif // STACKPAGE_H
