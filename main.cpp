#include "init.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    INIT w;
    w.show();
    return a.exec();
}
