//
// Client (game) - Option B: send CLEARED events, receive GARBAGE
//
#include <iostream>
#include <string>
#include <cstdlib>
#include <conio.h>
#include <windows.h>      // Sleep, GetTickCount
#include <stdlib.h>       // system("CLS")

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif

#include "table.h"

// -------------------- helpers --------------------
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

// Troque a connectToServer antiga por esta (sem getaddrinfo/freeaddrinfo)
static bool connectToServer(const std::string& host, const std::string& port, SOCKET& outSock) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;

    int p = std::stoi(port);
    if (p <= 0 || p > 65535) {
        std::cerr << "Invalid port\n";
        closesocket(s);
        return false;
    }
    addr.sin_port = htons((u_short)p);

    // host pode ser IP ("127.0.0.1") ou nome ("localhost")
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


static void processIncoming(table& ta, const std::string& line, bool& running) {
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
        std::cout << "\n>>> Opponent left the game. <<<\n";
        running = false;
        return;
    }

    // opcional: "START" se seu server mandar
    if (line == "START") {
        // só ignora; pode usar pra mostrar "go!"
        return;
    }
}

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::string port = "5555";
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = argv[2];

    SOCKET sock = INVALID_SOCKET;
    if (!connectToServer(host, port, sock)) {
        std::cerr << "Could not connect to server.\n";
        return 1;
    }

    std::cout << "Connected to server " << host << ":" << port << "\n";

    // game
    table ta;
    ta.add_block();

    system("CLS");
    ta.print_table();

    // timing
    DWORD lastFall = GetTickCount();
    const DWORD fallIntervalMs = 500;

    // networking receive buffer
    std::string rxBuffer;
    rxBuffer.reserve(4096);

    bool running = true;

    while (running) {
        // -------- 1) Input (non-blocking) --------
        if (_kbhit()) {
            char key = _getch();

            switch (key) {
                case 'a': ta.block_left();   break;
                case 'd': ta.block_right();  break;
                case 's': ta.block_descend();break; // soft drop
                case 'w': ta.rotate_block(); break;
                case ' ': ta.block_drop();   break; // hard drop
                case 'q': running = false;   break;
                default: break;
            }

            // se limpou linhas nessa ação -> manda pro server
            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) {
                sendLine(sock, "CLEARED " + std::to_string(cleared));
            }

            system("CLS");
            ta.print_table();
        }

        // -------- 2) Gravidade (clock automático) --------
        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) {
                sendLine(sock, "CLEARED " + std::to_string(cleared));
            }

            system("CLS");
            ta.print_table();
        }

        // -------- 3) Rede (recv non-blocking + parse por linhas) --------
        char temp[512];
        while (true) {
            int r = recv(sock, temp, sizeof(temp), 0);
            if (r > 0) {
                rxBuffer.append(temp, temp + r);
            } else {
                int err = WSAGetLastError();
                if (r == SOCKET_ERROR && err == WSAEWOULDBLOCK) {
                    // nada a receber agora
                    break;
                }
                if (r == 0) {
                    std::cout << "\nServer closed connection.\n";
                    running = false;
                    break;
                }
                if (r == SOCKET_ERROR) {
                    std::cout << "\nrecv error.\n";
                    running = false;
                    break;
                }
                break;
            }
        }

        // parse lines
        size_t pos = 0;
        while (true) {
            size_t nl = rxBuffer.find('\n', pos);
            if (nl == std::string::npos) break;

            std::string line = rxBuffer.substr(pos, nl - pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();

            processIncoming(ta, line, running);

            pos = nl + 1;
        }
        if (pos > 0) rxBuffer.erase(0, pos);

        // -------- 4) pausa --------
        Sleep(10);
    }

    // finaliza rede
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();

    return 0;
}
