#include <iostream>
#include <string>
#include <cstdlib>
#include <conio.h>
#include <windows.h>   // Sleep, GetTickCount, CreateThread

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include "table.h"

// -------------------- WinSock helpers --------------------
static bool sendLine(SOCKET s, const std::string& line) {
    std::string msg = line;
    if (msg.empty() || msg.back() != '\n') msg.push_back('\n');

    int total = 0;
    int len = (int)msg.size();
    while (total < len) {
        int sent = send(s, msg.c_str() + total, len - total, 0);
        if (sent == SOCKET_ERROR) return false;
        total += sent;
    }
    return true;
}

// Connect SEM getaddrinfo (compatível)
static bool connectToServer(const std::string& host, int port, SOCKET& outSock) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);

    unsigned long ip = inet_addr(host.c_str());
    if (ip != INADDR_NONE) {
        addr.sin_addr.s_addr = ip;
    } else {
        hostent* he = gethostbyname(host.c_str());
        if (!he || !he->h_addr_list || !he->h_addr_list[0]) {
            std::cerr << "Could not resolve host: " << host << "\n";
            closesocket(s);
            return false;
        }
        addr.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr_list[0]);
    }

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "connect failed\n";
        closesocket(s);
        return false;
    }

    // non-blocking
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);

    outSock = s;
    return true;
}

// -------------------- Embedded server (thread) --------------------
struct ServerState {
    int port = 5555;
    SOCKET listenSock = INVALID_SOCKET;
    SOCKET clients[2]{ INVALID_SOCKET, INVALID_SOCKET };
    std::string buffers[2];

    volatile LONG ready = 0;    // 1 quando estiver escutando
    volatile LONG running = 1;  // 1 enquanto servidor ativo
};

static int garbageFromCleared(int cleared) {
    // regra simples: 1->0, 2->1, 3->2, 4->4
    if (cleared <= 1) return 0;
    if (cleared == 2) return 1;
    if (cleared == 3) return 2;
    return 4;
}

static void processServerLine(int idx, ServerState* st, const std::string& line) {
    int other = 1 - idx;

    if (line.rfind("CLEARED ", 0) == 0) {
        int k = std::stoi(line.substr(8));
        int g = garbageFromCleared(k);
        if (g > 0 && st->clients[other] != INVALID_SOCKET) {
            sendLine(st->clients[other], "GARBAGE " + std::to_string(g));
        }
        return;
    }

    if (line == "GAMEOVER") {
        if (st->clients[other] != INVALID_SOCKET) sendLine(st->clients[other], "YOU_WIN");
        return;
    }
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
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
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

    InterlockedExchange(&st->ready, 1);

    // aceita 2 clientes (host e o outro PC)
    st->clients[0] = accept(st->listenSock, nullptr, nullptr);
    if (st->clients[0] == INVALID_SOCKET) {
        InterlockedExchange(&st->running, 0);
        return 0;
    }
    st->clients[1] = accept(st->listenSock, nullptr, nullptr);
    if (st->clients[1] == INVALID_SOCKET) {
        closesocket(st->clients[0]);
        st->clients[0] = INVALID_SOCKET;
        InterlockedExchange(&st->running, 0);
        return 0;
    }

    sendLine(st->clients[0], "START");
    sendLine(st->clients[1], "START");

    // loop de repasse (select)
    while (InterlockedCompareExchange(&st->running, 1, 1) == 1) {
        fd_set readfds;
        FD_ZERO(&readfds);

        SOCKET maxfd = 0;
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

        for (int i = 0; i < 2; ++i) {
            if (st->clients[i] == INVALID_SOCKET) continue;
            if (!FD_ISSET(st->clients[i], &readfds)) continue;

            char temp[512];
            int r = recv(st->clients[i], temp, sizeof(temp), 0);
            if (r <= 0) {
                int other = 1 - i;
                closesocket(st->clients[i]);
                st->clients[i] = INVALID_SOCKET;
                if (st->clients[other] != INVALID_SOCKET) {
                    sendLine(st->clients[other], "OPPONENT_LEFT");
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

                processServerLine(i, st, line);

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

// -------------------- Game loop --------------------
static void processIncoming(table& ta, const std::string& line, bool& running, bool& started) {
    if (line == "START") {
        started = true;
        system("CLS");
        ta.print_table();
        return;
    }
    if (line.rfind("GARBAGE ", 0) == 0) {
        int n = std::stoi(line.substr(8));
        ta.apply_garbage(n);
        system("CLS");
        ta.print_table();
        return;
    }
    if (line == "YOU_WIN") {
        system("CLS");
        ta.print_table();
        std::cout << "\n>>> YOU WIN! <<<\n";
        running = false;
        return;
    }
    if (line == "OPPONENT_LEFT") {
        system("CLS");
        ta.print_table();
        std::cout << "\n>>> Opponent left. <<<\n";
        running = false;
        return;
    }
}

static void runSinglePlayer() {
    table ta;
    ta.add_block();
    system("CLS");
    ta.print_table();

    DWORD lastFall = GetTickCount();
    const DWORD fallIntervalMs = 500;

    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q') break;

            if (key == 'a') ta.block_left();
            if (key == 'd') ta.block_right();
            if (key == 's') ta.block_descend();
            if (key == 'w') ta.rotate_block();
            if (key == ' ') ta.block_drop();

            system("CLS");
            ta.print_table();
        }

        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;
            system("CLS");
            ta.print_table();
        }

        Sleep(10);
    }
}

static void runMultiplayerClient(SOCKET sock) {
    table ta;
    ta.add_block();
    system("CLS");
    ta.print_table();

    bool running = true;
    bool started = false;

    DWORD lastFall = GetTickCount();
    const DWORD fallIntervalMs = 500;

    std::string rxBuffer;
    rxBuffer.reserve(4096);

    while (running) {
        // 1) Rede: recv non-blocking
        char temp[512];
        while (true) {
            int r = recv(sock, temp, sizeof(temp), 0);
            if (r > 0) {
                rxBuffer.append(temp, temp + r);
            } else {
                int err = WSAGetLastError();
                if (r == SOCKET_ERROR && err == WSAEWOULDBLOCK) break;
                if (r == 0) { running = false; break; }
                if (r == SOCKET_ERROR) { running = false; break; }
                break;
            }
        }

        // parse linhas
        size_t pos = 0;
        while (true) {
            size_t nl = rxBuffer.find('\n', pos);
            if (nl == std::string::npos) break;

            std::string line = rxBuffer.substr(pos, nl - pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();

            processIncoming(ta, line, running, started);

            pos = nl + 1;
        }
        if (pos > 0) rxBuffer.erase(0, pos);

        // se ainda não começou, só espera
        if (!started) {
            if (_kbhit()) {
                char key = _getch();
                if (key == 'q') { running = false; break; }
            }
            Sleep(30);
            continue;
        }

        // 2) Input
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q') { running = false; break; }

            if (key == 'a') ta.block_left();
            if (key == 'd') ta.block_right();
            if (key == 's') ta.block_descend();
            if (key == 'w') ta.rotate_block();
            if (key == ' ') ta.block_drop();

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

            system("CLS");
            ta.print_table();
        }

        // 3) Clock automático
        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

            system("CLS");
            ta.print_table();
        }

        Sleep(10);
    }
}

// -------------------- Menu helpers --------------------
static int readInt(const std::string& prompt, int def) {
    std::cout << prompt << " (default " << def << "): ";
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

static std::string readStr(const std::string& prompt, const std::string& def) {
    std::cout << prompt << " (default " << def << "): ";
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) return def;
    return s;
}

int main() {
    // WinSock init (uma vez)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    while (true) {
        system("CLS");
        std::cout << "==== TETRIS ====\n";
        std::cout << "1) Singleplayer\n";
        std::cout << "2) Multiplayer\n";
        std::cout << "0) Exit\n";
        std::cout << "> ";

        std::string opt;
        std::getline(std::cin, opt);
        if (opt == "0") break;

        if (opt == "1") {
            runSinglePlayer();
            continue;
        }

        if (opt == "2") {
            system("CLS");
            std::cout << "== Multiplayer ==\n";
            std::cout << "1) Create room (Host)\n";
            std::cout << "2) Join room\n";
            std::cout << "0) Back\n";
            std::cout << "> ";

            std::string mo;
            std::getline(std::cin, mo);
            if (mo == "0") continue;

            if (mo == "1") {
                int port = readInt("Port", 5555);

                // start embedded server
                ServerState st;
                st.port = port;

                DWORD tid = 0;
                HANDLE hThread = CreateThread(nullptr, 0, serverThreadProc, &st, 0, &tid);
                if (!hThread) {
                    std::cout << "Could not start server thread.\n";
                    std::cout << "Press ENTER...\n";
                    std::string dummy; std::getline(std::cin, dummy);
                    continue;
                }

                // espera o server ficar pronto
                DWORD t0 = GetTickCount();
                while (InterlockedCompareExchange(&st.ready, 0, 0) == 0) {
                    if (GetTickCount() - t0 > 3000) break;
                    Sleep(50);
                }
                if (InterlockedCompareExchange(&st.ready, 0, 0) == 0) {
                    std::cout << "Server failed to start (port in use?).\n";
                    InterlockedExchange(&st.running, 0);
                    WaitForSingleObject(hThread, 1000);
                    CloseHandle(hThread);
                    std::cout << "Press ENTER...\n";
                    std::string dummy; std::getline(std::cin, dummy);
                    continue;
                }

                std::cout << "Room created!\n";
                std::cout << "Your IP (share with friend): use ipconfig\n";
                std::cout << "Port: " << port << "\n";
                std::cout << "Waiting opponent... (press ENTER to start local client)\n";
                std::string dummy; std::getline(std::cin, dummy);

                // host connects locally
                SOCKET sock = INVALID_SOCKET;
                if (!connectToServer("127.0.0.1", port, sock)) {
                    std::cout << "Host could not connect to local server.\n";
                    InterlockedExchange(&st.running, 0);
                    WaitForSingleObject(hThread, 1000);
                    CloseHandle(hThread);
                    std::cout << "Press ENTER...\n";
                    std::getline(std::cin, dummy);
                    continue;
                }

                runMultiplayerClient(sock);

                // cleanup
                closesocket(sock);
                InterlockedExchange(&st.running, 0);
                WaitForSingleObject(hThread, 1000);
                CloseHandle(hThread);

                continue;
            }

            if (mo == "2") {
                std::string ip = readStr("Server IP", "127.0.0.1");
                int port = readInt("Port", 5555);

                SOCKET sock = INVALID_SOCKET;
                if (!connectToServer(ip, port, sock)) {
                    std::cout << "Could not connect to server.\n";
                    std::cout << "Press ENTER...\n";
                    std::string dummy; std::getline(std::cin, dummy);
                    continue;
                }

                runMultiplayerClient(sock);
                closesocket(sock);
                continue;
            }
        }
    }

    WSACleanup();
    return 0;
}
