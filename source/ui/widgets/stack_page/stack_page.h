#ifndef STACK_PAGE_H
#define STACK_PAGE_H

#include <QWidget>
#include "components/display.h"
#include "data/media_collection.h"
#include "threads/sort_thread.h"
#include "threads/stack_thread.h"

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
    void loadSettings();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    // Manages opened files
    MediaCollection manager;

    // Stacking configuration
    StackConfig config;
    void initializeConfig();

    // Separate threads to keep the UI responsive
    SortThread sortingThread;
    // StackThread stackingThread;

    // Needed to initialize 'stackingThread'
    QString outputDir;
    std::vector<int> percentages;
};

#endif // STACK_PAGE_H
