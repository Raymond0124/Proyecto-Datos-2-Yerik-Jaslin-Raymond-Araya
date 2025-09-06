// mainwindow.cpp
#include "mainwindow.h"
#include <QHeaderView>
#include <QDebug>
#include <QHostAddress>
#include <QTableWidgetItem>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentTime(0)
{
    this->resize(1200, 800);
    this->setWindowTitle("Memory Profiler");

    tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    setupVistaGeneral();
    setupMapaMemoria();
    setupSocket();

    // Timer para actualizar el eje X del gráfico cada segundo
    timeCounter = new QTimer(this);
    connect(timeCounter, &QTimer::timeout, this, &MainWindow::incrementTime);
    timeCounter->start(1000); // 1 segundo
}

MainWindow::~MainWindow()
{
    if (server && server->isListening()) {
        server->close();
    }
}

void MainWindow::setupVistaGeneral()
{
    vistaGeneralTab = new QWidget();
    QVBoxLayout *vgLayout = new QVBoxLayout(vistaGeneralTab);
    vgLayout->setSpacing(10);
    vgLayout->setContentsMargins(10, 10, 10, 10);

    // Métricas generales
    metricasLabel = new QLabel("Esperando datos del memory profiler...", this);
    metricasLabel->setStyleSheet(
        "QLabel {"
        "   background-color: #f0f0f0;"
        "   border: 1px solid #cccccc;"
        "   padding: 10px;"
        "   font-weight: bold;"
        "   font-size: 12px;"
        "}"
    );
    metricasLabel->setFixedHeight(60);
    vgLayout->addWidget(metricasLabel);

    // Barras de progreso
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
    chart->addSeries(memorySeries);
    chart->setTitle("Línea de Tiempo - Uso de Memoria");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Configurar ejes
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
    chartView->setMinimumHeight(300);
    vgLayout->addWidget(chartView);

    // Resumen top 3 archivos
    QLabel *resumenTitulo = new QLabel("Top 3 Archivos con Mayores Asignaciones:");
    resumenTitulo->setStyleSheet("font-weight: bold; font-size: 14px;");
    vgLayout->addWidget(resumenTitulo);

    resumenList = new QListWidget();
    resumenList->setStyleSheet(
        "QListWidget {"
        "   background-color: #e3f2fd;"
        "   border: 1px solid #90caf9;"
        "   border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "   border-bottom: 1px solid #bbdefb;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #bbdefb;"
        "}"
    );
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

    memoriaTable = new QTableWidget(0, 4, this);
    memoriaTable->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Tipo de Dato" << "Tamaño (bytes)" << "Estado");

    // Configurar tabla
    memoriaTable->horizontalHeader()->setStretchLastSection(true);
    memoriaTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    memoriaTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    memoriaTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    memoriaTable->setAlternatingRowColors(true);
    memoriaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoriaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoriaTable->setSortingEnabled(true);
    memoriaTable->verticalHeader()->setVisible(false);

    memoriaTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: white;"
        "   gridline-color: #d0d0d0;"
        "   selection-background-color: #4CAF50;"
        "}"
        "QHeaderView::section {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   padding: 8px;"
        "   border: 1px solid #1976D2;"
        "   font-weight: bold;"
        "}"
    );

    mmLayout->addWidget(memoriaTable);
    tabs->addTab(mapaMemoriaTab, "Mapa de Memoria");
}

void MainWindow::setupSocket()
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newConnection);

    if (!server->listen(QHostAddress::Any, 5000)) {
        qDebug() << "Error al iniciar servidor:" << server->errorString();
    } else {
        qDebug() << "Servidor iniciado en puerto 5000";
    }
}

void MainWindow::newConnection()
{
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readData);
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    qDebug() << "Nueva conexión establecida";
}

void MainWindow::readData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    qDebug() << "Datos recibidos:" << data;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Error parsing JSON:" << error.errorString();
        return;
    }

    if (doc.isObject()) {
        QJsonObject json = doc.object();
        QMetaObject::invokeMethod(this, "updateUIFromJson", Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, json));
    }
}

void MainWindow::updateUIFromJson(const QJsonObject &json)
{
    // Actualizar métricas generales
    int usoMem = json["memoriaActual"].toInt();
    int asignAct = json["asignacionesActivas"].toInt();
    int leaks = json["memoryLeaksMB"].toInt();
    int maxUso = json["usoMaximo"].toInt();
    int totalAsig = json["totalAsignaciones"].toInt();

    metricasLabel->setText(QString(
        "Uso actual: %1 MB | Asignaciones activas: %2 | "
        "Memory leaks: %3 MB | Uso máximo: %4 MB | Total asignaciones: %5")
        .arg(usoMem).arg(asignAct).arg(leaks).arg(maxUso).arg(totalAsig));

    // Actualizar barras de progreso
    if (maxUso > 0) {
        usoMemoriaBar->setMaximum(maxUso);
        usoMemoriaBar->setValue(usoMem);
        leaksBar->setMaximum(maxUso);
        leaksBar->setValue(leaks);
    }

    // Actualizar gráfico de línea de tiempo
    memorySeries->append(currentTime, usoMem);

    // Mantener ventana deslizante de 60 segundos
    if (memorySeries->count() > 60) {
        memorySeries->remove(0);
        axisX->setRange(currentTime - 59, currentTime + 1);
    }

    // Ajustar escala Y si es necesario
    if (usoMem > axisY->max()) {
        axisY->setMax(usoMem + 10);
    }

    // Actualizar top 3 archivos
    resumenList->clear();
    QJsonArray top = json["topArchivos"].toArray();
    for (int i = 0; i < top.size() && i < 3; ++i) {
        QJsonObject obj = top[i].toObject();
        QString texto = QString("📁 %1 - %2 asignaciones - %3 MB")
                       .arg(obj["archivo"].toString())
                       .arg(obj["asignaciones"].toInt())
                       .arg(obj["mb"].toInt());
        resumenList->addItem(texto);
    }

    // Actualizar mapa de memoria
    QJsonArray bloques = json["bloquesMemoria"].toArray();
    memoriaTable->setRowCount(bloques.size());

    for (int i = 0; i < bloques.size(); ++i) {
        QJsonObject b = bloques[i].toObject();

        memoriaTable->setItem(i, 0, new QTableWidgetItem(b["direccion"].toString()));
        memoriaTable->setItem(i, 1, new QTableWidgetItem(b["tipo"].toString()));
        memoriaTable->setItem(i, 2, new QTableWidgetItem(QString::number(b["tamano"].toInt())));

        QString estado = b["estado"].toString();
        QTableWidgetItem *estadoItem = new QTableWidgetItem(estado);

        // Colores para diferentes estados
        if (estado == "Libre") {
            estadoItem->setBackground(QColor("#4CAF50")); // Verde
            estadoItem->setForeground(QColor("white"));
        } else if (estado == "Usado") {
            estadoItem->setBackground(QColor("#f44336")); // Rojo
            estadoItem->setForeground(QColor("white"));
        } else if (estado == "Leak") {
            estadoItem->setBackground(QColor("#FF9800")); // Naranja
            estadoItem->setForeground(QColor("white"));
        } else {
            estadoItem->setBackground(QColor("#FFC107")); // Amarillo
            estadoItem->setForeground(QColor("black"));
        }

        memoriaTable->setItem(i, 3, estadoItem);
    }

    // Ajustar columnas
    memoriaTable->resizeColumnsToContents();
}

void MainWindow::incrementTime()
{
    currentTime++;
}
