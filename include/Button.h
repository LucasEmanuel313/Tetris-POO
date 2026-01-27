#pragma once

#include <SFML/Graphics.hpp>
#include <string>

#include "Mouse.h"

class Button {
    bool isOver = false;
    bool isPressed = false;
    bool isPressedInside = false;
    bool onRelease = false;

    sf::Color defaultColor = sf::Color(150, 150, 150);
    sf::Color hoverColor = sf::Color(200, 200, 200);
    sf::Color pressedColor = sf::Color(100, 100, 100);

    sf::Color defaultTextColor = sf::Color::Black;
    sf::Color hoverTextColor = sf::Color::Black;
    sf::Color pressedTextColor = sf::Color::White;

    sf::RectangleShape button_shape;
    sf::Text button_text;

    void apply_style() {
        button_shape.setSize({200.f, 70.f});
        button_shape.setOutlineThickness(2.f);
        button_shape.setOutlineColor(sf::Color::Black);

        button_text.setCharacterSize(20);

        if (isPressed) {
            button_shape.setFillColor(pressedColor);
            button_text.setFillColor(pressedTextColor);
        } else if (isOver) {
            button_shape.setFillColor(hoverColor);
            button_text.setFillColor(hoverTextColor);
        } else {
            button_shape.setFillColor(defaultColor);
            button_text.setFillColor(defaultTextColor);
        }
    }

public:
    Button(const sf::Font& font, sf::Vector2f position, const std::string& text)
        : button_text(font) {
        button_shape.setPosition(position);
        button_text.setPosition({position.x + 30.f, position.y + 10.f});
        button_text.setString(text);
    }

    void draw(sf::RenderWindow& window) {
        apply_style();
        window.draw(button_shape);
        window.draw(button_text);
    }

    void Update(Mouse& mouse) {
        onRelease = false;

        const bool contains = button_shape.getGlobalBounds().contains(mouse.position);
        isOver = contains;

        if (contains) {
            if (mouse.isPressed) {
                isPressed = true;
                isPressedInside = true;
            } else {
                if (isPressed && isPressedInside) {
                    onRelease = true;
                }
                isPressed = false;
                isPressedInside = false;
            }
        } else {
            if (isPressed && isPressedInside) {
                onRelease = true;
            }
            isPressed = false;
            isPressedInside = false;
        }
    }

    void setPosition(sf::Vector2f position) {
        button_shape.setPosition(position);
        button_text.setPosition({position.x + 30.f, position.y + 10.f});
    }

    bool getOnRelease() const {
        return onRelease;
    }
};
