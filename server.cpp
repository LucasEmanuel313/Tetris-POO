#include <iostream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib") // MSVC ignora no g++, mas ok

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

static int garbageFromCleared(int cleared) {
    // 1->0, 2->1, 3->2, 4->4
    if (cleared <= 1) return 0;
    if (cleared == 2) return 1;
    if (cleared == 3) return 2;
    return 4;
}

static void processLine(int idx, SOCKET clients[2], const std::string& line) {
    int other = 1 - idx;

    if (line.rfind("CLEARED ", 0) == 0) {
        int k = std::stoi(line.substr(8));
        int g = garbageFromCleared(k);
        if (g > 0 && clients[other] != INVALID_SOCKET) {
            sendLine(clients[other], "GARBAGE " + std::to_string(g));
            std::cout << "Client " << (idx+1) << " cleared " << k
                      << " -> sent GARBAGE " << g << " to client " << (other+1) << "\n";
        }
        return;
    }

    if (line == "GAMEOVER") {
        if (clients[other] != INVALID_SOCKET) sendLine(clients[other], "YOU_WIN");
        std::cout << "Client " << (idx+1) << " GAMEOVER\n";
        return;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    addr.sin_port = htons(5555);

    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, 2) == SOCKET_ERROR) {
        std::cerr << "listen failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port 5555...\n";
    std::cout << "Waiting 2 clients...\n";

    SOCKET clients[2]{ INVALID_SOCKET, INVALID_SOCKET };
    clients[0] = accept(listenSock, nullptr, nullptr);
    if (clients[0] == INVALID_SOCKET) {
        std::cerr << "accept client1 failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    std::cout << "Client 1 connected\n";

    clients[1] = accept(listenSock, nullptr, nullptr);
    if (clients[1] == INVALID_SOCKET) {
        std::cerr << "accept client2 failed\n";
        closesocket(clients[0]);
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    std::cout << "Client 2 connected\n";

    sendLine(clients[0], "START");
    sendLine(clients[1], "START");

    // buffers por cliente
    std::string buffers[2];

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);

        SOCKET maxfd = 0;
        for (int i = 0; i < 2; ++i) {
            if (clients[i] != INVALID_SOCKET) {
                FD_SET(clients[i], &readfds);
                if (clients[i] > maxfd) maxfd = clients[i];
            }
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select((int)maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (ready == SOCKET_ERROR) {
            std::cerr << "select error\n";
            break;
        }

        for (int i = 0; i < 2; ++i) {
            if (clients[i] == INVALID_SOCKET) continue;
            if (!FD_ISSET(clients[i], &readfds)) continue;

            char temp[512];
            int r = recv(clients[i], temp, sizeof(temp), 0);
            if (r <= 0) {
                std::cout << "Client " << (i+1) << " disconnected\n";
                closesocket(clients[i]);
                clients[i] = INVALID_SOCKET;

                int other = 1 - i;
                if (clients[other] != INVALID_SOCKET) sendLine(clients[other], "OPPONENT_LEFT");
                continue;
            }

            buffers[i].append(temp, temp + r);

            size_t pos = 0;
            while (true) {
                size_t nl = buffers[i].find('\n', pos);
                if (nl == std::string::npos) break;

                std::string line = buffers[i].substr(pos, nl - pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();

                processLine(i, clients, line);
                pos = nl + 1;
            }
            if (pos > 0) buffers[i].erase(0, pos);
        }

        if (clients[0] == INVALID_SOCKET && clients[1] == INVALID_SOCKET) {
            std::cout << "Both clients disconnected. Closing server.\n";
            break;
        }
    }

    if (clients[0] != INVALID_SOCKET) closesocket(clients[0]);
    if (clients[1] != INVALID_SOCKET) closesocket(clients[1]);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}
