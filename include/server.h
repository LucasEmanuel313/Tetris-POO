#ifndef SERVER_H
#define SERVER_H

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

struct ServerState;

struct ServerHandle {
    ServerState* state = nullptr;
    HANDLE thread = nullptr;
};

// Inicia o servidor em uma thread. Retorna true se ficou "ready".
bool startServer(int port, ServerHandle& out, std::string& error);

// Para o servidor (se estiver rodando) e libera recursos.
void stopServer(ServerHandle& handle);

#endif
