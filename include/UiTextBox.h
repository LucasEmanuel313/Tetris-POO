#pragma once

#include <SFML/Graphics.hpp>

inline void wrapTextToWidth(sf::Text& text, float maxWidth) {
    const sf::String original = text.getString();
    if (original.isEmpty()) return;

    sf::String result;
    sf::String currentLine;
    sf::String currentWord;

    auto flushWord = [&]() {
        if (currentWord.isEmpty()) return;

        sf::String candidate = currentLine;
        if (!candidate.isEmpty()) candidate += ' ';
        candidate += currentWord;

        text.setString(candidate);
        const float w = text.getLocalBounds().size.x;
        if (!currentLine.isEmpty() && w > maxWidth) {
            if (!result.isEmpty()) result += '\n';
            result += currentLine;
            currentLine = currentWord;
        } else {
            currentLine = candidate;
        }

        currentWord.clear();
    };

    for (std::size_t i = 0; i < original.getSize(); ++i) {
        const auto ch = original[i];
        if (ch == '\n') {
            flushWord();
            if (!result.isEmpty()) result += '\n';
            result += currentLine;
            currentLine.clear();
            continue;
        }
        if (ch == ' ' || ch == '\t') {
            flushWord();
            continue;
        }
        currentWord += ch;
    }

    flushWord();
    if (!currentLine.isEmpty()) {
        if (!result.isEmpty()) result += '\n';
        result += currentLine;
    }

    text.setString(result);
}

inline void drawTextWithBox(sf::RenderTarget& target,
                           sf::Text& text,
                           float padding = 6.f,
                           sf::Color bgColor = sf::Color(255, 255, 255, 220),
                           sf::Color outlineColor = sf::Color::Black,
                           float outlineThickness = 2.f) {
    const sf::FloatRect b = text.getGlobalBounds();

    sf::RectangleShape box;
    box.setPosition({b.position.x - padding, b.position.y - padding});
    box.setSize({b.size.x + 2.f * padding, b.size.y + 2.f * padding});
    box.setFillColor(bgColor);
    box.setOutlineColor(outlineColor);
    box.setOutlineThickness(outlineThickness);

    target.draw(box);
    target.draw(text);
}
