#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include "library.h" // Ajusta la ruta según tu proyecto


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentTime(0), clientSocket(nullptr)
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

MainWindow::~MainWindow() {}

void MainWindow::setupVistaGeneral()
{
    vistaGeneralTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(vistaGeneralTab);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    metricasLabel = new QLabel("Esperando datos del memory profiler...", this);
    metricasLabel->setStyleSheet(
        "QLabel { background-color: #f0f0f0; border: 1px solid #cccccc; padding: 10px; font-weight: bold; font-size: 12px; }"
    );
    metricasLabel->setFixedHeight(60);
    layout->addWidget(metricasLabel);

    QHBoxLayout *barsLayout = new QHBoxLayout();
    usoMemoriaBar = new QProgressBar();
    usoMemoriaBar->setFormat("Uso Memoria: %v MB (%p%)");
    usoMemoriaBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");

    leaksBar = new QProgressBar();
    leaksBar->setFormat("Memory Leaks: %v MB (%p%)");
    leaksBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");

    barsLayout->addWidget(usoMemoriaBar);
    barsLayout->addWidget(leaksBar);
    layout->addLayout(barsLayout);

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

    chart->addSeries(memorySeries);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    memorySeries->attachAxis(axisX);
    memorySeries->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);

    QLabel *resumenTitulo = new QLabel("Top 3 Archivos con Mayores Asignaciones:");
    resumenTitulo->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(resumenTitulo);

    resumenList = new QListWidget();
    resumenList->setMaximumHeight(120);
    layout->addWidget(resumenList);

    tabs->addTab(vistaGeneralTab, "Vista General");
}

void MainWindow::setupMapaMemoria()
{
    mapaMemoriaTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(mapaMemoriaTab);
    layout->setContentsMargins(10,10,10,10);

    QLabel *mapaTitulo = new QLabel("Mapa de Memoria - Bloques Asignados");
    mapaTitulo->setStyleSheet("font-weight: bold; font-size: 16px; padding: 10px;");
    layout->addWidget(mapaTitulo);

    memoriaTable = new QTableWidget(0, 4, this);
    memoriaTable->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Tipo de Dato" << "Tamaño (bytes)" << "Estado");
    memoriaTable->horizontalHeader()->setStretchLastSection(true);
    memoriaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memoriaTable->setAlternatingRowColors(true);
    memoriaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoriaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoriaTable->setSortingEnabled(true);
    memoriaTable->verticalHeader()->setVisible(false);
    memoriaTable->setStyleSheet(
        "QTableWidget { background-color: white; gridline-color: #d0d0d0; selection-background-color: #4CAF50; border: none; }"
        "QHeaderView::section { background-color: #2196F3; color: white; padding: 8px; border: none; font-weight: bold; font-size: 12px; }"
    );

    layout->addWidget(memoriaTable);
    tabs->addTab(mapaMemoriaTab, "Mapa de Memoria");
}

void MainWindow::setupMemoryTab()
{
    memoryTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(memoryTab);
    QLabel *label = new QLabel("Memory por Archivo (se actualizará con JSON recibido)");
    layout->addWidget(label);
    tabs->addTab(memoryTab, "Memory Tab");
}

void MainWindow::setupLeaksTab()
{
    leaksTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(leaksTab);
    QLabel *label = new QLabel("Memory Leaks (se actualizará con JSON recibido)");
    layout->addWidget(label);
    tabs->addTab(leaksTab, "Leaks Tab");
}

// ------------------ Timer ------------------
void MainWindow::incrementTime()
{
    currentTime++;
    axisX->setRange(std::max(0, currentTime - 60), currentTime);
}

// ------------------ Socket ------------------
void MainWindow::setupSocket()
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    if(!server->listen(QHostAddress::Any, 5000)) {
        qDebug() << "No se pudo iniciar el servidor";
    } else {
        qDebug() << "Servidor iniciado en puerto 5000";
    }
}

void MainWindow::newConnection()
{
    clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readData);
    qDebug() << "Cliente conectado";
}

void MainWindow::updateLeaksTab(const QJsonArray &leaks)
{
    // Limpia el tab
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(leaksTab->layout());
    if (!layout) return;

    // Elimina widgets previos
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QLabel *title = new QLabel("Lista de Memory Leaks detectados:");
    title->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget(leaks.size(), 4, leaksTab);
    table->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Tamaño" << "Archivo" << "Línea");
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int row = 0;
    for (auto val : leaks) {
        QJsonObject leak = val.toObject();
        table->setItem(row, 0, new QTableWidgetItem(leak["direccion"].toString()));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(leak["tamano"].toInt())));
        table->setItem(row, 2, new QTableWidgetItem(leak["archivo"].toString()));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(leak["linea"].toInt())));
        row++;
    }

    layout->addWidget(table);
}
void MainWindow::readData()
{
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString("memory");

    if (type == "memory") {
        updateUIFromJson(obj);
    } else if (type == "leaks") {
        updateLeaksTab(obj["data"].toArray());
    }
}


void MainWindow::updateUIFromJson(const QJsonObject &json)
{
    // Actualiza métricas generales
    int memoriaActual = json["memoriaActual"].toInt();
    int leaks = json["leaks"].toInt();
    metricasLabel->setText(QString("Memoria Actual: %1 MB | Leaks: %2 MB").arg(memoriaActual).arg(leaks));

    usoMemoriaBar->setValue(memoriaActual);
    leaksBar->setValue(leaks);

    // Actualiza gráfica
    memorySeries->append(currentTime, memoriaActual);
    axisX->setRange(std::max(0, currentTime - 60), currentTime);

    // Actualiza top archivos
    resumenList->clear();
    QJsonArray topArchivos = json["topArchivos"].toArray();
    for (auto val : topArchivos) {
        QJsonObject obj = val.toObject();
        QString file = obj["file"].toString();
        int size = obj["size"].toInt();
        resumenList->addItem(QString("%1 - %2 bytes").arg(file).arg(size));
    }

    // Actualiza tabla de memoria
    memoriaTable->setRowCount(0);
    QJsonArray bloques = json["bloques"].toArray();
    int row = 0;
    for (auto val : bloques) {
        QJsonObject obj = val.toObject();
        memoriaTable->insertRow(row);
        memoriaTable->setItem(row, 0, new QTableWidgetItem(obj["direccion"].toString()));
        memoriaTable->setItem(row, 1, new QTableWidgetItem(obj.contains("tipo") ? obj["tipo"].toString() : "Desconocido"));
        memoriaTable->setItem(row, 2, new QTableWidgetItem(QString::number(obj.contains("tam") ? obj["tam"].toInt() : 0)));
        memoriaTable->setItem(row, 3, new QTableWidgetItem(obj.contains("estado") ? obj["estado"].toString() : "Activo"));
        row++;
    }
}
//hola


