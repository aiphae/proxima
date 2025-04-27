#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>
#include <QFutureWatcher>
#include "components/display.h"
#include "modules/stacker.h"

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
    void analyzingCompleted();

private:
    // UI
    Ui::StackPage *ui;
    void connectUI();
    void updateUI();
    void enableConfigEdit();

    // Display
    std::unique_ptr<Display> display;
    void displayFrame(const int frameNumber);
    int currentFrame = 0;

    Stacker stacker;

    int totalFrames = 0;
    void analyzeFrames();
};

#endif // STACKPAGE_H
