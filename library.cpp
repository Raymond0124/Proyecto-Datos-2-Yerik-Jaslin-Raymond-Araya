#include "library.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <QJsonArray>
#include <QTcpSocket>
#include <QJsonDocument>
#include <thread>

// Variables globales
std::map<void*, AllocationInfo> allocations;
std::map<std::string, size_t> fileAllocCount;
std::map<std::string, size_t> fileAllocBytes;
std::mutex allocMutex;

// ---------------- Sobrecarga de operadores ----------------
void* operator new(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();

    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations[ptr] = AllocationInfo(size, file, line);
        fileAllocCount[file]++;
        fileAllocBytes[file] += size;
    }

    sendMemoryStateToGUI();
    return ptr;
}

void* operator new[](size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();

    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations[ptr] = AllocationInfo(size, file, line);
        fileAllocCount[file]++;
        fileAllocBytes[file] += size;
    }

    sendMemoryStateToGUI();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
    }
    free(ptr);
    sendMemoryStateToGUI();
}

void operator delete[](void* ptr) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
    }
    free(ptr);
    sendMemoryStateToGUI();
}

// ---------------- Reportes ----------------
void printMemoryReport() {
    std::cout << "\n=== Reporte de Asignaciones por Archivo ===\n";
    for (auto& [file, count] : fileAllocCount) {
        std::cout << "Archivo: " << file
                  << " | Asignaciones: " << count
                  << " | Memoria total: " << fileAllocBytes[file] << " bytes\n";
    }
}

void printLeakReport() {
    size_t totalLeaks = 0;
    size_t biggestLeak = 0;
    std::string biggestLeakFile;
    std::map<std::string, size_t> leaksByFile;

    for (auto& [ptr, info] : allocations) {
        totalLeaks += info.size;
        leaksByFile[info.file] += info.size;
        if (info.size > biggestLeak) {
            biggestLeak = info.size;
            biggestLeakFile = info.file;
        }
    }

    std::cout << "\n=== Reporte de Memory Leaks ===\n";
    std::cout << "Total fugado: " << totalLeaks << " bytes\n";
    std::cout << "Leak más grande: " << biggestLeak << " bytes en " << biggestLeakFile << "\n";

    std::string worstFile;
    size_t worstLeak = 0;
    for (auto& [file, size] : leaksByFile) {
        if (size > worstLeak) {
            worstLeak = size;
            worstFile = file;
        }
    }
    std::cout << "Archivo con más leaks: " << worstFile << " (" << worstLeak << " bytes)\n";

    size_t totalAllocated = 0;
    for (auto& [_, b] : fileAllocBytes) totalAllocated += b;
    double leakRate = totalAllocated ? (double)totalLeaks / (double)totalAllocated : 0.0;
    std::cout << "Tasa de leaks: " << leakRate * 100 << "%\n";
}

// ---------------- Exportar JSON ----------------
QJsonObject exportMemoryState() {
    QJsonObject json;

    size_t memoriaActual = 0;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (auto& [_, info] : allocations) memoriaActual += info.size;
    }

    json["memoriaActual"] = static_cast<int>(memoriaActual / 1024); // KB
    json["leaks"] = static_cast<int>(memoriaActual / 1024);

    // Top archivos
    QJsonArray topArchivos;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (auto& [file, bytes] : fileAllocBytes) {
            QJsonObject obj;
            obj["file"] = QString::fromStdString(file);
            obj["size"] = static_cast<int>(bytes);
            topArchivos.append(obj);
        }
    }
    json["topArchivos"] = topArchivos;

    // Bloques
    QJsonArray bloques;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (auto& [ptr, info] : allocations) {
            QJsonObject obj;
            obj["direccion"] = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE*2, 16, QLatin1Char('0'));
            obj["tam"] = static_cast<int>(info.size);
            obj["archivo"] = QString::fromStdString(info.file);
            bloques.append(obj);
        }
    }
    json["bloques"] = bloques;

    return json;
}

// ---------------- Enviar datos TCP ----------------
void sendMemoryStateToGUI(const std::string &host, int port) {
    QJsonObject json = exportMemoryState();
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    std::thread([data, host, port]() {
        QTcpSocket socket;
        socket.connectToHost(QString::fromStdString(host), port);
        if (socket.waitForConnected(100)) {
            socket.write(data);
            socket.flush();
            socket.waitForBytesWritten(100);
            socket.disconnectFromHost();
        }
    }).detach();
}
