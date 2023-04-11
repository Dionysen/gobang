#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<bool>("bool");
    MainWindow w;
    w.show();
    return a.exec();
}
