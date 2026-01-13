#include "NetworkClient.hpp"
#include <iostream>

static bool g_wsaInitDone = false;

NetworkClient::NetworkClient() {}

NetworkClient::~NetworkClient() {
    m_running.store(false);

    if (m_rxThread.joinable()) {
        m_rxThread.join();
    }
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
    // Não chamamos WSACleanup globalmente aqui para evitar conflito entre objetos.
}

bool NetworkClient::connectTo(const std::string& host, const std::string& port) {
    if (!g_wsaInitDone) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
        g_wsaInitDone = true;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        return false;
    }

    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET) {
        freeaddrinfo(result);
        std::cerr << "socket failed\n";
        return false;
    }

    if (::connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        closesocket(s);
        freeaddrinfo(result);
        std::cerr << "connect failed\n";
        return false;
    }

    freeaddrinfo(result);

    m_sock = s;
    m_connected.store(true);
    m_running.store(true);

    m_rxThread = std::thread(&NetworkClient::rxLoop, this);
    return true;
}

bool NetworkClient::sendLine(const std::string& line) {
    if (!m_connected.load()) return false;

    std::string msg = line;
    if (msg.empty() || msg.back() != '\n') msg.push_back('\n');

    int totalSent = 0;
    int len = (int)msg.size();
    while (totalSent < len) {
        int sent = send(m_sock, msg.c_str() + totalSent, len - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            m_connected.store(false);
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool NetworkClient::pollLine(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (m_incoming.empty()) return false;
    out = std::move(m_incoming.front());
    m_incoming.pop();
    return true;
}

void NetworkClient::pushIncoming(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_incoming.push(line);
}

void NetworkClient::rxLoop() {
    std::string buffer;
    buffer.reserve(4096);

    char temp[512];

    while (m_running.load() && m_connected.load()) {
        int r = recv(m_sock, temp, sizeof(temp), 0);
        if (r <= 0) {
            m_connected.store(false);
            break;
        }
        buffer.append(temp, temp + r);

        // separa por '\n'
        size_t pos = 0;
        while (true) {
            size_t nl = buffer.find('\n', pos);
            if (nl == std::string::npos) break;

            std::string line = buffer.substr(pos, nl - pos);
            // remove '\r' se vier do Windows
            if (!line.empty() && line.back() == '\r') line.pop_back();

            pushIncoming(line);
            pos = nl + 1;
        }
        // mantém o resto
        if (pos > 0) buffer.erase(0, pos);
    }
}
