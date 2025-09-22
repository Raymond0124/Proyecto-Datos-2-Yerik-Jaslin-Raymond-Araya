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

    // --- Mapa de Memoria ---
    QTableWidget *memoriaTable;

    // --- Socket ---
    QTcpServer *server;
    QTcpSocket *clientSocket;

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
    void newConnection();
    void readData();

    // --- Funciones auxiliares ---
    void incrementTime();
    void updateUIFromJson(const QJsonObject &json);
    void updateLeaksTab(const QJsonArray &leaks);
};

#endif // MAINWINDOW_H
