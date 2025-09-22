#include "library.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <QJsonArray>
#include <QJsonDocument>
#include <thread>
#include <filesystem>

// Variables globales
std::map<void*, AllocationInfo> allocations;
std::map<std::string, size_t> fileAllocCount;
std::map<std::string, size_t> fileAllocBytes;
std::mutex allocMutex;

// Variables de socket
std::atomic<bool> socketConnected(false);
QTcpSocket* clientSocket = nullptr;



// -------- Sobrecarga de operadores --------
void* operator new(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations[ptr] = AllocationInfo{size, file, line};
        fileAllocCount[file]++;
        fileAllocBytes[file] += size;
    }
    sendMemoryUpdate();
    return ptr;
}

void* operator new[](size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations[ptr] = AllocationInfo{size, file, line};
        fileAllocCount[file]++;
        fileAllocBytes[file] += size;
    }
    sendMemoryUpdate();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
    }
    free(ptr);
    sendMemoryUpdate();
}

void operator delete[](void* ptr) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
    }
    free(ptr);
    sendMemoryUpdate();
}



// Para emparejar con operator new(size_t, const char*, int)
void operator delete(void* ptr, const char* file, int line) noexcept {
    if (!ptr) return;
        {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
        }
    free(ptr);
}

// Para emparejar con operator new[](size_t, const char*, int)
void operator delete[](void* ptr, const char* file, int line) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        allocations.erase(ptr);
    }
    free(ptr);
}




// -------- Reportes --------
void printMemoryReport() {
    std::cout << "\n=== Reporte de Asignaciones por Archivo ===\n";
    for (auto& [file, count] : fileAllocCount) {
        std::cout << "Archivo: " << file
                  << " | Asignaciones: " << count
                  << " | Memoria total: " << fileAllocBytes[file] << " bytes\n";
    }
}


#include <iomanip>


void printLeakReport(const std::string &pathFilter )
{
    std::lock_guard<std::mutex> lock(allocMutex);

    if (allocations.empty()) {
        std::cout << "[Leak Report] ✅ No se detectaron fugas de memoria.\n";
        return;
    }

    // Agrupar por archivo (opcionalmente filtrado)
    std::map<std::string, std::vector<std::pair<void*, AllocationInfo>>> byFile;
    size_t totalLeaked = 0;
    for (auto const &p : allocations) {
        const void* ptr = p.first;
        const AllocationInfo &info = p.second;
        std::string file = info.file;
        if (!pathFilter.empty() && file.find(pathFilter) == std::string::npos) {
            // Si se especifica filtro, ignorar archivos que no contengan el filtro
            continue;
        }
        byFile[file].push_back({(void*)ptr, info});
        totalLeaked += info.size;
    }

    if (byFile.empty()) {
        std::cout << "[Leak Report] (tras filtrar) ✅ No quedan fugas visibles.\n";
        return;
    }

    std::cout << "\n[Leak Report] ❌ Se detectaron " << allocations.size() << " fugas ("
              << totalLeaked << " bytes en total). Mostrando por archivo:\n\n";

    for (auto &entry : byFile) {
        const std::string &fullpath = entry.first;
        std::string fname;
        try {
            fname = std::filesystem::path(fullpath).filename().string();
        } catch (...) {
            fname = fullpath; // fallback
        }
        std::cout << "Archivo: " << fname << " (ruta: " << fullpath << ")\n";
        for (auto &pr : entry.second) {
            void* addr = pr.first;
            const AllocationInfo &info = pr.second;
            std::cout << "  - Dirección: " << std::setw(14) << std::hex << std::showbase
                      << (uintptr_t)addr << std::dec << std::noshowbase
                      << " | Tamaño: " << info.size << " bytes"
                      << " | Línea: " << info.line << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Total de memoria filtrada (reportada): " << totalLeaked << " bytes\n";
}

void sendLeakReport() {
    QJsonArray leaksArray;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (const auto& [ptr, info] : allocations) {
            QJsonObject leakObj;
            leakObj["direccion"] = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0'));
            leakObj["tamano"] = static_cast<int>(info.size);
            leakObj["archivo"] = QString::fromStdString(info.file);
            leakObj["linea"] = info.line;
            leaksArray.append(leakObj);
        }
    }

    QJsonObject root;
    root["type"] = "leaks";   // 👈 para diferenciar en la GUI
    root["data"] = leaksArray;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write(jsonData);
        clientSocket->flush();
    }
}





// -------- Exportar JSON --------
QJsonObject exportMemoryState() {
    QJsonObject json;
    size_t memoriaActual = 0;

    QJsonArray topArchivos;
    QJsonArray bloques;

    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (auto& [_, info] : allocations) memoriaActual += info.size;
        for (auto& [file, bytes] : fileAllocBytes) {
            QJsonObject obj;
            obj["file"] = QString::fromStdString(file);
            obj["size"] = static_cast<int>(bytes);
            topArchivos.append(obj);
        }
        for (auto& [ptr, info] : allocations) {
            QJsonObject obj;
            obj["direccion"] = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE*2, 16, QLatin1Char('0'));
            obj["tam"] = static_cast<int>(info.size);
            obj["archivo"] = QString::fromStdString(info.file);
            bloques.append(obj);
        }
    }

    json["memoriaActual"] = static_cast<int>(memoriaActual);
    json["leaks"] = static_cast<int>(memoriaActual);
    json["topArchivos"] = topArchivos;
    json["bloques"] = bloques;
    return json;
}

// -------- Conexión GUI --------
void connectToGui(const std::string &host, int port) {
    if (!clientSocket) clientSocket = new QTcpSocket();
    clientSocket->connectToHost(QString::fromStdString(host), port);
    socketConnected = clientSocket->waitForConnected(3000);
    if (!socketConnected) {
        std::cerr << "No se pudo conectar al GUI\n";
    }
}

void sendMemoryUpdate() {
    QJsonObject json = exportMemoryState();
    json["type"] = "memory";  // 👈 diferencia

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    std::thread([data]() {
        if (clientSocket && socketConnected) {
            clientSocket->write(data);
            clientSocket->flush();
        }
    }).detach();
}
