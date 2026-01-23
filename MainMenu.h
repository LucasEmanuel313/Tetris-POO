#pragma once

#include <SFML/Graphics.hpp>

#include "Button.h"
#include "GameState.h"
#include "Mouse.h"

class MainMenu {
    Button singleButton;
    Button multiButton;
    Button exitButton;

public:
    explicit MainMenu(const sf::Font& font)
        : singleButton(font, {500.f, 250.f}, "Single Player"),
          multiButton(font, {500.f, 350.f}, "Multiplayer"),
          exitButton(font, {500.f, 450.f}, "Exit") {}

    void draw(sf::RenderWindow& window) {
        singleButton.draw(window);
        multiButton.draw(window);
        exitButton.draw(window);
    }

    void update(Mouse& mouse, GameState& current_state, bool& exitRequested) {
        singleButton.Update(mouse);
        multiButton.Update(mouse);
        exitButton.Update(mouse);

        if (singleButton.getOnRelease()) {
            current_state = GameState::SINGLEPLAYER;
        }
        if (multiButton.getOnRelease()) {
            current_state = GameState::MULTIPLAYER;
        }
        if (exitButton.getOnRelease()) {
            exitRequested = true;
        }
    }
};
