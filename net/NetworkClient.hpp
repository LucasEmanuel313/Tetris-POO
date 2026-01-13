#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Conecta no servidor (ex: host="127.0.0.1", port="5555")
    bool connectTo(const std::string& host, const std::string& port);

    // Envia uma linha (termina com '\n' se não tiver)
    bool sendLine(const std::string& line);

    // Tenta pegar uma mensagem recebida (não bloqueia). Retorna true se pegou.
    bool pollLine(std::string& out);

    bool isConnected() const { return m_connected.load(); }

private:
    SOCKET m_sock = INVALID_SOCKET;
    std::thread m_rxThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    std::mutex m_mtx;
    std::queue<std::string> m_incoming;

    void rxLoop();
    void pushIncoming(const std::string& line);
};
