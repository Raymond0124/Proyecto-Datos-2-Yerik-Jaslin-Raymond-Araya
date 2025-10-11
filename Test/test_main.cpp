#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <random>
#include "library.h"

// ✅ CORRECCIÓN: Definir la macro ANTES de cualquier uso de new
#define new new(__FILE__, __LINE__)

int main() {
    std::cout << "=== Iniciando Memory Profiler Test Ampliado ===" << std::endl;

    // Conectar al GUI del profiler
    connectToGui("127.0.0.1", 5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    const int duration_seconds = 30;
    const int cycles_per_second = 10;
    const int total_cycles = duration_seconds * cycles_per_second;

    std::mt19937_64 rng(static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count()));

    std::uniform_int_distribution<int> smallDist(8, 64);
    std::uniform_int_distribution<int> mediumDist(128, 512);
    std::uniform_int_distribution<int> largeDist(1024, 4096);
    std::uniform_int_distribution<int> typeDist(0, 6);
    std::uniform_int_distribution<int> choice(0, 99);

    std::vector<void*> tempAllocations;
    tempAllocations.reserve(1000);

    std::cout << "Iniciando ciclo de " << duration_seconds << " segundos..." << std::endl;

    for (int cycle = 0; cycle < total_cycles; ++cycle) {
        for (int i = 0; i < 8; ++i) {
            int r = choice(rng);

            if (r < 60) {
                // 🔹 60% asignaciones pequeñas liberadas inmediatamente
                int t = typeDist(rng);
                switch (t) {
                    case 0: {
                        bool* b = new bool(true);
                        delete b;
                        break;
                    }
                    case 1: {
                        int* x = new int(42);
                        delete x;
                        break;
                    }
                    case 2: {
                        char* c = new char('Z');
                        delete c;
                        break;
                    }
                    case 3: {
                        float* f = new float(3.14f);
                        delete f;
                        break;
                    }
                    case 4: {
                        double* d = new double(2.71828);
                        delete d;
                        break;
                    }
                    case 5: {
                        long* l = new long(1234567);
                        delete l;
                        break;
                    }
                    case 6: {
                        int size = smallDist(rng);
                        char* arr = new char[size];
                        arr[0] = 'A';
                        delete[] arr;
                        break;
                    }
                }

            } else if (r < 85) {
                // 🔹 25% asignaciones medianas temporales
                int size = mediumDist(rng);
                char* p = new char[size];
                p[0] = 'M';
                tempAllocations.push_back(p);

            } else if (r < 95) {
                // 🔹 10% asignaciones grandes temporales
                int size = largeDist(rng);
                char* p = new char[size];
                p[0] = 'L';
                tempAllocations.push_back(p);

            } else {
                // 🔸 5% Memory leak intencional
                int t = typeDist(rng);
                switch (t) {
                    case 0: new bool(false); break;
                    case 1: new int(999); break;
                    case 2: new char('X'); break;
                    case 3: new float(9.81f); break;
                    case 4: new double(1.618); break;
                    case 5: new long(987654); break;
                    case 6: {
                        int size = smallDist(rng);
                        new char[size]; // intencionalmente no se libera
                        break;
                    }
                }
                std::cout << "[LEAK INTENCIONAL] tipo primitivo o array no liberado en línea "
                          << __LINE__ << std::endl;
            }
        }

        // 🔁 Liberar parte de las asignaciones temporales
        if (!tempAllocations.empty()) {
            size_t toFree = std::min<size_t>(tempAllocations.size(), 3);
            for (size_t k = 0; k < toFree; ++k) {
                delete[] static_cast<char*>(tempAllocations.back());
                tempAllocations.pop_back();
            }
        }

        // Mostrar progreso cada segundo
        if (cycle % cycles_per_second == 0) {
            std::cout << "⏱️  Segundo " << (cycle / cycles_per_second)
                      << " | Asignaciones temporales: " << tempAllocations.size() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / cycles_per_second));
    }

    // 🔚 Fin del test
    std::cout << "\n=== Fin del ciclo de pruebas ===" << std::endl;
    std::cout << "Asignaciones temporales restantes: " << tempAllocations.size() << std::endl;

    // Reportes
    std::cout << "\n--- Reporte de Memoria ---" << std::endl;
    printMemoryReport();

    std::cout << "\n--- Reporte de Leaks ---" << std::endl;
    printLeakReport();

    std::cout << "\nEnviando reporte de leaks al GUI..." << std::endl;
    sendLeakReport();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 🔓 Liberar toda la memoria temporal
    std::cout << "\nLiberando asignaciones temporales..." << std::endl;
    for (void* p : tempAllocations)
        delete[] static_cast<char*>(p);
    tempAllocations.clear();

    sendMemoryUpdate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    disconnectFromGui();

    std::cout << "=== Test finalizado ===" << std::endl;
    return 0;
}
