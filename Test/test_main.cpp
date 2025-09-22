#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>

#include "library.h"

// 👇 Esto hace que TODAS las llamadas a new incluyan file y line
#define new new(__FILE__, __LINE__)

int main() {
    // 1. Conectarse a la GUI
    connectToGui("127.0.0.1", 5000);

    // 2. Generar asignaciones variadas
    int* a = new int;                 // liberado más tarde
    double* b = new double[5];        // liberado más tarde
    char* c = new char[20];           // leak intencional (no se libera)
    std::vector<int>* vec = new std::vector<int>(10, 42); // liberado

    // 3. Enviar estado a la GUI
    sendMemoryUpdate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Liberar algunas asignaciones
    delete a;
    delete[] b;
    delete vec;
    // ⚠️ char* c no se libera → leak
    sendMemoryUpdate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 5. Generar más fugas
    int* leak1 = new int[100];    // leak
    char* leak2 = new char[50];   // leak
    sendMemoryUpdate();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 6. Reportes finales
    printMemoryReport();
    printLeakReport(); // ✅ Ahora debe mostrar fugas (c, leak1, leak2)
    sendLeakReport();


    // Mantener el programa vivo un poco para que GUI reciba datos
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
