#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#include <conio.h>
#include <iostream>
#include <string>

#include "console_ui.h"
#include "game_modes.h"
#include "net_client.h"
#include "table.h"

void runSinglePlayer() {
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

            ta.spawn_if_needed();

            renderFrame(ta);
        }

        DWORD now = GetTickCount();
        if (now - lastFall >= fallIntervalMs) {
            ta.block_descend();
            lastFall = now;

            ta.spawn_if_needed();
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

void runMultiplayerClient(SOCKET sock) {
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
        std::cout << "(q=cancelar)\n";
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

                // O servidor só manda WAITING pro host (slot 0)
                isHost = true;

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

                // interrompe a partida imediatamente (não deixa o host continuar jogando)
                started = false;
                initialized = false;
                sentGameOver = false;
                postGameWaiting = false;
                pendingGarbage = 0;
                pendingIncomingGarbage = 0;
                opp.hasBoard = false;
                ta.reset(false);
                flushConsoleInputEvents();

                // deixa a mensagem visível por pelo menos 2s
                Sleep(2000);

                if (isHost) {
                    // host volta a esperar outro jogador
                    menuClear();
                    std::cout << "Oponente saiu. Aguardando outro jogador... (press q to cancel)\n";
                    std::cout.flush();
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
            if (landed > 0) {
                if (pendingIncomingGarbage > 0) {
                    ta.apply_garbage(pendingIncomingGarbage);
                    pendingIncomingGarbage = 0;
                }
                ta.spawn_if_needed();
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
            if (landed > 0) {
                if (pendingIncomingGarbage > 0) {
                    ta.apply_garbage(pendingIncomingGarbage);
                    pendingIncomingGarbage = 0;
                }
                ta.spawn_if_needed();
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
