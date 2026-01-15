#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>
#include "Mouse.h"

class Button {
  bool isOver = false;
  bool isPressed = false;
  bool isPressedInside = false;
  bool onPress = false;
  bool onRelease = false;

  

  sf::Color defaultColor = sf::Color(150, 150, 150);
  sf::Color hoverColor = sf::Color(200, 200, 200);
  sf::Color pressedColor = sf::Color(100, 100, 100);

  sf::Color defaultTextColor = sf::Color::Black;
  sf::Color hoverTextColor = sf::Color::Black;
  sf::Color pressedTextColor = sf::Color::White;

  sf::RectangleShape  button_shape;
  sf::Text button_text;
  //sf::Text  button_text;

  private:
    void set_values(){
 
        button_shape.setSize({200.f, 70.f});
        button_shape.setOutlineThickness(2.f);
        button_shape.setOutlineColor(sf::Color(0, 0, 0));


        button_text.setCharacterSize(20);
        button_text.setFillColor(sf::Color::Black);

        if(isPressed){
          button_shape.setFillColor(pressedColor);
          button_text.setFillColor(pressedTextColor);
        } else if(isOver){
          button_shape.setFillColor(hoverColor);
          button_text.setFillColor(hoverTextColor);
        } else {
          button_shape.setFillColor(defaultColor);
          button_text.setFillColor(defaultTextColor);
        }
    };

  public:
    Button(sf::Font font, sf::Vector2f position, std::string text) : button_text(font) {
        button_shape.setPosition(position);
        button_text.setPosition({position.x + 30.f, position.y + 10.f});
        button_text.setString(text);
    };
    void draw_button(sf::RenderWindow &window){
      set_values();
      window.draw(button_shape);
      window.draw(button_text);
    };
    void Update(Mouse& mouse){
      if(mouse.onPress == true){
        std::cout << "Button: " << mouse.position.x << ", " << mouse.position.y << std::endl;
      }

      if(button_shape.getGlobalBounds().contains(mouse.position)){
        isOver = true;
        if(mouse.isPressed){
          if(!isPressed){
            onPress = true;
          } else {
            onPress = false;
          }
          isPressed = true;
          isPressedInside = true;
        } else {
          if(isPressed && isPressedInside){
            onRelease = true;
          } else {
            onRelease = false;
          }
          isPressed = false;
          isPressedInside = false;
        }
      } else {
        isOver = false;
        if(isPressed && isPressedInside){
          onRelease = true;
        } else {
          onRelease = false;
        }
        isPressed = false;
        isPressedInside = false;
      }
    }
    void setPosition(sf::Vector2f position){
      button_shape.setPosition(position);
      button_text.setPosition({position.x + 30.f, position.y + 10.f});
    }
    bool getOnRelease(){
      return onRelease;
    }
    ~Button() {
      
    }
    

};