#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <QWidget>

namespace Ui {
class HomePage;
}

class HomePage : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();

signals:
    void currentPageChanged(int pageIndex);

private:
    Ui::HomePage *ui;

    void connectButtons();
};

#endif // HOME_PAGE_H
