#include <QApplication>
#include <QMainWindow>

#include "MapWidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMainWindow main_window;
    main_window.setGeometry(0, 0, 1200, 800);

    MapWidget map_widget;
    main_window.setCentralWidget(&map_widget);

    main_window.show();

    return a.exec();
}
