// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTableWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTabWidget *tabs;

    // Vista general
    QWidget *vistaGeneralTab;
    QLabel *metricasLabel;
    QProgressBar *usoMemoriaBar;
    QProgressBar *leaksBar;
    QChartView *chartView;
    QLineSeries *memorySeries;
    QChart *chart;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QListWidget *resumenList;
    QTimer *timeCounter;
    int currentTime;

    // Mapa de memoria
    QWidget *mapaMemoriaTab;
    QTableWidget *memoriaTable;

    // Socket
    QTcpServer *server;

    void setupVistaGeneral();
    void setupMapaMemoria();
    void setupSocket();

private slots:
    void newConnection();
    void readData();
    void updateUIFromJson(const QJsonObject &json);
    void incrementTime();
};

#endif // MAINWINDOW_H
