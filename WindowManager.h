#pragma once

#include <SFML/Graphics.hpp>

#include "GameState.h"

class WindowManager {
private:
    sf::RenderWindow window;
    GameState current_state;

public:
    WindowManager();

    void setState(GameState state) { current_state = state; }
    GameState getState() const { return current_state; }

    sf::RenderWindow& getWindow() { return window; }
    bool isOpen() const { return window.isOpen(); }
};
