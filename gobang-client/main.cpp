#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
#ifdef WIN32
    QFont font;
    font.setFamily("Microsoft YaHei");
    a.setFont(font);
#endif
    qRegisterMetaType<std::string>("std::string"); // for lambda function, maybe
    qRegisterMetaType<bool>("bool");
    MainWindow w;
    w.show();

    return a.exec();
}
