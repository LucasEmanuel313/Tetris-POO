#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class TextField {
    sf::RectangleShape box;
    sf::Text text;
    bool focused = false;

    std::string value;
    std::size_t maxLen = 32;

    bool allowDigits = true;
    bool allowDot = false;

public:
    TextField(const sf::Font& font, sf::Vector2f pos, sf::Vector2f size)
        : text(font) {
        box.setPosition(pos);
        box.setSize(size);
        box.setFillColor(sf::Color(255, 255, 255, 200));
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color::Black);

        text.setCharacterSize(18);
        text.setFillColor(sf::Color::Black);
        text.setPosition({pos.x + 8.f, pos.y + 6.f});
    }

    void setMaxLen(std::size_t n) { maxLen = n; }
    void setAllowDot(bool v) { allowDot = v; }

    void setValue(const std::string& v) {
        value = v;
        if (value.size() > maxLen) value.resize(maxLen);
        text.setString(value);
    }

    const std::string& getValue() const { return value; }

    bool isFocused() const { return focused; }

    void handleEvent(const sf::Event& ev, const sf::RenderWindow& window) {
        if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
            const sf::Vector2i pixelPos = mb->position;
            sf::Vector2f mouse = window.mapPixelToCoords(pixelPos);
            focused = box.getGlobalBounds().contains(mouse);
            box.setOutlineColor(focused ? sf::Color(30, 120, 255) : sf::Color::Black);
        }

        if (!focused) return;

        if (const auto* te = ev.getIf<sf::Event::TextEntered>()) {
            // backspace
            if (te->unicode == 8) {
                if (!value.empty()) value.pop_back();
                text.setString(value);
                return;
            }
            // ignore non-ascii
            if (te->unicode < 32 || te->unicode > 126) return;
            if (value.size() >= maxLen) return;

            char c = static_cast<char>(te->unicode);
            if (allowDigits && (c >= '0' && c <= '9')) {
                value.push_back(c);
            } else if (allowDot && c == '.') {
                value.push_back(c);
            }
            text.setString(value);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        window.draw(text);
    }
};
