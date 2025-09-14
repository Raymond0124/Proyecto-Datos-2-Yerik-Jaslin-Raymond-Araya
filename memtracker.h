#ifndef MEMTRACKER_H
#define MEMTRACKER_H

#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>

// No redefinimos AllocationInfo ni variables globales que ya existen en library.h

// Variables de socket
extern std::atomic<bool> socketConnected;
extern QTcpSocket* clientSocket;

// Funciones de conexión y envío de datos
void connectToGui(const std::string &host = "127.0.0.1", int port = 5000);
void sendMemoryUpdate();

#endif // MEMTRACKER_H
