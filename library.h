#ifndef MEMORY_H
#define MEMORY_H

#include <map>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <QJsonObject>
#include <QTcpSocket>

// ---------------- Info de cada asignación ----------------
struct AllocationInfo {
    size_t size;
    std::string file;
    int line;
    std::chrono::steady_clock::time_point timestamp;

    AllocationInfo(size_t s = 0, const std::string& f = "", int l = 0)
        : size(s), file(f), line(l), timestamp(std::chrono::steady_clock::now()) {}
};

// ---------------- Variables globales ----------------
extern std::map<void*, AllocationInfo> allocations;
extern std::map<std::string, size_t> fileAllocCount;
extern std::map<std::string, size_t> fileAllocBytes;
extern std::mutex allocMutex;

// Variables de socket
extern std::atomic<bool> socketConnected;
extern QTcpSocket* clientSocket;



// ---------------- Sobrecarga de new/delete ----------------
void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

// ---------------- Funciones de reporte ----------------
void printMemoryReport();
// library.h
void printLeakReport(const std::string &pathFilter = "");

void sendLeakReport();

// ---------------- Exportar estado a JSON ----------------
QJsonObject exportMemoryState();

// ---------------- Conexión GUI ----------------
void connectToGui(const std::string &host = "127.0.0.1", int port = 5000);
void sendMemoryUpdate();


// ⚠️ Opcional: mantener la versión vieja si la quieres
//void sendMemoryStateToGUI(const std::string &host = "127.0.0.1", int port = 5000);

#endif // MEMORY_H
