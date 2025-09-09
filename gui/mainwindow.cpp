#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentTime(0)
{
    this->resize(1200, 800);
    this->setWindowTitle("Memory Profiler");

    tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    setupVistaGeneral();
    setupMapaMemoria();
    setupMemoryTab();
    setupLeaksTab();
    setupSocket();


    timeCounter = new QTimer(this);
    connect(timeCounter, &QTimer::timeout, this, &MainWindow::incrementTime);
    timeCounter->start(1000);
}

MainWindow::~MainWindow()
{
}


void MainWindow::setupVistaGeneral()
{
    vistaGeneralTab = new QWidget();
    QVBoxLayout *vgLayout = new QVBoxLayout(vistaGeneralTab);
    vgLayout->setSpacing(10);
    vgLayout->setContentsMargins(10, 10, 10, 10);


    metricasLabel = new QLabel("Esperando datos del memory profiler...", this);
    metricasLabel->setStyleSheet(
        "QLabel { background-color: #f0f0f0; border: 1px solid #cccccc; padding: 10px; font-weight: bold; font-size: 12px; }"
    );
    metricasLabel->setFixedHeight(60);
    vgLayout->addWidget(metricasLabel);


    QHBoxLayout *barsLayout = new QHBoxLayout();

    usoMemoriaBar = new QProgressBar();
    usoMemoriaBar->setFormat("Uso Memoria: %v MB (%p%)");
    usoMemoriaBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");

    leaksBar = new QProgressBar();
    leaksBar->setFormat("Memory Leaks: %v MB (%p%)");
    leaksBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");

    barsLayout->addWidget(usoMemoriaBar);
    barsLayout->addWidget(leaksBar);
    vgLayout->addLayout(barsLayout);

    // Gráfico de línea de tiempo
    memorySeries = new QLineSeries();
    memorySeries->setName("Uso de Memoria");

    chart = new QChart();

    chart->setTitle("Línea de Tiempo - Uso de Memoria");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    axisX = new QValueAxis();
    axisX->setTitleText("Tiempo (segundos)");
    axisX->setLabelFormat("%d");
    axisX->setRange(0, 60);

    axisY = new QValueAxis();
    axisY->setTitleText("Memoria (MB)");
    axisY->setLabelFormat("%d");
    axisY->setRange(0, 100);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    memorySeries->attachAxis(axisX);
    memorySeries->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    vgLayout->addWidget(chartView);

    // Resumen top 3 archivos
    QLabel *resumenTitulo = new QLabel("Top 3 Archivos con Mayores Asignaciones:");
    resumenTitulo->setStyleSheet("font-weight: bold; font-size: 14px;");
    vgLayout->addWidget(resumenTitulo);

    resumenList = new QListWidget();
    resumenList->setMaximumHeight(120);
    vgLayout->addWidget(resumenList);

    tabs->addTab(vistaGeneralTab, "Vista General");
}


void MainWindow::setupMapaMemoria()
{
    mapaMemoriaTab = new QWidget();
    QVBoxLayout *mmLayout = new QVBoxLayout(mapaMemoriaTab);
    mmLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *mapaTitulo = new QLabel("Mapa de Memoria - Bloques Asignados");
    mapaTitulo->setStyleSheet("font-weight: bold; font-size: 16px; padding: 10px;");
    mmLayout->addWidget(mapaTitulo);

    // Crear tabla con 4 columnas
    memoriaTable = new QTableWidget(0, 4, this);
    memoriaTable->setHorizontalHeaderLabels(QStringList()
        << "Dirección" << "Tipo de Dato" << "Tamaño (bytes)" << "Estado");

    // Encabezados azules estilo la captura
    memoriaTable->horizontalHeader()->setStretchLastSection(true);
    memoriaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    memoriaTable->setAlternatingRowColors(true);
    memoriaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoriaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoriaTable->setSortingEnabled(true);
    memoriaTable->verticalHeader()->setVisible(false);

    // Estilo más limpio como en tu captura
    memoriaTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: white;"
        "   gridline-color: #d0d0d0;"
        "   selection-background-color: #4CAF50;"
        "   border: none;"
        "}"
        "QHeaderView::section {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   padding: 8px;"
        "   border: none;"
        "   font-weight: bold;"
        "   font-size: 12px;"
        "}"
    );

    mmLayout->addWidget(memoriaTable);
    tabs->addTab(mapaMemoriaTab, "Mapa de Memoria");
}


// ------------------ Pestaña: Asignación por archivo ------------------
void MainWindow::setupMemoryTab()
{
    QWidget *memoryTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(memoryTab);

    QBarSeries *series = new QBarSeries();
    QStringList categories;

    for (const auto &p : fileAllocBytes) {
        const std::string &file = p.first;
        size_t size = p.second;
        QBarSet *set = new QBarSet(QString::fromStdString(file));
        *set << static_cast<int>(size);
        series->append(set);
        categories << QString::fromStdString(file);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Asignaciones de Memoria por Archivo");

    QBarCategoryAxis *axisX_cat = new QBarCategoryAxis();
    axisX_cat->append(categories);
    QValueAxis *axisY_val = new QValueAxis();
    axisY_val->setTitleText("Bytes");

    chart->addAxis(axisX_cat, Qt::AlignBottom);
    chart->addAxis(axisY_val, Qt::AlignLeft);

    series->attachAxis(axisX_cat);
    series->attachAxis(axisY_val);

    QChartView *chartViewLocal = new QChartView(chart);
    chartViewLocal->setRenderHint(QPainter::Antialiasing);

    layout->addWidget(chartViewLocal);
    tabs->addTab(memoryTab, "Asignación por Archivo");
}

// ------------------ Pestaña: Memory Leaks ------------------
void MainWindow::setupLeaksTab()
{
    QWidget *leaksTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(leaksTab);

    std::map<std::string, size_t> leaksByFile;
    for (const auto &entry : allocations) {
        const AllocationInfo &info = entry.second;
        leaksByFile[info.file] += info.size;
    }

    QPieSeries *series = new QPieSeries();
    for (const auto &p : leaksByFile) {
        series->append(QString::fromStdString(p.first), static_cast<qreal>(p.second));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Distribución de Memory Leaks");

    QChartView *chartViewLocal = new QChartView(chart);
    chartViewLocal->setRenderHint(QPainter::Antialiasing);

    layout->addWidget(chartViewLocal);
    tabs->addTab(leaksTab, "Memory Leaks");
}


void MainWindow::updateUIFromJson(const QJsonObject &json)
{

    int memoriaActual = json["memoriaActual"].toInt();
    int leaks = json["leaks"].toInt();
    metricasLabel->setText(QString("Memoria Actual: %1 MB | Leaks: %2 MB").arg(memoriaActual).arg(leaks));

    usoMemoriaBar->setValue(memoriaActual);
    leaksBar->setValue(leaks);


    currentTime++;
    axisX->setRange(std::max(0, currentTime - 60), currentTime);
    memorySeries->append(currentTime, memoriaActual);


    resumenList->clear();
    QJsonArray topArchivos = json["topArchivos"].toArray();
    for (auto val : topArchivos) {
        QJsonObject obj = val.toObject();
        QString file = obj["file"].toString();
        int size = obj["size"].toInt();
        resumenList->addItem(QString("%1 - %2 bytes").arg(file).arg(size));
    }

    // Actualizar mapa de memoria
    memoriaTable->setRowCount(0);
    QJsonArray bloques = json["bloques"].toArray();
    int row = 0;
    for (auto val : bloques) {
        QJsonObject obj = val.toObject();
        memoriaTable->insertRow(row);
        memoriaTable->setItem(row, 0, new QTableWidgetItem(obj["direccion"].toString()));
        memoriaTable->setItem(row, 1, new QTableWidgetItem(QString::number(obj["tam"].toInt())));
        memoriaTable->setItem(row, 2, new QTableWidgetItem(obj["archivo"].toString()));
        row++;
    }
}


void MainWindow::incrementTime()
{
    currentTime++;
    axisX->setRange(std::max(0, currentTime - 60), currentTime);
    int randomValue = QRandomGenerator::global()->bounded(100);
    memorySeries->append(currentTime, randomValue);
}


void MainWindow::setupSocket() {}
void MainWindow::newConnection() {}
void MainWindow::readData() {}
