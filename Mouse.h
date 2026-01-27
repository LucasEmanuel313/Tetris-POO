#pragma once

#include <SFML/Graphics.hpp>

class Mouse {
public:
    bool isPressed = false;
    bool onPress = false;
    bool onRelease = false;

    sf::Vector2f position;

    void Update(sf::RenderWindow& window) {
        const sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        position = window.mapPixelToCoords(pixelPos);

        onPress = false;
        onRelease = false;

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            if (!isPressed) {
                onPress = true;
            }
            isPressed = true;
        } else {
            if (isPressed) {
                onRelease = true;
            }
            isPressed = false;
        }
    }
};
