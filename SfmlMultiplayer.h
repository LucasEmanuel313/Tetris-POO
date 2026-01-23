#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#include <chrono>
#include <iostream>
#include <string>

#include <SFML/Graphics.hpp>

#include "Button.h"
#include "GameState.h"
#include "MainMenu.h"
#include "MusicManager.h"
#include "SfmlGame.h"
#include "TextField.h"
#include "net_client.h"
#include "server.h"
#include "table.h"

struct SfmlOpponentState {
    bool hasBoard = false;
    int score = 0;
    std::string data;

    SfmlOpponentState() : data(10 * 22, '.') {}
};

class SfmlOpponentRenderer {
    static constexpr int COLS = 10;
    static constexpr int ROWS = 22;
    static constexpr float BLOCK = 30.f;

    sf::RectangleShape cells[COLS][ROWS];
    sf::Vector2f origin;

public:
    explicit SfmlOpponentRenderer(sf::Vector2f origin_) : origin(origin_) {
        for (int x = 0; x < COLS; ++x) {
            for (int y = 0; y < ROWS; ++y) {
                cells[x][y].setSize({BLOCK, BLOCK});
                cells[x][y].setOutlineThickness(2.f);
                cells[x][y].setOutlineColor(sf::Color::Black);
            }
        }
    }

    void draw(sf::RenderWindow& window, const SfmlOpponentState& opp) {
        const std::string empty(10 * 22, '.');
        const std::string& data = opp.hasBoard ? opp.data : empty;

        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                char c = data[y * 10 + x];
                float sx = origin.x + x * BLOCK;
                float sy = origin.y + (ROWS - 1 - y) * BLOCK;
                cells[x][y].setPosition({sx, sy});
                cells[x][y].setFillColor((c == '#') ? sf::Color(80, 80, 80) : sf::Color::White);
                window.draw(cells[x][y]);
            }
        }
    }
};

class SfmlMultiplayer {
public:
    enum class Mode {
        Menu,
        Waiting,
        Playing,
        PostGame
    };

private:
    // UI
    Button hostBtn;
    Button joinBtn;
    Button backBtn;
    Button connectBtn;

    TextField ipField;
    TextField portField;

    sf::Text title;
    sf::Text hint;

    // networking
    ServerHandle server;
    bool hosting = false;
    SOCKET sock = INVALID_SOCKET;

    bool running = false;
    bool started = false;
    bool initialized = false;
    bool sentGameOver = false;
    bool isHost = false;
    bool postGameWaiting = false;

    int pendingIncomingGarbage = 0;

    std::string rxBuffer;

    // gameplay
    table ta;
    SfmlOpponentState opp;

    SfmlGame localRenderer;
    SfmlOpponentRenderer oppRenderer;

    using clock = std::chrono::steady_clock;
    clock::time_point lastFall;
    std::chrono::milliseconds fallInterval{500};

    Mode mode = Mode::Menu;
    std::string statusLine;

    int enterCooldownFrames = 0;

    // post game
    Button rematchBtn;
    Button leaveBtn;

    void closeSock() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
    }

    void stopHosting() {
        if (hosting) {
            stopServer(server);
            hosting = false;
        }
    }

    void setStatus(const std::string& s) {
        statusLine = s;
        hint.setString(statusLine);
    }

    void startClient(const std::string& ip, int port, bool host) {
        isHost = host;

        SOCKET s = INVALID_SOCKET;
        if (!connectToServer(ip, port, s)) {
            setStatus("Falha ao conectar.");
            return;
        }

        sock = s;
        running = true;
        started = false;
        initialized = false;
        sentGameOver = false;
        postGameWaiting = false;
        pendingIncomingGarbage = 0;
        opp.hasBoard = false;
        ta.reset(false);
        rxBuffer.clear();

        lastFall = clock::now();
        mode = Mode::Waiting;
        setStatus("Conectado. Aguardando oponente...");
    }

    void processLine(const std::string& line) {
        if (line == "WAITING") {
            started = false;
            initialized = false;
            sentGameOver = false;
            postGameWaiting = false;
            pendingIncomingGarbage = 0;
            opp.hasBoard = false;
            ta.reset(false);
            mode = Mode::Waiting;
            setStatus("Aguardando oponente...");
            return;
        }
        if (line == "ROLE HOST") { isHost = true; return; }
        if (line == "ROLE GUEST") { isHost = false; return; }

        if (line == "START" || line == "REMATCH_START") {
            started = true;
            postGameWaiting = false;
            sentGameOver = false;

            ta.reset(true);
            initialized = true;
            pendingIncomingGarbage = 0;
            opp.hasBoard = false;

            lastFall = clock::now();

            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            mode = Mode::Playing;
            setStatus("Partida iniciada!");
            return;
        }

        if (line == "REMATCH_ABORT") {
            setStatus("O oponente nao aceitou revanche.");
            running = false;
            return;
        }

        if (line.rfind("GARBAGE ", 0) == 0) {
            int n = std::stoi(line.substr(8));
            if (!initialized) {
                // ignorado aqui (console tinha pendingGarbage pre-start)
            } else {
                pendingIncomingGarbage += n;
                sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            }
            return;
        }

        if (line.rfind("BOARD ", 0) == 0) {
            size_t p1 = line.find(' ');
            size_t p2 = (p1 == std::string::npos) ? std::string::npos : line.find(' ', p1 + 1);
            if (p2 != std::string::npos) {
                try {
                    opp.score = std::stoi(line.substr(p1 + 1, p2 - (p1 + 1)));
                    std::string data = line.substr(p2 + 1);
                    if (data.size() == 10 * 22) {
                        opp.data = std::move(data);
                        opp.hasBoard = true;
                    }
                } catch (...) {
                }
            }
            return;
        }

        if (line == "YOU_WIN") {
            mode = Mode::PostGame;
            postGameWaiting = false;
            setStatus("VOCE VENCEU!");
            return;
        }

        if (line == "OPPONENT_LEFT") {
            // guest sai; host volta a esperar
            started = false;
            initialized = false;
            sentGameOver = false;
            postGameWaiting = false;
            pendingIncomingGarbage = 0;
            opp.hasBoard = false;
            ta.reset(false);

            if (isHost) {
                mode = Mode::Waiting;
                setStatus("Oponente saiu. Aguardando outro...");
            } else {
                running = false;
            }
            return;
        }
    }

    void pumpNetwork() {
        if (sock == INVALID_SOCKET) return;

        char temp[512];
        while (true) {
            int r = recv(sock, temp, sizeof(temp), 0);
            if (r > 0) {
                rxBuffer.append(temp, temp + r);
                continue;
            }
            if (r == 0) { running = false; return; }

            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
            running = false;
            return;
        }

        size_t pos = 0;
        while (true) {
            size_t nl = rxBuffer.find('\n', pos);
            if (nl == std::string::npos) break;
            std::string line = rxBuffer.substr(pos, nl - pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            processLine(line);
            pos = nl + 1;
        }
        if (pos > 0) rxBuffer.erase(0, pos);
    }

public:
    SfmlMultiplayer(const sf::Font& font, sf::RenderWindow& window)
        : hostBtn(font, {500.f, 250.f}, "Host"),
          joinBtn(font, {500.f, 330.f}, "Join"),
          backBtn(font, {500.f, 410.f}, "Back"),
          connectBtn(font, {500.f, 490.f}, "Connect"),
          ipField(font, {460.f, 250.f}, {280.f, 40.f}),
          portField(font, {460.f, 310.f}, {280.f, 40.f}),
          title(font),
          hint(font),
          ta(),
          opp(),
          localRenderer(ta, window, font),
          oppRenderer({SfmlGame::GRID_POS_X - (SfmlGame::GRID_WIDTH + 60.f), SfmlGame::GRID_POS_Y}),
          rematchBtn(font, {500.f, 520.f}, "Rematch"),
          leaveBtn(font, {500.f, 600.f}, "Leave") {

        title.setString("Multiplayer");
        title.setCharacterSize(28);
        title.setFillColor(sf::Color::Black);
        title.setPosition({500.f, 160.f});

        hint.setCharacterSize(18);
        hint.setFillColor(sf::Color::Black);
        hint.setPosition({420.f, 700.f});

        ipField.setAllowDot(true);
        ipField.setMaxLen(32);
        ipField.setValue("127.0.0.1");

        portField.setAllowDot(false);
        portField.setMaxLen(5);
        portField.setValue("5555");

        setStatus("Escolha Host ou Join");
    }

    void onEnter() {
        // Evita que o mesmo clique que trocou o estado do menu acione um botão aqui.
        enterCooldownFrames = 2;
    }

    ~SfmlMultiplayer() {
        if (sock != INVALID_SOCKET) {
            sendLine(sock, "LEAVE");
        }
        closeSock();
        stopHosting();
    }

    void handleEvent(const sf::Event& ev, sf::RenderWindow& window) {
        ipField.handleEvent(ev, window);
        portField.handleEvent(ev, window);

        if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) {
                // sai do multiplayer
                if (sock != INVALID_SOCKET) sendLine(sock, "LEAVE");
                running = false;
            }
        }
    }

    // retorna true se quiser voltar pro menu principal
    bool update(Mouse& mouse) {
        // network tick
        if (running) pumpNetwork();
        if (running == false && mode != Mode::Menu) {
            closeSock();
            stopHosting();
            mode = Mode::Menu;
            setStatus("Desconectado.");
        }

        // logic: playing
        if (mode == Mode::Playing) {
            auto now = clock::now();
            if (now - lastFall >= fallInterval) {
                ta.block_descend();
                lastFall = now;

                int cleared = ta.pop_cleared_lines_event();
                if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

                int landed = ta.pop_landed_event();
                if (landed > 0) {
                    if (pendingIncomingGarbage > 0) {
                        ta.apply_garbage(pendingIncomingGarbage);
                        pendingIncomingGarbage = 0;
                    }
                    ta.spawn_if_needed();
                }

                sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            }

            // controles
            localRenderer.HandleEvents();

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

            int landed = ta.pop_landed_event();
            if (landed > 0) {
                if (pendingIncomingGarbage > 0) {
                    ta.apply_garbage(pendingIncomingGarbage);
                    pendingIncomingGarbage = 0;
                }
                ta.spawn_if_needed();
            }

            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());

            if (ta.is_game_over() && !sentGameOver) {
                sendLine(sock, "GAMEOVER");
                sentGameOver = true;
                sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
                mode = Mode::PostGame;
                setStatus("VOCE PERDEU");
            }
        }

        // UI interactions
        if (mode == Mode::Menu) {
            if (enterCooldownFrames > 0) {
                --enterCooldownFrames;
                return false;
            }
            hostBtn.Update(mouse);
            joinBtn.Update(mouse);
            backBtn.Update(mouse);

            if (hostBtn.getOnRelease()) {
                int port = 5555;
                try { port = std::stoi(portField.getValue()); } catch (...) {}

                std::string err;
                if (!startServer(port, server, err)) {
                    setStatus(err);
                } else {
                    hosting = true;
                    startClient("127.0.0.1", port, true);
                    setStatus("Sala criada. Aguardando oponente...");
                }
            }

            if (joinBtn.getOnRelease()) {
                int port = 5555;
                try { port = std::stoi(portField.getValue()); } catch (...) {}
                startClient(ipField.getValue(), port, false);
            }

            if (backBtn.getOnRelease()) {
                return true;
            }
        }

        if (mode == Mode::PostGame) {
            rematchBtn.Update(mouse);
            leaveBtn.Update(mouse);

            if (rematchBtn.getOnRelease()) {
                sendLine(sock, "REMATCH YES");
                postGameWaiting = true;
                mode = Mode::Waiting;
                setStatus("Aguardando oponente aceitar revanche...");
            }
            if (leaveBtn.getOnRelease()) {
                sendLine(sock, "LEAVE");
                running = false;
            }
        }

        return false;
    }

    void draw(sf::RenderWindow& window) {
        if (mode == Mode::Menu) {
            window.draw(title);

            sf::Text ipLabel(title);
            ipLabel.setString("IP:");
            ipLabel.setCharacterSize(18);
            ipLabel.setPosition({420.f, 255.f});
            window.draw(ipLabel);

            sf::Text portLabel(title);
            portLabel.setString("Port:");
            portLabel.setCharacterSize(18);
            portLabel.setPosition({420.f, 315.f});
            window.draw(portLabel);

            ipField.draw(window);
            portField.draw(window);

            hostBtn.draw(window);
            joinBtn.draw(window);
            backBtn.draw(window);
        }
        else if (mode == Mode::Waiting) {
            sf::Text t(title);
            t.setString(statusLine);
            t.setCharacterSize(20);
            t.setPosition({360.f, 160.f});
            window.draw(t);

            oppRenderer.draw(window, opp);
            localRenderer.draw_game();
        }
        else if (mode == Mode::Playing) {
            oppRenderer.draw(window, opp);
            localRenderer.draw_game();

            sf::Text g(title);
            g.setString("Garbage: +" + std::to_string(pendingIncomingGarbage));
            g.setCharacterSize(16);
            g.setPosition({SfmlGame::PANEL_POS_X, SfmlGame::GRID_POS_Y + 700.f});
            g.setFillColor(sf::Color::Black);
            window.draw(g);
        }
        else if (mode == Mode::PostGame) {
            oppRenderer.draw(window, opp);
            localRenderer.draw_game();

            sf::RectangleShape overlay;
            overlay.setSize({1200.f, 800.f});
            overlay.setFillColor(sf::Color(0, 0, 0, 120));
            window.draw(overlay);

            sf::Text msg(title);
            msg.setString(statusLine);
            msg.setCharacterSize(28);
            msg.setFillColor(sf::Color::White);
            msg.setPosition({450.f, 420.f});
            window.draw(msg);

            rematchBtn.draw(window);
            leaveBtn.draw(window);
        }

        window.draw(hint);
    }
};
