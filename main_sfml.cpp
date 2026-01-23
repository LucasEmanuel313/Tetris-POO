#include <chrono>
#include <iostream>
#include <optional>

#include <SFML/Graphics.hpp>

#include "GameState.h"
#include "MainMenu.h"
#include "Mouse.h"
#include "MusicManager.h"
#include "SfmlGame.h"
#include "WindowManager.h"
#include "table.h"

static bool tryLoadFont(sf::Font& font) {
    return font.openFromFile("Tetris.ttf") || font.openFromFile("tomerge/Tetris.ttf");
}

static bool tryLoadTexture(sf::Texture& tex) {
    return tex.loadFromFile("Images/Background_Tetris.jpg") || tex.loadFromFile("tomerge/Images/Background_Tetris.jpg");
}

int main() {
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
    Mouse mouse;

    auto lastFall = clock::now();
    const auto fallInterval = std::chrono::milliseconds(500);

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        mouse.Update(window);

        if (current_state == GameState::MENU) {
            menu.update(mouse, current_state, exitRequested);
            if (exitRequested) window.close();
        }

        auto now = clock::now();
        if (current_state == GameState::SINGLEPLAYER) {
            if (now - lastFall >= fallInterval) {
                ta.block_descend();
                ta.spawn_if_needed();
                lastFall = now;
            }
            game.HandleEvents();
        }

        // render
        window.clear(sf::Color::White);
        if (bgSprite.has_value()) window.draw(*bgSprite);

        musicManager.updateMusic(current_state);

        if (current_state == GameState::MENU) {
            menu.draw(window);
        } else if (current_state == GameState::SINGLEPLAYER) {
            game.draw_game();
        }

        window.display();
    }

    return 0;
}
