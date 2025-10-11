#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QPainter>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHeaderView>
#include <QListWidget>
#include <QtCharts>
#include <QDialog>
#include <QFormLayout>
#include <QTextEdit>
#include <QPushButton>

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentTime(0), clientSocket(nullptr)
{
    this->resize(1200, 800);
    this->setWindowTitle("Memory Profiler - TEC");

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

// ✅ NUEVO: Mostrar diálogo con detalles del bloque
void MainWindow::showBlockDetails(int row) {
    if (row < 0 || row >= memoriaTable->rowCount()) return;

    QString direccion = memoriaTable->item(row, 0)->text();
    QString tipo = memoriaTable->item(row, 1)->text();
    QString tamano = memoriaTable->item(row, 2)->text();
    QString estado = memoriaTable->item(row, 3)->text();
    QString linea = memoriaTable->item(row, 4)->text();

    // Buscar info adicional en blockDetailsMap
    QString archivo = "Desconocido";
    QString dataPreview = "No disponible";

    if (blockDetailsMap.contains(direccion)) {
        QJsonObject details = blockDetailsMap[direccion];
        archivo = details["archivo"].toString("Desconocido");
        dataPreview = details["dataPreview"].toString("No disponible");
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Detalles del Bloque de Memoria");
    dialog->resize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    QLabel *dirLabel = new QLabel(direccion);
    dirLabel->setStyleSheet("font-family: monospace; font-weight: bold; color: #2196F3;");
    formLayout->addRow("Dirección:", dirLabel);

    QLabel *tipoLabel = new QLabel(tipo);
    tipoLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    formLayout->addRow("Tipo de Dato:", tipoLabel);

    QLabel *tamLabel = new QLabel(tamano + " bytes");
    tamLabel->setStyleSheet("font-weight: bold;");
    formLayout->addRow("Tamaño:", tamLabel);

    QLabel *estadoLabel = new QLabel(estado);
    estadoLabel->setStyleSheet("color: " + QString(estado == "Activo" ? "#4CAF50" : "#f44336") + ";");
    formLayout->addRow("Estado:", estadoLabel);

    QLabel *archivoLabel = new QLabel(archivo);
    archivoLabel->setWordWrap(true);
    formLayout->addRow("Archivo:", archivoLabel);

    QLabel *lineaLabel = new QLabel(linea);
    formLayout->addRow("Línea:", lineaLabel);

    layout->addLayout(formLayout);

    // Vista previa de datos
    QLabel *previewTitle = new QLabel("Vista Previa del Contenido:");
    previewTitle->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(previewTitle);

    QTextEdit *previewText = new QTextEdit();
    previewText->setReadOnly(true);
    previewText->setStyleSheet("background-color: #f5f5f5; border: 1px solid #ddd; padding: 10px; font-family: monospace;");
    previewText->setText(dataPreview);
    previewText->setMaximumHeight(150);
    layout->addWidget(previewText);

    QPushButton *closeBtn = new QPushButton("Cerrar");
    closeBtn->setStyleSheet("padding: 8px 20px; background-color: #2196F3; color: white; border: none; border-radius: 4px;");
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    dialog->exec();
}

// ----- Setup Vista General -----
void MainWindow::setupVistaGeneral()
{
    vistaGeneralTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(vistaGeneralTab);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    metricasLabel = new QLabel("Esperando datos del memory profiler...", this);
    metricasLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 1px solid #cccccc; padding: 10px; font-weight: bold; font-size: 12px; }");
    metricasLabel->setFixedHeight(60);
    layout->addWidget(metricasLabel);

    QHBoxLayout *barsLayout = new QHBoxLayout();
    usoMemoriaBar = new QProgressBar();
    usoMemoriaBar->setFormat("Uso Memoria: %v MB");
    usoMemoriaBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    usoMemoriaBar->setMinimum(0);
    usoMemoriaBar->setMaximum(100);

    leaksBar = new QProgressBar();
    leaksBar->setFormat("Memory Leaks: %v MB");
    leaksBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
    leaksBar->setMinimum(0);
    leaksBar->setMaximum(100);

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
    axisY->setLabelFormat("%.2f");
    axisY->setRange(0, 10);

    chart->addSeries(memorySeries);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    memorySeries->attachAxis(axisX);
    memorySeries->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView, 1);

    QLabel *resumenTitulo = new QLabel("Top Archivos con Mayores Asignaciones:");
    resumenTitulo->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(resumenTitulo);

    resumenList = new QListWidget();
    resumenList->setMaximumHeight(120);
    layout->addWidget(resumenList);

    tabs->addTab(vistaGeneralTab, "Vista General");
}

// ----- Setup Mapa de Memoria -----
void MainWindow::setupMapaMemoria()
{
    mapaMemoriaTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(mapaMemoriaTab);
    layout->setContentsMargins(10,10,10,10);

    QLabel *mapaTitulo = new QLabel("Mapa de Memoria - Bloques Asignados (Click en dirección para ver detalles)");
    mapaTitulo->setStyleSheet("font-weight: bold; font-size: 16px; padding: 10px;");
    layout->addWidget(mapaTitulo);

    memoriaTable = new QTableWidget(0, 5, this);
    memoriaTable->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Tipo de Dato" << "Tamaño (bytes)" << "Estado" << "Línea");
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

    // ✅ Conectar señal de doble clic
    connect(memoriaTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::showBlockDetails);

    layout->addWidget(memoriaTable);
    tabs->addTab(mapaMemoriaTab, "Mapa de Memoria");
}

// ----- Setup Memory Tab -----
void MainWindow::setupMemoryTab()
{
    memoryTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(memoryTab);
    layout->setContentsMargins(10,10,10,10);

    QLabel *label = new QLabel("Asignación de Memoria por Archivo Fuente");
    label->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(label);

    memoryByFileTable = new QTableWidget(0, 3, memoryTab);
    memoryByFileTable->setHorizontalHeaderLabels(QStringList() << "Archivo" << "Asignaciones" << "Bytes");
    memoryByFileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    memoryByFileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    memoryByFileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    memoryByFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryByFileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoryByFileTable->setAlternatingRowColors(true);
    layout->addWidget(memoryByFileTable);

    tabs->addTab(memoryTab, "Asignación por Archivo");
}

// ----- Setup Leaks Tab -----
void MainWindow::setupLeaksTab()
{
    leaksTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(leaksTab);
    layout->setContentsMargins(10,10,10,10);

    QLabel *label = new QLabel("Memory Leaks (se actualizará con datos recibidos)");
    label->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(label);

    tabs->addTab(leaksTab, "Memory Leaks");
}

// ----- Timer -----
void MainWindow::incrementTime()
{
    currentTime++;
    axisX->setRange(std::max(0, currentTime - 60), currentTime);
}

// ----- Socket -----
void MainWindow::setupSocket()
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    if(!server->listen(QHostAddress::Any, 5000)) {
        qDebug() << "❌ No se pudo iniciar el servidor en puerto 5000";
    } else {
        qDebug() << "✅ Servidor iniciado en puerto 5000";
    }
}

void MainWindow::newConnection()
{
    clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readData);
    qDebug() << "✅ Cliente conectado";
}

// Limpia de forma segura widgets/layouts del leaksTab
void MainWindow::updateLeaksTab(const QJsonArray &leaks)
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(leaksTab->layout());
    if (!layout) return;

    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        } else if (QLayout* l = item->layout()) {
            QLayoutItem* it;
            while ((it = l->takeAt(0)) != nullptr) {
                if (QWidget* sw = it->widget()) sw->deleteLater();
                delete it;
            }
            delete l;
        }
        delete item;
    }

    QLabel *title = new QLabel(QString("Lista de Memory Leaks detectados: %1 total").arg(leaks.size()));
    title->setStyleSheet("font-weight: bold; font-size: 14px; color: #d32f2f;");
    layout->addWidget(title);

    if (leaks.isEmpty()) {
        QLabel *noLeaks = new QLabel("✅ No se detectaron memory leaks");
        noLeaks->setStyleSheet("color: #4CAF50; font-size: 12px; padding: 20px;");
        layout->addWidget(noLeaks);
        return;
    }

    QTableWidget *table = new QTableWidget(leaks.size(), 6, leaksTab);
    table->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Tamaño (bytes)" << "Tipo" << "Archivo" << "Línea" << "Valor");
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);

    int row = 0;
    for (auto val : leaks) {
        QJsonObject leak = val.toObject();
        table->setItem(row, 0, new QTableWidgetItem(leak["direccion"].toString()));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(leak["tamano"].toVariant().toLongLong())));
        table->setItem(row, 2, new QTableWidgetItem(leak["tipo"].toString("unknown")));
        table->setItem(row, 3, new QTableWidgetItem(leak["archivo"].toString()));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(leak["linea"].toInt())));
        table->setItem(row, 5, new QTableWidgetItem(leak["dataPreview"].toString("N/A")));
        row++;
    }

    layout->addWidget(table);
}

// ----- Lectura robusta (framing por líneas) -----
void MainWindow::readData()
{
    if (!clientSocket) return;

    readBuffer.append(clientSocket->readAll());

    while (true) {
        int newlineIndex = readBuffer.indexOf('\n');
        if (newlineIndex == -1) break;

        QByteArray line = readBuffer.left(newlineIndex);
        readBuffer.remove(0, newlineIndex + 1);

        if (line.trimmed().isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << err.errorString() << " data:" << line;
            continue;
        }

        if (!doc.isObject()) continue;
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString("memory");

        if (type == "memory") {
            updateUIFromJson(obj);
        } else if (type == "leaks") {
            updateLeaksTab(obj["data"].toArray());
        }
    }
}

// ----- Actualizar UI -----
void MainWindow::updateUIFromJson(const QJsonObject &json)
{
    qint64 memoriaActualMB = json["memoriaActual"].toVariant().toLongLong();
    qint64 leaksMB = json["leaks"].toVariant().toLongLong();

    metricasLabel->setText(QString("Memoria Actual: %1 MB | Leaks: %2 MB").arg(memoriaActualMB).arg(leaksMB));

    if (memoriaActualMB > axisY->max()) {
        double newMax = memoriaActualMB * 1.5;
        axisY->setRange(0, newMax);
        usoMemoriaBar->setMaximum(static_cast<int>(newMax));
        leaksBar->setMaximum(static_cast<int>(newMax));
    }

    usoMemoriaBar->setValue(static_cast<int>(memoriaActualMB));
    leaksBar->setValue(static_cast<int>(leaksMB));

    memorySeries->append(currentTime, memoriaActualMB);
    axisX->setRange(std::max(0, currentTime - 60), currentTime);

    resumenList->clear();
    QJsonArray topArchivos = json["topArchivos"].toArray();
    for (auto val : topArchivos) {
        QJsonObject obj = val.toObject();
        QString file = obj["file"].toString();
        qint64 size = obj["size"].toVariant().toLongLong();
        qint64 count = obj["count"].toVariant().toLongLong();
        resumenList->addItem(QString("%1 - %2 bytes (%3 asignaciones)").arg(file).arg(size).arg(count));
    }

    memoryByFileTable->setRowCount(0);
    int rowFile = 0;
    for (auto val : topArchivos) {
        QJsonObject obj = val.toObject();
        QString file = obj["file"].toString();
        qint64 size = obj["size"].toVariant().toLongLong();
        qint64 count = obj["count"].toVariant().toLongLong();
        memoryByFileTable->insertRow(rowFile);
        memoryByFileTable->setItem(rowFile, 0, new QTableWidgetItem(file));
        memoryByFileTable->setItem(rowFile, 1, new QTableWidgetItem(QString::number(count)));
        memoryByFileTable->setItem(rowFile, 2, new QTableWidgetItem(QString::number(size)));
        rowFile++;
    }

    // ✅ Limpiar mapa anterior de detalles
    blockDetailsMap.clear();

    memoriaTable->setRowCount(0);
    QJsonArray bloques = json["bloques"].toArray();
    int row = 0;
    for (auto val : bloques) {
        QJsonObject obj = val.toObject();
        QString direccion = obj.value("direccion").toString("0x0");
        QString tipo = obj.value("tipo").toString("unknown");
        qint64 tam = obj.value("tam").toVariant().toLongLong();
        QString estado = obj.value("estado").toString("Desconocido");
        int linea = obj.value("linea").toInt(0);
        QString dataPreview = obj.value("dataPreview").toString("N/A");

        // ✅ Guardar detalles completos
        blockDetailsMap[direccion] = obj;

        memoriaTable->insertRow(row);
        QTableWidgetItem *addrItem = new QTableWidgetItem(direccion);
        QTableWidgetItem *typeItem = new QTableWidgetItem(tipo);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(QString::number(tam));
        QTableWidgetItem *stateItem = new QTableWidgetItem(estado);
        QTableWidgetItem *lineItem = new QTableWidgetItem(QString::number(linea));

        QColor bgColor;
        if (tam < 64) {
            bgColor = QColor(200, 255, 200);
        } else if (tam < 1024) {
            bgColor = QColor(255, 255, 200);
        } else {
            bgColor = QColor(255, 200, 200);
        }

        addrItem->setBackground(QBrush(bgColor));
        typeItem->setBackground(QBrush(bgColor));
        sizeItem->setBackground(QBrush(bgColor));
        stateItem->setBackground(QBrush(bgColor));
        lineItem->setBackground(QBrush(bgColor));

        // ✅ Hacer la dirección clickeable (cursor pointer)
        addrItem->setToolTip("Doble clic para ver detalles");
        QFont font = addrItem->font();
        font.setUnderline(true);
        addrItem->setFont(font);
        addrItem->setForeground(QColor("#2196F3"));

        memoriaTable->setItem(row, 0, addrItem);
        memoriaTable->setItem(row, 1, typeItem);
        memoriaTable->setItem(row, 2, sizeItem);
        memoriaTable->setItem(row, 3, stateItem);
        memoriaTable->setItem(row, 4, lineItem);
        row++;
    }
}