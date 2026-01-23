#include "WindowManager.h"

WindowManager::WindowManager()
    : window(sf::VideoMode({1200, 800}), "Tetris"), current_state(GameState::MENU) {
    window.setFramerateLimit(60);
}
