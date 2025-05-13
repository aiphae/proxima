#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include "components/display.h"
#include "data/mediamanager.h"
#include "threads/sortingthread.h"
#include "threads/stackingthread.h"

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

private:
    // UI and responsiveness
    Ui::StackPage *ui;
    void connectUI();
    void updateOutputDimensions();
    void enableConfigEdit();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    // Manages opened files
    MediaManager manager;

    // Stacking configuration
    StackConfig config;
    void initializeConfig();

    // Separate threads to keep the UI responsive
    SortingThread sortingThread;
    StackingThread stackingThread;

    // Needed to initialize 'stackingThread'
    QString outputDir;
    std::vector<int> percentages;
};

#endif // STACKPAGE_H
