#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

struct ServerState;

struct ServerStateDeleter {
    void operator()(ServerState* p) const;
};

struct ServerHandle {
    std::unique_ptr<ServerState, ServerStateDeleter> state;
    HANDLE thread = nullptr;
};

// Starts the server on a background thread. Returns true if it becomes "ready".
bool startServer(int port, ServerHandle& out, std::string& error);

// Stops the server (if running) and releases resources.
void stopServer(ServerHandle& handle);

#endif
