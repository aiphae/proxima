#include "main_window/main_window.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Proxima");
    QCoreApplication::setApplicationName("Proxima");

    MainWindow w;
    w.show();

    return a.exec();
}
