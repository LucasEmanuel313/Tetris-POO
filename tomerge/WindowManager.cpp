#include <SFML/Graphics.hpp>
#include <iostream>
#include "Button.h"
#include "GameState.h"

class WindowManager {
private:
    sf::RenderWindow window;
    GameState current_state;
    
public:
    WindowManager() : window(sf::VideoMode({1200, 800}), "Tetris") {
        window.setFramerateLimit(60);
        current_state = GameState::MENU;
    }
    
    void setState(GameState state) { current_state = state; }
    GameState getState() const { return current_state; }
    
    sf::RenderWindow& getWindow() { return window; }
    bool isOpen() { return window.isOpen(); }
};