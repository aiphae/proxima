#ifndef PROCESS_PAGE_H
#define PROCESS_PAGE_H

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include "components/display.h"
#include "processing/image_processor.h"

namespace Ui {
class ProcessPage;
}

class ProcessPage : public QWidget {
    Q_OBJECT

public:
    explicit ProcessPage(QWidget *parent = nullptr);
    ~ProcessPage();

private slots:
    void selectFile();
    void saveFile();

private:
    // UI and responsiveness
    Ui::ProcessPage *ui;
    void setupUI();
    void connectUI();
    void resetUI();

    // Wavelets elements
    std::vector<QSlider *> gainSliders;

    // Display
    std::unique_ptr<Display> display;
    void displayProcessed();

    ImageProcessor processor;
};

#endif // PROCESS_PAGE_H
