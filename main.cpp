
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include <iostream>
#include <string>

#include "console_ui.h"
#include "game_modes.h"
#include "net_client.h"
#include "server.h"

// -------------------- Menu helpers --------------------
static int readInt(const std::string& prompt, int def) {
    std::cout << prompt << " (default " << def << "): ";
    std::cout.flush();
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

static std::string readStr(const std::string& prompt, const std::string& def) {
    std::cout << prompt << " (default " << def << "): ";
    std::cout.flush();
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) return def;
    return s;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    while (true) {
        menuClear();
        flushConsoleInputEvents();
        std::cout << "==== TETRIS ====\n";
        std::cout << "1) Singleplayer\n";
        std::cout << "2) Multiplayer\n";
        std::cout << "0) Exit\n";
        std::cout << "> ";
        std::cout.flush();

        std::string opt;
        std::getline(std::cin, opt);
        if (opt == "0") break;

        if (opt == "1") {
            runSinglePlayer();
            continue;
        }

        if (opt == "2") {
            menuClear();
            std::cout << "== Multiplayer ==\n";
            std::cout << "1) Create room (Host)\n";
            std::cout << "2) Join room\n";
            std::cout << "0) Back\n";
            std::cout << "> ";
            std::cout.flush();

            std::string mo;
            std::getline(std::cin, mo);
            if (mo == "0") continue;

            if (mo == "1") {
                int port = readInt("Port", 5555);

                ServerHandle server;
                std::string error;
                if (!startServer(port, server, error)) {
                    std::cout << error << "\nPress ENTER...\n";
                    std::cout.flush();
                    std::string dummy;
                    std::getline(std::cin, dummy);
                    continue;
                }

                menuClear();
                std::cout << "Room created!\n";
                std::cout << "Your IP (share with friend): use ipconfig\n";
                std::cout << "Port: " << port << "\n";
                std::cout << "Press ENTER to start local client (host)...\n";
                std::cout.flush();
                std::string dummy; std::getline(std::cin, dummy);

                SOCKET sock = INVALID_SOCKET;
                if (!connectToServer("127.0.0.1", port, sock)) {
                    std::cout << "Host could not connect to local server.\n";
                    stopServer(server);
                    std::cout << "Press ENTER...\n";
                    std::cout.flush();
                    std::string dummy;
                    std::getline(std::cin, dummy);
                    continue;
                }

                runMultiplayerClient(sock);
                closesocket(sock);

                stopServer(server);
                continue;
            }

            if (mo == "2") {
                std::string ip = readStr("Server IP", "127.0.0.1");
                int port = readInt("Port", 5555);

                SOCKET sock = INVALID_SOCKET;
                if (!connectToServer(ip, port, sock)) {
                    std::cout << "Could not connect to server.\nPress ENTER...\n";
                    std::cout.flush();
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
