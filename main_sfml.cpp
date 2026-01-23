#include <chrono>
#include <iostream>
#include <optional>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include <SFML/Graphics.hpp>

#include "GameState.h"
#include "MainMenu.h"
#include "Mouse.h"
#include "MusicManager.h"
#include "SfmlGame.h"
#include "SfmlMultiplayer.h"
#include "WindowManager.h"
#include "table.h"

static bool tryLoadFont(sf::Font& font) {
    return font.openFromFile("Tetris.ttf") || font.openFromFile("tomerge/Tetris.ttf");
}

static bool tryLoadTexture(sf::Texture& tex) {
    return tex.loadFromFile("Images/Background_Tetris.jpg") || tex.loadFromFile("tomerge/Images/Background_Tetris.jpg");
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    using clock = std::chrono::steady_clock;

    WindowManager windowManager;
    sf::RenderWindow& window = windowManager.getWindow();

    GameState current_state = GameState::MENU;
    bool exitRequested = false;

    sf::Font font;
    if (!tryLoadFont(font)) {
        std::cerr << "Warning: could not load Tetris.ttf (tried root and tomerge).\n";
    }

    sf::Texture bgTexture;
    std::optional<sf::Sprite> bgSprite;
    bool hasBg = tryLoadTexture(bgTexture);
    if (hasBg) {
        bgSprite.emplace(bgTexture);
    }

    MusicManager musicManager;

    table ta;
    ta.add_block();

    SfmlGame game(ta, window, font);
    MainMenu menu(font);
    SfmlMultiplayer mp(font, window);
    Mouse mouse;

    bool showGameOver = false;

    auto lastFall = clock::now();
    const auto fallInterval = std::chrono::milliseconds(500);

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (current_state == GameState::MULTIPLAYER) {
                mp.handleEvent(*event, window);
            }

            if (current_state == GameState::GAME_OVER) {
                if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                    if (kp->code == sf::Keyboard::Key::Enter) {
                        ta.reset(true);
                        showGameOver = false;
                        current_state = GameState::SINGLEPLAYER;
                        lastFall = clock::now();
                    }
                    if (kp->code == sf::Keyboard::Key::Escape) {
                        ta.reset(true);
                        showGameOver = false;
                        current_state = GameState::MENU;
                    }
                }
            }
        }

        mouse.Update(window);

        if (current_state == GameState::MENU) {
            menu.update(mouse, current_state, exitRequested);
            if (exitRequested) window.close();
        }

        if (current_state == GameState::MULTIPLAYER) {
            bool backToMenu = mp.update(mouse);
            if (backToMenu) {
                current_state = GameState::MENU;
            }
        }

        auto now = clock::now();
        if (current_state == GameState::SINGLEPLAYER) {
            if (now - lastFall >= fallInterval) {
                ta.block_descend();
                ta.spawn_if_needed();
                lastFall = now;
            }
            game.HandleEvents();

            if (ta.is_game_over()) {
                showGameOver = true;
                current_state = GameState::GAME_OVER;
            }
        }

        // render
        window.clear(sf::Color::White);
        if (bgSprite.has_value()) window.draw(*bgSprite);

        musicManager.updateMusic(current_state);

        if (current_state == GameState::MENU) {
            menu.draw(window);
        } else if (current_state == GameState::SINGLEPLAYER) {
            game.draw_game();
        } else if (current_state == GameState::MULTIPLAYER) {
            mp.draw(window);
        } else if (current_state == GameState::GAME_OVER) {
            game.draw_game();
            if (showGameOver) {
                sf::RectangleShape overlay;
                overlay.setSize({1200.f, 800.f});
                overlay.setFillColor(sf::Color(0, 0, 0, 140));
                window.draw(overlay);

                sf::Text msg(font);
                msg.setString("GAME OVER\nENTER = restart\nESC = menu");
                msg.setCharacterSize(32);
                msg.setFillColor(sf::Color::White);
                msg.setPosition({360.f, 320.f});
                window.draw(msg);
            }
        }

        window.display();
    }

    WSACleanup();
    return 0;
}
