#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include <iostream>
#include <string>

#include "net_client.h"

bool sendLine(SOCKET s, const std::string& line) {
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

bool connectToServer(const std::string& host, int port, SOCKET& outSock) {
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
