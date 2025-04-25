#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include <QFutureWatcher>
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

signals:
    void sortingProgressUpdated(int current);
    void analyzingComplete();

private:
    // UI
    Ui::StackPage *ui;
    void connectUI();
    void updateUI();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    // Stacking
    Stacker::Config config;
    Stacker::Source source;

    int totalFrames = 0;
    void analyzeFrames();
};

#endif // STACKPAGE_H
