#include "library.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <QJsonArray>
#include <QJsonDocument>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    using SOCKET = int;
#endif

std::map<void*, AllocationInfo> allocations;
std::map<std::string, size_t> fileAllocCount;
std::map<std::string, size_t> fileAllocBytes;
std::mutex allocMutex;
std::atomic<bool> socketConnected(false);
static std::mutex socketMutex;
static SOCKET nativeSock = INVALID_SOCKET;

static void closeSocket() {
#ifdef _WIN32
    if (nativeSock != INVALID_SOCKET) closesocket(nativeSock);
#else
    if (nativeSock != INVALID_SOCKET) close(nativeSock);
#endif
    nativeSock = INVALID_SOCKET;
    socketConnected = false;
}

// ✅ Función para generar vista previa del contenido
std::string generateDataPreview(void* ptr, size_t size, const std::string& type) {
    if (!ptr || size == 0) return "null";

    std::stringstream ss;

    try {
        if (type == "int" && size >= sizeof(int)) {
            ss << *static_cast<int*>(ptr);
        }
        else if (type == "bool" && size >= sizeof(bool)) {
            ss << (*static_cast<bool*>(ptr) ? "true" : "false");
        }
        else if (type == "char" && size >= sizeof(char)) {
            char c = *static_cast<char*>(ptr);
            if (std::isprint(c)) ss << "'" << c << "'";
            else ss << "'\\x" << std::hex << (int)c << "'";
        }
        else if (type == "float" && size >= sizeof(float)) {
            ss << std::fixed << std::setprecision(2) << *static_cast<float*>(ptr);
        }
        else if (type == "double" && size >= sizeof(double)) {
            ss << std::fixed << std::setprecision(2) << *static_cast<double*>(ptr);
        }
        else if (type == "char[]" || type == "string") {
            char* str = static_cast<char*>(ptr);
            size_t maxLen = std::min(size, (size_t)50);
            ss << "\"";
            for (size_t i = 0; i < maxLen && str[i] != '\0'; i++) {
                if (std::isprint(str[i])) ss << str[i];
                else ss << ".";
            }
            if (size > 50) ss << "...";
            ss << "\"";
        }
        else if (type.find("[]") != std::string::npos) {
            // Arrays
            ss << "[";
            if (type.find("int") != std::string::npos && size >= sizeof(int)) {
                int* arr = static_cast<int*>(ptr);
                size_t count = std::min((size_t)5, size / sizeof(int));
                for (size_t i = 0; i < count; i++) {
                    ss << arr[i];
                    if (i < count - 1) ss << ", ";
                }
                if (size / sizeof(int) > 5) ss << "...";
            }
            else if (type.find("bool") != std::string::npos && size >= sizeof(bool)) {
                bool* arr = static_cast<bool*>(ptr);
                size_t count = std::min((size_t)5, size / sizeof(bool));
                for (size_t i = 0; i < count; i++) {
                    ss << (arr[i] ? "true" : "false");
                    if (i < count - 1) ss << ", ";
                }
                if (size / sizeof(bool) > 5) ss << "...";
            }
            else if (type.find("float") != std::string::npos && size >= sizeof(float)) {
                float* arr = static_cast<float*>(ptr);
                size_t count = std::min((size_t)5, size / sizeof(float));
                for (size_t i = 0; i < count; i++) {
                    ss << std::fixed << std::setprecision(2) << arr[i];
                    if (i < count - 1) ss << ", ";
                }
                if (size / sizeof(float) > 5) ss << "...";
            }
            else if (type.find("double") != std::string::npos && size >= sizeof(double)) {
                double* arr = static_cast<double*>(ptr);
                size_t count = std::min((size_t)5, size / sizeof(double));
                for (size_t i = 0; i < count; i++) {
                    ss << std::fixed << std::setprecision(2) << arr[i];
                    if (i < count - 1) ss << ", ";
                }
                if (size / sizeof(double) > 5) ss << "...";
            }
            ss << "]";
        }
        else {
            // Tipo desconocido - mostrar bytes en hex
            ss << "0x";
            size_t maxBytes = std::min(size, (size_t)16);
            unsigned char* bytes = static_cast<unsigned char*>(ptr);
            for (size_t i = 0; i < maxBytes; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
            }
            if (size > 16) ss << "...";
        }
    } catch (...) {
        ss << "<error reading data>";
    }

    return ss.str();
}

// ✅ Sobrecarga CON tipo específico
void* operator new(size_t size, const char* file, int line, const char* type) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        AllocationInfo info(size, file ? file : "unknown", line, type ? type : "unknown");
        info.address = ptr;
        info.dataPreview = generateDataPreview(ptr, size, type ? type : "unknown");
        allocations[ptr] = info;
        std::string fileKey = file ? file : "unknown";
        fileAllocCount[fileKey]++;
        fileAllocBytes[fileKey] += size;
    }
    sendMemoryUpdate();
    return ptr;
}

void* operator new[](size_t size, const char* file, int line, const char* type) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        AllocationInfo info(size, file ? file : "unknown", line, type ? type : "unknown");
        info.address = ptr;
        info.dataPreview = generateDataPreview(ptr, size, type ? type : "unknown");
        allocations[ptr] = info;
        std::string fileKey = file ? file : "unknown";
        fileAllocCount[fileKey]++;
        fileAllocBytes[fileKey] += size;
    }
    sendMemoryUpdate();
    return ptr;
}

// ✅ Sobrecarga SIN tipo (backward compatibility)
void* operator new(size_t size, const char* file, int line) {
    return operator new(size, file, line, "scalar");
}

void* operator new[](size_t size, const char* file, int line) {
    return operator new[](size, file, line, "array");
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        auto it = allocations.find(ptr);
        if (it != allocations.end()) {
            size_t bytesToSubtract = it->second.size;
            std::string file = it->second.file;
            fileAllocBytes[file] = (fileAllocBytes[file] >= bytesToSubtract) ? fileAllocBytes[file] - bytesToSubtract : 0;
            if (fileAllocCount[file] > 0) fileAllocCount[file]--;
            allocations.erase(it);
        }
    }
    free(ptr);
    sendMemoryUpdate();
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete(void* ptr, const char*, int) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, const char*, int) noexcept {
    operator delete(ptr);
}

// ✅ Actualizar vista previa después de modificar datos
void updateDataPreview(void* ptr, const std::string& typeHint) {
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(allocMutex);
    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
        std::string type = typeHint.empty() ? it->second.type : typeHint;
        it->second.dataPreview = generateDataPreview(ptr, it->second.size, type);
        sendMemoryUpdate();
    }
}

void printMemoryReport() {
    std::lock_guard<std::mutex> lock(allocMutex);
    std::cout << "\n=== Reporte de Asignaciones por Archivo ===\n";
    for (auto& [file, count] : fileAllocCount) {
        std::cout << "Archivo: " << file
                  << " | Asignaciones: " << count
                  << " | Memoria total: " << fileAllocBytes[file] << " bytes\n";
    }
}

void printLeakReport(const std::string &pathFilter) {
    std::lock_guard<std::mutex> lock(allocMutex);
    if (allocations.empty()) {
        std::cout << "[Leak Report] ✅ No se detectaron fugas.\n";
        return;
    }

    std::map<std::string, std::vector<std::pair<void*, AllocationInfo>>> byFile;
    size_t totalLeaked = 0;
    for (auto const &p : allocations) {
        const void* ptr = p.first;
        const AllocationInfo &info = p.second;
        if (!pathFilter.empty() && info.file.find(pathFilter) == std::string::npos) continue;
        byFile[info.file].push_back({(void*)ptr, info});
        totalLeaked += info.size;
    }

    if (byFile.empty()) {
        std::cout << "[Leak Report] ✅ No quedan fugas tras filtro.\n";
        return;
    }

    std::cout << "\n[Leak Report] ❌ " << allocations.size()
              << " fugas detectadas (" << totalLeaked << " bytes)\n";
    for (auto &entry : byFile) {
        std::cout << "Archivo: " << entry.first << "\n";
        for (auto &pr : entry.second) {
            std::cout << "  - " << pr.second.size << " bytes en línea "
                      << pr.second.line << " tipo: " << pr.second.type
                      << " valor: " << pr.second.dataPreview << "\n";
        }
    }
}

void sendLeakReport() {
    QJsonArray leaksArray;
    {
        std::lock_guard<std::mutex> lock(allocMutex);
        for (const auto& [ptr, info] : allocations) {
            QJsonObject leakObj;
            leakObj["direccion"] = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0'));
            leakObj["tamano"] = static_cast<qint64>(info.size);
            leakObj["archivo"] = QString::fromStdString(info.file);
            leakObj["linea"] = info.line;
            leakObj["tipo"] = QString::fromStdString(info.type);
            leakObj["dataPreview"] = QString::fromStdString(info.dataPreview);
            leaksArray.append(leakObj);
        }
    }

    QJsonObject root;
    root["type"] = "leaks";
    root["data"] = leaksArray;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    jsonData.append('\n');

    std::lock_guard<std::mutex> lock(socketMutex);
    if (socketConnected && nativeSock != INVALID_SOCKET) {
#ifdef _WIN32
        send(nativeSock, reinterpret_cast<const char*>(jsonData.constData()), static_cast<int>(jsonData.size()), 0);
#else
        ::send(nativeSock, jsonData.constData(), jsonData.size(), 0);
#endif
    }
}

QJsonObject exportMemoryState() {
    std::lock_guard<std::mutex> lock(allocMutex);

    QJsonObject json;
    uint64_t memoriaActual = 0;

    QJsonArray topArchivos;
    QJsonArray bloques;

    for (const auto& [_, info] : allocations)
        memoriaActual += info.size;

    for (const auto& [file, bytes] : fileAllocBytes) {
        if (bytes > 0) {
            QJsonObject obj;
            obj["file"] = QString::fromStdString(file);
            obj["size"] = static_cast<qint64>(bytes);
            obj["count"] = static_cast<qint64>(fileAllocCount[file]);
            topArchivos.append(obj);
        }
    }

    for (const auto& [ptr, info] : allocations) {
        QJsonObject obj;
        obj["direccion"] = QString("0x%1").arg((quintptr)ptr, 16, 16, QLatin1Char('0'));
        obj["tam"] = static_cast<qint64>(info.size);
        obj["tipo"] = QString::fromStdString(info.type);
        obj["estado"] = QString("Activo");
        obj["linea"] = info.line;
        obj["archivo"] = QString::fromStdString(info.file);
        obj["dataPreview"] = QString::fromStdString(info.dataPreview);
        bloques.append(obj);
    }

    double memoriaMB = memoriaActual / (1024.0 * 1024.0);
    json["memoriaActualMB"] = memoriaMB;
    json["memoriaActualBytes"] = static_cast<qint64>(memoriaActual);
    json["memoriaActual"] = memoriaMB;
    json["leaks"] = static_cast<qint64>(allocations.size());
    json["topArchivos"] = topArchivos;
    json["bloques"] = bloques;
    json["type"] = "memory";

    return json;
}

void connectToGui(const std::string &host, int port) {
    std::lock_guard<std::mutex> lock(socketMutex);

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    if (nativeSock != INVALID_SOCKET)
        closeSocket();

    nativeSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nativeSock == INVALID_SOCKET) {
        socketConnected = false;
        std::cerr << "❌ Error: socket() falló\n";
        return;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(static_cast<uint16_t>(port));
    servAddr.sin_addr.s_addr = inet_addr(host.c_str());
    if (servAddr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent* he = gethostbyname(host.c_str());
        if (he && he->h_length > 0)
            servAddr.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr_list[0]);
    }

    int res = connect(nativeSock, reinterpret_cast<sockaddr*>(&servAddr), sizeof(servAddr));
    if (res == SOCKET_ERROR) {
        closeSocket();
        std::cerr << "❌ No se pudo conectar al GUI (" << host << ":" << port << ")\n";
        return;
    }

    socketConnected = true;
    std::cout << "✅ Conectado al GUI (" << host << ":" << port << ")\n";
}

void disconnectFromGui() {
    std::lock_guard<std::mutex> lock(socketMutex);
    closeSocket();
#ifdef _WIN32
    WSACleanup();
#endif
}

void sendMemoryUpdate() {
    if (!socketConnected || nativeSock == INVALID_SOCKET)
        return;

    QJsonObject json = exportMemoryState();
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');

    std::lock_guard<std::mutex> lock(socketMutex);
    if (socketConnected && nativeSock != INVALID_SOCKET) {
#ifdef _WIN32
        send(nativeSock, reinterpret_cast<const char*>(data.constData()), static_cast<int>(data.size()), 0);
#else
        ::send(nativeSock, data.constData(), data.size(), 0);
#endif
    }
}