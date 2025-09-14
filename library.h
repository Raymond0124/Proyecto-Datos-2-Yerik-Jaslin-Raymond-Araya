#ifndef PROYECTO_DATOS_2_LIBRARY_H
#define PROYECTO_DATOS_2_LIBRARY_H

#ifndef LIBRARY_H
#define LIBRARY_H

#include <map>
#include <string>
#include <chrono>
#include <mutex>
#include <QJsonObject>


// Información de cada asignación
struct AllocationInfo {
    size_t size;
    std::string file;
    int line;
    std::chrono::steady_clock::time_point timestamp;

    AllocationInfo(size_t s = 0, const std::string& f = "", int l = 0)
        : size(s), file(f), line(l), timestamp(std::chrono::steady_clock::now()) {}
};

// Variables globales
extern std::map<void*, AllocationInfo> allocations;
extern std::map<std::string, size_t> fileAllocCount;
extern std::map<std::string, size_t> fileAllocBytes;
extern std::mutex allocMutex;

// Funciones de reporte
void printMemoryReport();
void printLeakReport();

// Exportar estado de memoria a JSON
QJsonObject exportMemoryState();

// Enviar datos a la GUI vía socket TCP
void sendMemoryStateToGUI(const std::string &host = "127.0.0.1", int port = 5000);

#endif // LIBRARY_H



#endif // PROYECTO_DATOS_2_LIBRARY_H