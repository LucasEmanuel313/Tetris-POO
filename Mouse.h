#include <SFML/Graphics.hpp>

class Mouse{
    public:
    bool isPressed = false;
    bool onPress = false;
    bool onRelease = false;

    
    sf::Vector2f position;
    void Update(sf::RenderWindow &window){
        position = sf::Vector2f(sf::Mouse::getPosition(window));
        onPress = false;
        onRelease = false;
        if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
            if(!isPressed){
                onPress = true;
            } else {
                onPress = false;
            }
            isPressed = true;
        } else {
            if(isPressed){
                onRelease = true;
            } else {
                onRelease = false;
            }
            isPressed = false;
        }
    }
};