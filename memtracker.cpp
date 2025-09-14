#include "memtracker.h"
#include "library.h"  // Trae AllocationInfo, allocations, fileAllocCount, fileAllocBytes, allocMutex
#include <QJsonDocument>
#include <thread>
#include <iostream>

// Variables de socket
std::atomic<bool> socketConnected(false);
QTcpSocket* clientSocket = nullptr;

// ---------------- Conexión socket ----------------
void connectToGui(const std::string &host, int port) {
    if (!clientSocket) clientSocket = new QTcpSocket();
    clientSocket->connectToHost(QString::fromStdString(host), port);
    socketConnected = clientSocket->waitForConnected(3000);
    if (!socketConnected) {
        std::cerr << "No se pudo conectar al GUI\n";
    }
}

// ---------------- Enviar actualización ----------------
void sendMemoryUpdate() {
    if (!socketConnected || !clientSocket) return;

    QJsonObject json;
    size_t memoriaActual = 0;

    QJsonArray topArchivos;
    QJsonArray bloques;

    {
        std::lock_guard<std::mutex> lock(allocMutex);

        for (auto &[ptr, info] : allocations)
            memoriaActual += info.size;

        for (auto &[file, bytes] : fileAllocBytes) {
            QJsonObject obj;
            obj["file"] = QString::fromStdString(file);
            obj["size"] = static_cast<int>(bytes);
            topArchivos.append(obj);
        }

        for (auto &[ptr, info] : allocations) {
            QJsonObject obj;
            obj["direccion"] = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QLatin1Char('0'));
            obj["tam"] = static_cast<int>(info.size);
            obj["archivo"] = QString::fromStdString(info.file);
            bloques.append(obj);
        }
    }

    json["memoriaActual"] = static_cast<int>(memoriaActual);
    json["leaks"] = static_cast<int>(memoriaActual);
    json["topArchivos"] = topArchivos;
    json["bloques"] = bloques;

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // Enviar en hilo aparte
    std::thread([data]() {
        if (clientSocket && socketConnected) {
            clientSocket->write(data);
            clientSocket->flush();
        }
    }).detach();
}
