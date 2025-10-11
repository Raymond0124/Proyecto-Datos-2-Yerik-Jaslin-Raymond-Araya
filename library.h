#ifndef MEMORY_H
#define MEMORY_H

#include <map>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <QJsonObject>

// ---------------- Info de cada asignación ----------------
struct AllocationInfo {
    size_t size;
    std::string file;
    int line;
    std::string type;
    std::string dataPreview;  // ✅ NUEVO: Vista previa del contenido
    void* address;            // ✅ NUEVO: Dirección para referencia
    std::chrono::steady_clock::time_point timestamp;

    AllocationInfo(size_t s = 0, const std::string& f = "", int l = 0, const std::string& t = "unknown")
        : size(s), file(f), line(l), type(t), dataPreview(""), address(nullptr),
          timestamp(std::chrono::steady_clock::now()) {}
};

// ---------------- Variables globales ----------------
extern std::map<void*, AllocationInfo> allocations;
extern std::map<std::string, size_t> fileAllocCount;
extern std::map<std::string, size_t> fileAllocBytes;
extern std::mutex allocMutex;
extern std::atomic<bool> socketConnected;

// ---------------- Macros mejorados para tipos específicos ----------------
#define NEW_INT new(__FILE__, __LINE__, "int")
#define NEW_BOOL new(__FILE__, __LINE__, "bool")
#define NEW_CHAR new(__FILE__, __LINE__, "char")
#define NEW_FLOAT new(__FILE__, __LINE__, "float")
#define NEW_DOUBLE new(__FILE__, __LINE__, "double")
#define NEW_STRING new(__FILE__, __LINE__, "string")
#define NEW_STRUCT(name) new(__FILE__, __LINE__, #name)

#define NEW_ARRAY_INT new[](__FILE__, __LINE__, "int[]")
#define NEW_ARRAY_BOOL new[](__FILE__, __LINE__, "bool[]")
#define NEW_ARRAY_CHAR new[](__FILE__, __LINE__, "char[]")
#define NEW_ARRAY_FLOAT new[](__FILE__, __LINE__, "float[]")
#define NEW_ARRAY_DOUBLE new[](__FILE__, __LINE__, "double[]")

// ---------------- Sobrecarga de new/delete con tipo ----------------
void* operator new(size_t size, const char* file, int line, const char* type);
void* operator new[](size_t size, const char* file, int line, const char* type);
void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, const char* file, int line) noexcept;
void operator delete[](void* ptr, const char* file, int line) noexcept;

// ---------------- Helper para actualizar vista previa ----------------
void updateDataPreview(void* ptr, const std::string& typeHint = "");

// ---------------- Funciones de reporte ----------------
void printMemoryReport();
void printLeakReport(const std::string &pathFilter = "");
void sendLeakReport();

// ---------------- Exportar estado a JSON ----------------
QJsonObject exportMemoryState();

// ---------------- Conexión GUI (sockets nativos) ----------------
void connectToGui(const std::string &host = "127.0.0.1", int port = 5000);
void disconnectFromGui();
void sendMemoryUpdate();

#endif // MEMORY_H
