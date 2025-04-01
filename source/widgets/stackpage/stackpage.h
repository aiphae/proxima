#ifndef STACKPAGE_H
#define STACKPAGE_H

#include <QWidget>

namespace Ui {
class StackPage;
}

class StackPage : public QWidget {
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

private:
    Ui::StackPage *ui;
};

#endif // STACKPAGE_H
