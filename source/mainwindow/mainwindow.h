#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum class Pages {
    Home = 0,
    Align = 1,
    Stack = 2,
    Process = 3
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void updateCurrentPage(int pageIndex);

private:
    Ui::MainWindow *ui;

    void initializePages();
    void connectButtons();
};

#endif // MAINWINDOW_H
