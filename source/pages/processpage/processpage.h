#ifndef PROCESSPAGE_H
#define PROCESSPAGE_H

#include <QWidget>

namespace Ui {
class ProcessPage;
}

class ProcessPage : public QWidget {
    Q_OBJECT

public:
    explicit ProcessPage(QWidget *parent = nullptr);
    ~ProcessPage();

private:
    Ui::ProcessPage *ui;
};

#endif // PROCESSPAGE_H
