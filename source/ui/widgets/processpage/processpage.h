#ifndef PROCESSPAGE_H
#define PROCESSPAGE_H

#include <QWidget>
#include "components/display.h"
#include "processing/imageprocessor.h"

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
    void connectUI();

    // Display
    std::unique_ptr<Display> display;

    ImageProcessor processor;
};

#endif // PROCESSPAGE_H
