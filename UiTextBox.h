#pragma once

#include <SFML/Graphics.hpp>

inline void drawTextWithBox(sf::RenderTarget& target,
                           sf::Text& text,
                           float padding = 6.f,
                           sf::Color bgColor = sf::Color(255, 255, 255, 220),
                           sf::Color outlineColor = sf::Color::Black,
                           float outlineThickness = 2.f) {
    const sf::FloatRect b = text.getLocalBounds();
    const sf::Vector2f p = text.getPosition();

    sf::RectangleShape box;
    box.setPosition({p.x + b.position.x - padding, p.y + b.position.y - padding});
    box.setSize({b.size.x + 2.f * padding, b.size.y + 2.f * padding});
    box.setFillColor(bgColor);
    box.setOutlineColor(outlineColor);
    box.setOutlineThickness(outlineThickness);

    target.draw(box);
    target.draw(text);
}
