#include "Button.h"

#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200

Button::Button(sf::Text text){
  button_shape = new sf::RectangleShape();
  font = new sf::Font();
  button_text = std::move(text);
  set_values();
}

Button::Button(){
  button_shape = new sf::RectangleShape();
  font = new sf::Font();
  font->openFromFile("Tetris.ttf");
  button_text.emplace(*font, "Close", 20U);

  set_values();
}

Button::~Button(){
  delete button_shape;
  delete font;
}

void Button::set_values(){
    sf::Color rectangleColor(150, 150, 150);
    button_shape->setPosition({200.f, 50.f});
    button_shape->setOutlineThickness(2.f);
    button_shape->setOutlineColor(sf::Color(0, 0, 0));
    button_shape->setFillColor(rectangleColor);


    if (!button_text.has_value()) {
      button_text.emplace(*font, "Close", 20U);
    }

    button_text->setFont(*font);
    button_text->setPosition(button_shape->getPosition() + sf::Vector2f(10.f, 10.f));   
    button_text->setString("Close");
    button_text->setCharacterSize(20);
}



