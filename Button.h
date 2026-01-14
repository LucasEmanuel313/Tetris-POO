#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>

class Button {
  sf::RectangleShape * button_shape;
  sf::Font * font;
  std::optional<sf::Text> button_text;

  private:
    void set_values();

  public:
    Button(sf::Text text);
    Button();
    ~Button();

};