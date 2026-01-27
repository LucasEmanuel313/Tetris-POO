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
#include "Game.h"
#include "TextField.h"
#include "UiTextBox.h"
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

    static sf::Color colorForType(char c) {
        switch (c) {
            case '0': return sf::Color(0, 150, 150);   // I
            case '1': return sf::Color(0, 128, 0);     // J
            case '2': return sf::Color(180, 100, 0);   // L
            case '3': return sf::Color(180, 180, 0);   // O
            case '4': return sf::Color(150, 0, 0);     // S
            case '5': return sf::Color(150, 0, 150);   // T
            case '6': return sf::Color(0, 0, 150);     // Z
            default: return sf::Color(200, 200, 200);
        }
    }

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
                cells[x][y].setFillColor((c == '.') ? sf::Color::White : colorForType(c));
                std::cout << "Character received: " << c << std::endl;
                window.draw(cells[x][y]);
            }
        }
    }
};

class SfmlMultiplayer {
public:
    enum class Mode {
        Menu,
        Connecting,
        Waiting,
        Playing,
        PostGame
    };

    enum class MenuPane {
        Root,
        JoinForm
    };

private:
    // UI
    Button hostBtn;
    Button joinBtn;
    Button backBtn;
    Button connectBtn;
    Button cancelConnectBtn;

    TextField ipField;
    TextField portField;

    sf::Text title;
    sf::Text hint;

    // networking
    ServerHandle server;
    bool hosting = false;
    SOCKET sock = INVALID_SOCKET;

    bool backToMainMenuRequested = false;

    std::string sessionIp;
    int sessionPort = 0;

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

    clock::time_point connectStart;
    std::chrono::milliseconds connectTimeout{5000};

    Mode mode = Mode::Menu;
    std::string statusLine;

    MenuPane menuPane = MenuPane::Root;

    int enterCooldownFrames = 0;

    bool pauseConfirmActive = false;

    // post game
    Button rematchBtn;
    Button leaveBtn;

    void closeSock();
    void stopHosting();
    void setStatus(const std::string& s);
    void applyMenuLayout();
    void startClient(const std::string& ip, int port, bool host);
    void beginConnect(const std::string& ip, int port, bool host);
    bool pollConnect(bool& outConnected, std::string& outError);
    void startSessionAfterConnect();
    void processLine(const std::string& line);
    void pumpNetwork();

public:
    SfmlMultiplayer(const sf::Font& font, sf::RenderWindow& window);
    ~SfmlMultiplayer();

    void onEnter();
    void handleEvent(const sf::Event& ev, sf::RenderWindow& window);

    // retorna true se quiser voltar pro menu principal
    bool update(Mouse& mouse);
    void draw(sf::RenderWindow& window);
};
