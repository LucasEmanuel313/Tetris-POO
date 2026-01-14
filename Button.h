#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>

class Button {
  sf::RectangleShape * button_shape;
  sf::Font * font;
  sf::Text button_text;

  private:
    void set_values();

  public:
    Button(sf::Text text);
    Button();
    ~Button();

};