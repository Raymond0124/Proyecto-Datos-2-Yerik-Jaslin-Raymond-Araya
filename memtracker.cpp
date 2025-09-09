#include "memtracker.h"
#include <iostream>
#include <cstdlib>   // malloc, free
#include <new>       // std::bad_alloc

// Definiciones de las variables globales
std::map<void*, AllocationInfo> allocations;
std::map<std::string, size_t> fileAllocCount;
std::map<std::string, size_t> fileAllocBytes;
std::mutex allocMutex;

// Sobrecarga de operator new
void* operator new(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();

    std::lock_guard<std::mutex> lock(allocMutex);
    allocations[ptr] = AllocationInfo(size, file, line, std::chrono::steady_clock::now());
    fileAllocCount[file]++;
    fileAllocBytes[file] += size;
    return ptr;
}

// Sobrecarga de operator delete
void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(allocMutex);
    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
        allocations.erase(it);
    }
    free(ptr);
}

// Reporte de asignaciones por archivo
void printMemoryReport() {
    std::cout << "\n=== Reporte de Asignaciones por Archivo ===\n";
    for (auto& [file, count] : fileAllocCount) {
        std::cout << "Archivo: " << file
                  << " | Asignaciones: " << count
                  << " | Memoria total: " << fileAllocBytes[file] << " bytes\n";
    }
}

// Reporte de memory leaks
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
