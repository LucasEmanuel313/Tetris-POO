#include <SFML/Graphics.hpp>
#include <iostream>
#include "Button.h"
#include "GameState.h"


class MainMenu{
    
    Button button;
    Button button2;
    Button button3;

    // Posicionar botões centralizados
    

public:
    void drawMainMenu(sf::RenderWindow& window) {
        // Desenhar o menu principal
        button.draw_button(window);
        button2.draw_button(window);
        button3.draw_button(window);

    }

    void updateMainMenu(Mouse& mouse, GameState& current_state) {
        button.Update(mouse);
        button2.Update(mouse);
        button3.Update(mouse);

        if(button.getOnRelease()){
            current_state = GameState::SINGLEPLAYER;
            std::cout << "Single Player pressed\n";
        }
        if(button2.getOnRelease()){
            current_state = GameState::MULTIPLAYER;
            std::cout << "Multiplayer pressed\n";
        }
        if(button3.getOnRelease()){
            // Sair do jogo ou fechar a janela
            std::cout << "Exit pressed\n";
        }
    }

    MainMenu(sf::Font& font) : button(font, {200.f, 50.f}, "Single Player"),
                               button2(font, {200.f, 150.f}, "Multiplayer"),
                               button3(font, {200.f, 250.f}, "Exit") {
        // Inicializar botões e outras configurações do menu
        button.setPosition({500.f, 250.f});
        button2.setPosition({500.f, 350.f});
        button3.setPosition({500.f, 450.f});
    }

};
