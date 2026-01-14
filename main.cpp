#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include <windows.h>   // Sleep, GetTickCount, CreateThread, console API
#include <conio.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include "table.h"

// -------------------- Console rendering helpers --------------------
static HANDLE g_hOut = INVALID_HANDLE_VALUE;
static bool g_consoleInited = false;

static void clearConsoleWinAPI() {
    if (g_hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(g_hOut, &csbi)) return;

    DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    DWORD count;
    COORD home = {0, 0};

    FillConsoleOutputCharacter(g_hOut, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(g_hOut, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(g_hOut, home);
}

static void flushConsoleInputEvents() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        FlushConsoleInputBuffer(hIn);
    }
}

static void initGameConsoleOnce() {
    if (g_consoleInited) return;
    g_consoleInited = true;

    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // esconder cursor (reduz flicker)
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 25;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(g_hOut, &ci);

    // acelera i/o do cout
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    clearConsoleWinAPI(); // limpa uma vez
}

static void renderFrame(table& ta) {
    initGameConsoleOnce();
    COORD home{0, 0};
    SetConsoleCursorPosition(g_hOut, home);
    ta.print_table();
    std::cout.flush();
}

static void menuClear() {
    // Para menus: limpar a tela inteira (sem flicker absurdo, porque não é por frame)
    if (g_hOut == INVALID_HANDLE_VALUE) g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    clearConsoleWinAPI();
}

static void drainKb() {
    while (_kbhit()) {
        (void)_getch();
    }
}

static bool confirmLeaveToMenu() {
    drainKb();
    menuClear();
    std::cout << "Sair para o menu? (Y/N) ";
    std::cout.flush();
    while (true) {
        char c = _getch();
        if (c == 'y' || c == 'Y') return true;
        if (c == 'n' || c == 'N' || c == 27) return false; // ESC = cancelar
    }
}

// 1 = revanche / nova partida, 0 = menu
// Não limpa a tela: use após renderizar o(s) tabuleiro(s)
static int postGameChoiceInline() {
    drainKb();
    std::cout << "1) Jogar outra partida\n";
    std::cout << "2) Voltar ao menu\n";
    std::cout << "> ";
    std::cout.flush();
    while (true) {
        char c = _getch();
        if (c == '1') return 1;
        if (c == '2' || c == 27) return 0;
    }
}

static int postGameChoiceWithTitle(const char* title) {
    std::cout << "\n" << title << "\n\n";
    return postGameChoiceInline();
}

// -------------------- Multiplayer rendering (two boards) --------------------
struct OpponentState {
    bool hasBoard = false;
    int score = 0;
    std::string data; // 220 chars: '.' or '#', y=0..21 then x=0..9

    OpponentState() : data(10 * 22, '.') {}
};

static std::string miniLineForType(int type, int miniRowFromTop) {
    if (type < 0 || type > 6) return "        ";
    const char (*p)[4][4] = TETROMINOES[type];
    std::string out;
    out.reserve(8);
    int y = 3 - miniRowFromTop;
    for (int x = 0; x < 4; ++x) {
        out += ((*p)[x][y] == '#') ? "[]" : "  ";
    }
    return out;
}

static std::string sidePanelLineMultiplayer(const table& local, int rowIndexFromTop, int pendingIncomingGarbage) {
    // Layout (22 linhas): HOLD(1+4) + blank(1) + NEXT(1 + 3*4 + 2 blanks) + Garbage(1)
    const int hold = local.get_hold_type();
    const std::vector<int> next = local.get_next_types(3);

    if (rowIndexFromTop == 0) return "HOLD";
    if (rowIndexFromTop >= 1 && rowIndexFromTop <= 4) {
        if (hold < 0) return (rowIndexFromTop == 1) ? "(none)" : "";
        return miniLineForType(hold, rowIndexFromTop - 1);
    }
    if (rowIndexFromTop == 5) return "";
    if (rowIndexFromTop == 6) return "NEXT";

    int base = 7;
    for (int n = 0; n < 3; ++n) {
        int start = base + n * 5;
        int end = start + 3;
        if (rowIndexFromTop >= start && rowIndexFromTop <= end) {
            int t = (n < (int)next.size()) ? next[n] : -1;
            return miniLineForType(t, rowIndexFromTop - start);
        }
        if (rowIndexFromTop == start + 4) return "";
    }

    if (rowIndexFromTop == 21) {
        if (pendingIncomingGarbage > 0) return "Garbage: +" + std::to_string(pendingIncomingGarbage);
        return "Garbage: 0";
    }
    return "";
}

static void renderFrameMultiplayer(const table& local, const OpponentState& opp, int pendingIncomingGarbage) {
    initGameConsoleOnce();
    COORD home{0, 0};
    SetConsoleCursorPosition(g_hOut, home);

    const std::string my = local.serialize_board();
    static const std::string kEmptyBoard(10 * 22, '.');
    const std::string& op = opp.hasBoard ? opp.data : kEmptyBoard;

    // títulos
    std::cout << "   VOCE";
    std::cout << "                          ";
    std::cout << "OPONENTE";
    std::cout << "\n\n";

    // 22 linhas do tabuleiro (imprime de cima para baixo)
    for (int y = 21; y >= 0; --y) {
        // board local
        for (int x = 0; x < 10; ++x) {
            char c = my[y * 10 + x];
            std::cout << '|' << (c == '#' ? '#' : ' ') << '|';
        }

        std::cout << "      ";

        // board oponente
        for (int x = 0; x < 10; ++x) {
            char c = op[y * 10 + x];
            std::cout << '|' << (c == '#' ? '#' : ' ') << '|';
        }

        // painel à direita (hold/next/garbage)
        int rowIndexFromTop = 21 - y;
        std::string panel = sidePanelLineMultiplayer(local, rowIndexFromTop, pendingIncomingGarbage);
        if (!panel.empty()) {
            std::cout << "   " << panel;
        }

        std::cout << '\n';
    }

    std::cout << "-------------------------------      -------------------------------\n";
    std::cout << "Score: " << local.get_score() << "   C=HOLD";
    std::cout << "                         ";
    std::cout << "Score: " << (opp.hasBoard ? opp.score : 0) << "\n";
    std::cout.flush();
}

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

    // rematch: -1 sem voto, 0 nao, 1 sim
    int rematchVote[2]{ -1, -1 };
    bool matchOver = false;

    volatile LONG ready = 0;    // 1 quando estiver escutando
    volatile LONG running = 1;  // 1 enquanto servidor ativo
};

static int garbageFromCleared(int cleared) {
    // regra: cada linha limpa envia 1 linha "morta" ao oponente
    if (cleared <= 0) return 0;
    if (cleared > 4) cleared = 4;
    return cleared;
}

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
        return true; // desconectar este cliente
    }

    if (line.rfind("REMATCH ", 0) == 0) {
        if (!st->matchOver) return false;

        if (line == "REMATCH YES") st->rematchVote[idx] = 1;
        else if (line == "REMATCH NO") st->rematchVote[idx] = 0;

        // se alguém disse NÃO, avisa e reseta
        if (st->rematchVote[idx] == 0) {
            if (st->clients[other] != INVALID_SOCKET) sendLine(st->clients[other], "REMATCH_ABORT");
            st->rematchVote[0] = -1;
            st->rematchVote[1] = -1;
            return false;
        }

        // ambos aceitaram
        if (st->rematchVote[0] == 1 && st->rematchVote[1] == 1) {
            if (st->clients[0] != INVALID_SOCKET) sendLine(st->clients[0], "REMATCH_START");
            if (st->clients[1] != INVALID_SOCKET) sendLine(st->clients[1], "REMATCH_START");
            st->rematchVote[0] = -1;
            st->rematchVote[1] = -1;
            st->matchOver = false;
        }
        return false;
    }

    // Repasse do estado do tabuleiro para o outro jogador
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

    // listen non-blocking para aceitar novos guests depois
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

            // se fechou par, inicia partida
            if (st->clients[0] != INVALID_SOCKET && st->clients[1] != INVALID_SOCKET) {
                st->rematchVote[0] = -1;
                st->rematchVote[1] = -1;
                st->matchOver = false;
                sendLine(st->clients[0], "START");
                sendLine(st->clients[1], "START");
            }
        }
    };

    // espera pelo host conectar primeiro
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

        // aceita novos clientes (guest) quando existir vaga
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

                // se o host saiu, encerra a sala e desconecta o guest
                if (i == 0) {
                    if (st->clients[1] != INVALID_SOCKET) {
                        sendLine(st->clients[1], "OPPONENT_LEFT");
                        closesocket(st->clients[1]);
                        st->clients[1] = INVALID_SOCKET;
                    }
                    InterlockedExchange(&st->running, 0);
                    break;
                }

                // se o guest saiu, mantém o host esperando outro
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

// -------------------- Game loop --------------------
static void runSinglePlayer() {
    table ta;
    ta.add_block();

    flushConsoleInputEvents();

    renderFrame(ta);

    DWORD lastFall = GetTickCount();
    const DWORD fallIntervalMs = 500;

    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q') {
                if (confirmLeaveToMenu()) break;
                renderFrame(ta);
                continue;
            }

            if (key == 'a') ta.block_left();
            if (key == 'd') ta.block_right();
            if (key == 's') ta.block_descend();
            if (key == 'w') ta.rotate_block();
            if (key == ' ') ta.block_drop();
            if (key == 'c') ta.hold_block();

            renderFrame(ta);
        }

        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;
            renderFrame(ta);
        }

        if (ta.is_game_over()) {
            menuClear();
            renderFrame(ta);

            int choice = postGameChoiceWithTitle(">>> GAME OVER <<<");
            if (choice == 1) {
                menuClear();
                ta.reset(true);
                flushConsoleInputEvents();
                lastFall = GetTickCount();
                renderFrame(ta);
                continue;
            }
            break;
        }

        Sleep(10);
    }

    menuClear(); // volta pro menu com tela limpa
}

static void runMultiplayerClient(SOCKET sock) {
    table ta;
    OpponentState opp;

    bool running = true;
    bool started = false;
    bool initialized = false;
    bool sentGameOver = false;
    bool isHost = false;
    bool postGameWaiting = false;
    int pendingGarbage = 0;
    int pendingIncomingGarbage = 0;

    menuClear();
    std::cout << "Multiplayer: waiting for opponent... (press q to quit)\n";
    std::cout.flush();

    flushConsoleInputEvents();

    DWORD lastFall = GetTickCount();
    const DWORD fallIntervalMs = 500;

    std::string rxBuffer;
    rxBuffer.reserve(4096);

    auto showOpponentLeft2s = [&]() {
        menuClear();
        std::cout << ">>> Oponente saiu. <<<\n";
        std::cout.flush();
        Sleep(2000);
    };

    auto showWaitingRematch = [&]() {
        menuClear();
        std::cout << "Aguardando oponente aceitar outra partida...\n";
        std::cout << "(C=HOLD | q=cancelar)\n";
        std::cout.flush();
    };

    // Se o oponente desconectar enquanto estamos travados num prompt (_getch),
    // não processamos a rede. Esta função "puxa" o socket rapidamente para
    // detectar desconexão/OPPONENT_LEFT antes de sair.
    auto opponentLeftDuringPrompt = [&]() -> bool {
        char temp[512];
        bool gotData = false;

        while (true) {
            int r = recv(sock, temp, sizeof(temp), 0);
            if (r > 0) {
                rxBuffer.append(temp, temp + r);
                gotData = true;
                continue;
            }
            if (r == 0) {
                return true; // conexão fechada
            }
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
            return true; // erro -> tratar como desconectou
        }

        if (!gotData && rxBuffer.empty()) return false;

        // busca simples: se chegou a linha OPPONENT_LEFT, considera desconectou
        // (não precisa processar tudo aqui)
        if (rxBuffer.find("OPPONENT_LEFT") != std::string::npos) return true;
        return false;
    };

    while (running) {
        // 1) Rede
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

        // 2) Parse linhas
        size_t pos = 0;
        while (true) {
            size_t nl = rxBuffer.find('\n', pos);
            if (nl == std::string::npos) break;

            std::string line = rxBuffer.substr(pos, nl - pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line == "WAITING") {
                menuClear();
                std::cout << "Waiting for opponent to join... (press q to cancel)\n";
                std::cout.flush();

                // volta ao estado de espera
                started = false;
                initialized = false;
                sentGameOver = false;
                postGameWaiting = false;
                pendingGarbage = 0;
                pendingIncomingGarbage = 0;
                opp.hasBoard = false;
                ta.reset(false);
                flushConsoleInputEvents();
            }
            else if (line == "ROLE HOST") {
                isHost = true;
            }
            else if (line == "ROLE GUEST") {
                isHost = false;
            }
            else if (line == "START") {
                started = true;
                postGameWaiting = false;
                sentGameOver = false;

                menuClear();

                // sempre começa uma partida nova limpa
                ta.reset(true);
                initialized = true;
                pendingGarbage = 0;
                pendingIncomingGarbage = 0;
                opp.hasBoard = false;
                flushConsoleInputEvents();

                lastFall = GetTickCount();
                // manda o board inicial e desenha as duas telas
                sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
                renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
            }
            else if (line == "REMATCH_START") {
                // reinicia partida se ambos aceitaram
                started = true;
                postGameWaiting = false;
                sentGameOver = false;

                menuClear();
                ta.reset(true);
                initialized = true;
                pendingIncomingGarbage = 0;
                opp.hasBoard = false;
                flushConsoleInputEvents();

                lastFall = GetTickCount();
                sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
                renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
            }
            else if (line == "REMATCH_ABORT") {
                if (initialized) {
                    menuClear();
                    if (!postGameWaiting) {
                        renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
                    } else {
                        showWaitingRematch();
                    }
                    std::cout << "\n>>> O oponente nao aceitou outra partida. <<<\n";
                    std::cout.flush();
                    Sleep(2000);
                }
                running = false;
            }
            else if (line.rfind("GARBAGE ", 0) == 0) {
                int n = std::stoi(line.substr(8));
                if (!initialized) pendingGarbage += n;
                else {
                    // lixo recebido agora fica pendente (aplica quando a peça travar)
                    pendingIncomingGarbage += n;
                    sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
                    if (!postGameWaiting) {
                        renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
                    }
                }
            }
            else if (line.rfind("BOARD ", 0) == 0) {
                // formato: BOARD <score> <220chars>
                // parse simples: encontra primeiro e segundo espaço
                size_t p1 = line.find(' ');
                size_t p2 = (p1 == std::string::npos) ? std::string::npos : line.find(' ', p1 + 1);
                if (p2 != std::string::npos) {
                    try {
                        opp.score = std::stoi(line.substr(p1 + 1, p2 - (p1 + 1)));
                        std::string data = line.substr(p2 + 1);
                        if (data.size() == 10 * 22) {
                            opp.data = std::move(data);
                            opp.hasBoard = true;
                            if (initialized && !postGameWaiting) {
                                renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
                            }
                        }
                    } catch (...) {
                        // ignora linha mal formada
                    }
                }
            }
            else if (line == "YOU_WIN") {
                if (initialized) {
                    menuClear();
                    renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
                }

                int choice = postGameChoiceWithTitle(">>> VOCE VENCEU | OPONENTE PERDEU <<<");
                if (choice == 1) {
                    sendLine(sock, "REMATCH YES");
                    postGameWaiting = true;
                    showWaitingRematch();
                } else {
                    if (opponentLeftDuringPrompt()) {
                        showOpponentLeft2s();
                    }
                    sendLine(sock, "LEAVE");
                    running = false;
                }
            }
            else if (line == "OPPONENT_LEFT") {
                menuClear();
                std::cout << ">>> Oponente saiu. <<<\n";
                std::cout.flush();

                // deixa a mensagem visível por pelo menos 2s
                Sleep(2000);

                if (isHost) {
                    // host volta a esperar outro jogador
                    menuClear();
                    std::cout << "Oponente saiu. Aguardando outro jogador... (press q to cancel)\n";
                    std::cout.flush();
                    started = false;
                    initialized = false;
                    sentGameOver = false;
                    postGameWaiting = false;
                    pendingGarbage = 0;
                    pendingIncomingGarbage = 0;
                    opp.hasBoard = false;
                    ta.reset(false);
                    flushConsoleInputEvents();
                } else {
                    running = false;
                }
            }

            pos = nl + 1;
        }
        if (pos > 0) rxBuffer.erase(0, pos);

        // 3) Antes do START
        if (!started || postGameWaiting) {
            if (_kbhit()) {
                char key = _getch();
                if (key == 'q') {
                    if (confirmLeaveToMenu()) {
                        sendLine(sock, "LEAVE");
                        running = false;
                        break;
                    }
                    if (postGameWaiting) {
                        showWaitingRematch();
                    } else {
                        menuClear();
                        std::cout << "Waiting for opponent to join... (press q to cancel)\n";
                        std::cout.flush();
                    }
                }
            }
            Sleep(30);
            continue;
        }

        if (started && !initialized) {
            Sleep(30);
            continue;
        }

        // 4) Input
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q') {
                if (confirmLeaveToMenu()) {
                    sendLine(sock, "LEAVE");
                    running = false;
                    break;
                }
                if (initialized) renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
                continue;
            }

            if (key == 'a') ta.block_left();
            if (key == 'd') ta.block_right();
            if (key == 's') ta.block_descend();
            if (key == 'w') ta.rotate_block();
            if (key == ' ') ta.block_drop();
            if (key == 'c') ta.hold_block();

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) {
                sendLine(sock, "CLEARED " + std::to_string(cleared));
            }

            // aplica lixo pendente somente depois que a peça travar
            int landed = ta.pop_landed_event();
            if (landed > 0 && pendingIncomingGarbage > 0) {
                ta.apply_garbage(pendingIncomingGarbage);
                pendingIncomingGarbage = 0;
            }

            // envia seu board e renderiza as duas telas
            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
        }

        // 5) Queda automática
        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) {
                sendLine(sock, "CLEARED " + std::to_string(cleared));
            }

            int landed = ta.pop_landed_event();
            if (landed > 0 && pendingIncomingGarbage > 0) {
                ta.apply_garbage(pendingIncomingGarbage);
                pendingIncomingGarbage = 0;
            }

            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);
        }

        // 6) Game over
        if (ta.is_game_over() && !sentGameOver) {
            sendLine(sock, "GAMEOVER");
            sentGameOver = true;

            // garante que o oponente veja seu board final
            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            menuClear();
            renderFrameMultiplayer(ta, opp, pendingIncomingGarbage);

            int choice = postGameChoiceWithTitle(">>> VOCE PERDEU | OPONENTE VENCEU <<<");
            if (choice == 1) {
                sendLine(sock, "REMATCH YES");
                postGameWaiting = true;
                showWaitingRematch();
            } else {
                if (opponentLeftDuringPrompt()) {
                    showOpponentLeft2s();
                }
                sendLine(sock, "LEAVE");
                break;
            }
        }

        Sleep(10);
    }

    menuClear();
}

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

    // pega handle do console cedo (pra menuClear funcionar)
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);

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

                ServerState* st = new ServerState();
                st->port = port;

                DWORD tid = 0;
                HANDLE hThread = CreateThread(nullptr, 0, serverThreadProc, st, 0, &tid);
                if (!hThread) {
                    delete st;
                    std::cout << "Could not start server thread.\nPress ENTER...\n";
                    std::cout.flush();
                    std::string dummy; std::getline(std::cin, dummy);
                    continue;
                }

                DWORD t0 = GetTickCount();
                while (InterlockedCompareExchange(&st->ready, 0, 0) == 0) {
                    if (GetTickCount() - t0 > 3000) break;
                    Sleep(50);
                }
                if (InterlockedCompareExchange(&st->ready, 0, 0) == 0) {
                    std::cout << "Server failed to start (port in use?).\n";
                    InterlockedExchange(&st->running, 0);
                    WaitForSingleObject(hThread, INFINITE);
                    CloseHandle(hThread);
                    delete st;
                    std::cout << "Press ENTER...\n";
                    std::cout.flush();
                    std::string dummy; std::getline(std::cin, dummy);
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
                    InterlockedExchange(&st->running, 0);
                    WaitForSingleObject(hThread, INFINITE);
                    CloseHandle(hThread);
                    delete st;
                    std::cout << "Press ENTER...\n";
                    std::cout.flush();
                    std::getline(std::cin, dummy);
                    continue;
                }

                runMultiplayerClient(sock);

                closesocket(sock);
                InterlockedExchange(&st->running, 0);
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);
                delete st;
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
