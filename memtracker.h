#ifndef MEMTRACKER_H
#define MEMTRACKER_H

#include <map>
#include <string>
#include <chrono>
#include <mutex>

// Información de cada asignación
struct AllocationInfo {
    size_t size;
    std::string file;
    int line;
    std::chrono::steady_clock::time_point timestamp;

    AllocationInfo(size_t s, const std::string& f, int l,
                   std::chrono::steady_clock::time_point t)
        : size(s), file(f), line(l), timestamp(t) {}

    AllocationInfo() : size(0), line(0), timestamp(std::chrono::steady_clock::now()) {}
};

// Variables globales
extern std::map<void*, AllocationInfo> allocations;
extern std::map<std::string, size_t> fileAllocCount;
extern std::map<std::string, size_t> fileAllocBytes;
extern std::mutex allocMutex;

// Funciones de reporte
void printMemoryReport();
void printLeakReport();

#endif // MEMTRACKER_H
