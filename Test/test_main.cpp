#include "memtracker.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>

#include "library.h"

int main() {
    // 1. Conectarse a la GUI
    connectToGui("127.0.0.1", 5000);

    // 2. Generar asignaciones variadas
    int* a = new int;                 // 4 bytes (int)
    double* b = new double[5];        // 40 bytes aprox
    char* c = new char[20];           // 20 bytes
    std::vector<int>* vec = new std::vector<int>(10, 42);

    // 3. Enviar estado a la GUI
    sendMemoryUpdate();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Liberar algunas asignaciones
    delete a;
    delete[] b;
    sendMemoryUpdate();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 5. Generar más bloques
    for (int i = 0; i < 5; i++) {
        new int[100 + i * 50];  // bloques variados, algunos sin liberar para generar leaks
    }

    sendMemoryUpdate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 6. Liberar vector
    delete vec;
    sendMemoryUpdate();

    // 7. Generar leaks intencionales
    char* leak1 = new char[30];
    int* leak2 = new int[50];

    sendMemoryUpdate();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 8. Reportes finales por consola (opcional)
    printMemoryReport();
    printLeakReport();

    // Mantener el programa vivo un poco para que GUI reciba datos
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
