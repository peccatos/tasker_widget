#include <QApplication>
#include "ResourceWidget.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    ResourceWidget w;
    w.show();

    return a.exec();
}
