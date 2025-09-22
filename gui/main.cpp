// main.cpp
#include <QApplication>
#include "mainwindow.h"
#include "library.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Inicia la ventana principal
    MainWindow w;
    w.show();

    // Conectar la librería de memory profiling al GUI
    // Se conecta a localhost:5000, que es donde el MainWindow escucha
    connectToGui("127.0.0.1", 5000);

    // Opcional: imprimir estado inicial de memoria
    printMemoryReport();
    printLeakReport();

    return a.exec();
}
