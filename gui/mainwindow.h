#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QTableWidget>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QHeaderView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "../memtracker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void incrementTime();
    void updateUIFromJson(const QJsonObject &json);

private:
    QTabWidget *tabs;

    // Vista general
    QWidget *vistaGeneralTab;
    QLabel *metricasLabel;
    QProgressBar *usoMemoriaBar;
    QProgressBar *leaksBar;
    QListWidget *resumenList;
    QLineSeries *memorySeries;
    QChart *chart;
    QChartView *chartView;
    QValueAxis *axisX;
    QValueAxis *axisY;
    int currentTime;

    // Mapa de memoria
    QWidget *mapaMemoriaTab;
    QTableWidget *memoriaTable;

    QTimer *timeCounter;




    void setupVistaGeneral();
    void setupMapaMemoria();
    void setupMemoryTab();
    void setupLeaksTab();
    void setupSocket();
    void newConnection();
    void readData();
};

#endif // MAINWINDOW_H
