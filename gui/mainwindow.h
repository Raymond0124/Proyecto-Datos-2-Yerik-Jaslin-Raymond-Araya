#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QMap>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // --- Tabs ---
    QTabWidget *tabs;
    QWidget *vistaGeneralTab;
    QWidget *mapaMemoriaTab;
    QWidget *memoryTab;
    QWidget *leaksTab;

    // --- Vista General ---
    QLabel *metricasLabel;
    QProgressBar *usoMemoriaBar;
    QProgressBar *leaksBar;
    QLineSeries *memorySeries;
    QChart *chart;
    QChartView *chartView;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QListWidget *resumenList;

    // --- Memory Tab ---
    QTableWidget *memoryByFileTable;

    // --- Mapa de Memoria ---
    QTableWidget *memoriaTable;

    // ✅ NUEVO: Mapa para almacenar detalles completos de cada bloque
    QMap<QString, QJsonObject> blockDetailsMap;

    // --- Socket ---
    QTcpServer *server;
    QTcpSocket *clientSocket;

    // Buffer de lectura (para manejar fragmentación TCP)
    QByteArray readBuffer;

    // --- Timer ---
    QTimer *timeCounter;
    int currentTime;

    // --- Setup Tabs ---
    void setupVistaGeneral();
    void setupMapaMemoria();
    void setupMemoryTab();
    void setupLeaksTab();

    // --- Socket ---
    void setupSocket();

private slots:
    void newConnection();
    void readData();

    // --- Funciones auxiliares ---
    void incrementTime();
    void updateUIFromJson(const QJsonObject &json);
    void updateLeaksTab(const QJsonArray &leaks);

    // ✅ NUEVO: Slot para mostrar detalles del bloque
    void showBlockDetails(int row);
};

#endif // MAINWINDOW_H