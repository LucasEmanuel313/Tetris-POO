#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#include <string>

#include "net_client.h"
#include "server.h"

// This file implements a very small TCP relay server for a 2-player match.
//
// Design notes (OOP / architecture):
// - `ServerState` is the server's "model" (all runtime state in one place).
// - The server runs on a dedicated Win32 thread (`serverThreadProc`).
// - Sockets are put in non-blocking mode and we use `select()` so the thread
//   can react to new clients and incoming messages without ever blocking forever.
// - The protocol is line-based: each message is a single line ending with '\n'.

struct ServerState {
    int port = 5555;
    SOCKET listenSock = INVALID_SOCKET;
    SOCKET clients[2]{ INVALID_SOCKET, INVALID_SOCKET };
    std::string buffers[2];

    // Rematch votes: -1 = no vote yet, 0 = no, 1 = yes.
    int rematchVote[2]{ -1, -1 };
    bool matchOver = false;

    volatile LONG ready = 0;    // 1 once the server is listening
    volatile LONG running = 1;  // 1 while the server is active
};

void ServerStateDeleter::operator()(ServerState* p) const {
    delete p;
}

static int garbageFromCleared(int cleared) {
    // Rule (simple version): each cleared line sends 1 garbage line.
    if (cleared <= 0) return 0;
    if (cleared > 4) cleared = 4;
    return cleared;
}

// Parse one incoming protocol line from client `idx`.
// Return true if this client asked to disconnect ("LEAVE").
static bool processServerLine(int idx, ServerState* st, const std::string& line) {
    int other = 1 - idx;

    if (line.rfind("CLEARED ", 0) == 0) {
        int k = std::stoi(line.substr(8));
        int g = garbageFromCleared(k);
        if (g > 0 && st->clients[other] != INVALID_SOCKET) {
            sendLine(st->clients[other], "GARBAGE " + std::to_string(g));
        }
        return false;
    }

    if (line == "GAMEOVER") {
        st->matchOver = true;
        st->rematchVote[0] = -1;
        st->rematchVote[1] = -1;
        if (st->clients[other] != INVALID_SOCKET) sendLine(st->clients[other], "YOU_WIN");
        return false;
    }

    if (line == "LEAVE") {
        return true; // disconnect this client
    }

    if (line.rfind("REMATCH ", 0) == 0) {
        if (!st->matchOver) return false;

        if (line == "REMATCH YES") st->rematchVote[idx] = 1;
        else if (line == "REMATCH NO") st->rematchVote[idx] = 0;

        // If someone said NO, notify the other player and reset.
        if (st->rematchVote[idx] == 0) {
            if (st->clients[other] != INVALID_SOCKET) sendLine(st->clients[other], "REMATCH_ABORT");
            st->rematchVote[0] = -1;
            st->rematchVote[1] = -1;
            return false;
        }

        // Both accepted.
        if (st->rematchVote[0] == 1 && st->rematchVote[1] == 1) {
            if (st->clients[0] != INVALID_SOCKET) sendLine(st->clients[0], "REMATCH_START");
            if (st->clients[1] != INVALID_SOCKET) sendLine(st->clients[1], "REMATCH_START");
            st->rematchVote[0] = -1;
            st->rematchVote[1] = -1;
            st->matchOver = false;
        }
        return false;
    }

    // Forward board state to the other player.
    if (line.rfind("BOARD ", 0) == 0) {
        if (st->clients[other] != INVALID_SOCKET) {
            sendLine(st->clients[other], line);
        }
        return false;
    }

    return false;
}

static DWORD WINAPI serverThreadProc(LPVOID param) {
    ServerState* st = reinterpret_cast<ServerState*>(param);

    st->listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (st->listenSock == INVALID_SOCKET) {
        InterlockedExchange(&st->ready, 0);
        InterlockedExchange(&st->running, 0);
        return 0;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((u_short)st->port);

    if (bind(st->listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(st->listenSock);
        st->listenSock = INVALID_SOCKET;
        InterlockedExchange(&st->ready, 0);
        InterlockedExchange(&st->running, 0);
        return 0;
    }

    if (listen(st->listenSock, 2) == SOCKET_ERROR) {
        closesocket(st->listenSock);
        st->listenSock = INVALID_SOCKET;
        InterlockedExchange(&st->ready, 0);
        InterlockedExchange(&st->running, 0);
        return 0;
    }

    // Non-blocking listen socket: accept() will not stall the whole server thread.
    {
        u_long mode = 1;
        ioctlsocket(st->listenSock, FIONBIO, &mode);
    }

    InterlockedExchange(&st->ready, 1);

    auto setNonBlocking = [](SOCKET s) {
        u_long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);
    };

    auto acceptClients = [&]() {
        // Accept as many pending clients as possible (until accept() would block).
        while (true) {
            SOCKET c = accept(st->listenSock, nullptr, nullptr);
            if (c == INVALID_SOCKET) break;

            setNonBlocking(c);

            int slot = -1;
            if (st->clients[0] == INVALID_SOCKET) slot = 0;
            else if (st->clients[1] == INVALID_SOCKET) slot = 1;

            if (slot == -1) {
                closesocket(c);
                continue;
            }

            st->clients[slot] = c;
            st->buffers[slot].clear();

            if (slot == 0) {
                sendLine(st->clients[0], "ROLE HOST");
                sendLine(st->clients[0], "WAITING");
            } else {
                sendLine(st->clients[1], "ROLE GUEST");
            }

            // When both slots are filled, the match can start.
            if (st->clients[0] != INVALID_SOCKET && st->clients[1] != INVALID_SOCKET) {
                st->rematchVote[0] = -1;
                st->rematchVote[1] = -1;
                st->matchOver = false;
                sendLine(st->clients[0], "START");
                sendLine(st->clients[1], "START");
            }
        }
    };

    // Wait for the host to connect first.
    while (InterlockedCompareExchange(&st->running, 1, 1) == 1 && st->clients[0] == INVALID_SOCKET) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(st->listenSock, &readfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        int ready = select((int)st->listenSock + 1, &readfds, nullptr, nullptr, &tv);
        if (ready == SOCKET_ERROR) break;
        if (ready > 0 && FD_ISSET(st->listenSock, &readfds)) acceptClients();
    }

    while (InterlockedCompareExchange(&st->running, 1, 1) == 1) {
        fd_set readfds;
        FD_ZERO(&readfds);

        SOCKET maxfd = st->listenSock;

        // Always listen for new clients while a slot is available.
        FD_SET(st->listenSock, &readfds);
        for (int i = 0; i < 2; ++i) {
            if (st->clients[i] != INVALID_SOCKET) {
                FD_SET(st->clients[i], &readfds);
                if (st->clients[i] > maxfd) maxfd = st->clients[i];
            }
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select((int)maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (ready == SOCKET_ERROR) break;

        if (ready > 0 && FD_ISSET(st->listenSock, &readfds)) {
            acceptClients();
        }

        for (int i = 0; i < 2; ++i) {
            if (st->clients[i] == INVALID_SOCKET) continue;
            if (!FD_ISSET(st->clients[i], &readfds)) continue;

            char temp[512];
            int r = recv(st->clients[i], temp, sizeof(temp), 0);

            if (r <= 0) {
                closesocket(st->clients[i]);
                st->clients[i] = INVALID_SOCKET;

                st->buffers[i].clear();
                st->rematchVote[0] = -1;
                st->rematchVote[1] = -1;
                st->matchOver = false;

                // If the host leaves, close the room and disconnect the guest.
                if (i == 0) {
                    if (st->clients[1] != INVALID_SOCKET) {
                        sendLine(st->clients[1], "OPPONENT_LEFT");
                        closesocket(st->clients[1]);
                        st->clients[1] = INVALID_SOCKET;
                    }
                    InterlockedExchange(&st->running, 0);
                    break;
                }

                // If the guest leaves, keep the host alive and waiting.
                if (i == 1) {
                    if (st->clients[0] != INVALID_SOCKET) {
                        sendLine(st->clients[0], "OPPONENT_LEFT");
                        sendLine(st->clients[0], "WAITING");
                    }
                }
                continue;
            }

            st->buffers[i].append(temp, temp + r);

            size_t pos = 0;
            while (true) {
                size_t nl = st->buffers[i].find('\n', pos);
                if (nl == std::string::npos) break;

                std::string line = st->buffers[i].substr(pos, nl - pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();

                bool disconnect = processServerLine(i, st, line);
                if (disconnect) {
                    closesocket(st->clients[i]);
                    st->clients[i] = INVALID_SOCKET;
                    st->buffers[i].clear();
                    st->rematchVote[0] = -1;
                    st->rematchVote[1] = -1;
                    st->matchOver = false;

                    if (i == 0) {
                        if (st->clients[1] != INVALID_SOCKET) {
                            sendLine(st->clients[1], "OPPONENT_LEFT");
                            closesocket(st->clients[1]);
                            st->clients[1] = INVALID_SOCKET;
                        }
                        InterlockedExchange(&st->running, 0);
                    } else {
                        if (st->clients[0] != INVALID_SOCKET) {
                            sendLine(st->clients[0], "OPPONENT_LEFT");
                            sendLine(st->clients[0], "WAITING");
                        }
                    }
                    break;
                }
                pos = nl + 1;
            }
            if (pos > 0) st->buffers[i].erase(0, pos);
        }

        if (st->clients[0] == INVALID_SOCKET && st->clients[1] == INVALID_SOCKET) break;
    }

    if (st->clients[0] != INVALID_SOCKET) closesocket(st->clients[0]);
    if (st->clients[1] != INVALID_SOCKET) closesocket(st->clients[1]);
    if (st->listenSock != INVALID_SOCKET) closesocket(st->listenSock);

    InterlockedExchange(&st->running, 0);
    return 0;
}

bool startServer(int port, ServerHandle& out, std::string& error) {
    error.clear();

    // `ServerHandle` owns the server state with RAII (`unique_ptr`).
    // We keep it in a local smart pointer until we're sure the thread started.
    std::unique_ptr<ServerState, ServerStateDeleter> st(new ServerState());
    st->port = port;

    DWORD tid = 0;
    HANDLE hThread = CreateThread(nullptr, 0, serverThreadProc, st.get(), 0, &tid);
    if (!hThread) {
        error = "Could not start server thread.";
        return false;
    }

    DWORD t0 = GetTickCount();
    while (InterlockedCompareExchange(&st->ready, 0, 0) == 0) {
        if (GetTickCount() - t0 > 3000) break;
        Sleep(50);
    }

    if (InterlockedCompareExchange(&st->ready, 0, 0) == 0) {
        InterlockedExchange(&st->running, 0);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        error = "Server failed to start (port in use?).";
        return false;
    }

    out.state = std::move(st);
    out.thread = hThread;
    return true;
}

void stopServer(ServerHandle& handle) {
    if (!handle.state) return;

    // Cooperative shutdown: tell the thread to stop, then join it.
    InterlockedExchange(&handle.state->running, 0);

    if (handle.thread) {
        WaitForSingleObject(handle.thread, INFINITE);
        CloseHandle(handle.thread);
        handle.thread = nullptr;
    }

    handle.state.reset();
}
