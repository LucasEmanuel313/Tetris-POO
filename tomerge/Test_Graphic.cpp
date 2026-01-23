#include <SFML/Graphics.hpp>
#include <iostream>
#include "Button.h"
#include "Game.h"
//#include "Mouse.h"


#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200
#define GRID_COLS 10
#define GRID_ROWS 22
#define GRID_POS_X 300
#define GRID_POS_Y 50
#define BLOCK_SIZE 30.f

#include "GameState.h"

class WindowManager {
private:
    sf::RenderWindow window;
    GameState current_state;
    
public:
    WindowManager() : window(sf::VideoMode({1200, 800}), "Tetris") {
        window.setFramerateLimit(60);
        current_state = GameState::MENU;
    }
    
    void setState(GameState state) { current_state = state; }
    GameState getState() const { return current_state; }
    
    sf::RenderWindow& getWindow() { return window; }
    bool isOpen() { return window.isOpen(); }
};

int main()
{
    WindowManager windowManager;
    sf::RenderWindow& window = windowManager.getWindow();

    GameState current_state = GameState::MENU;

    table ta;
    ta.add_block();
    Game singleplayer(&ta, windowManager.getWindow());

    Mouse mouse;
    sf::Font font;
    if (!font.openFromFile("Tetris.ttf")) {
        std::cerr << "Error loading Tetris.ttf\n";
        return 1;
    }

    // Centralizar botões: largura 200, então x = (1200 - 200) / 2 = 500
    Button button(font, {200.f, 50.f}, "Single Player"  );
    Button button2(font, {200.f, 150.f}, "Multiplayer"  );
    Button button3(font, {200.f, 250.f}, "Exit"  );
    
    // Posicionar botões centralizados
    button.setPosition({500.f, 250.f});
    button2.setPosition({500.f, 350.f});
    button3.setPosition({500.f, 450.f});

   
    while (window.isOpen())
    {
        // Processar eventos
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        // Atualizar mouse
        mouse.Update(window);
        button.Update(mouse);
        button2.Update(mouse);
        button3.Update(mouse);

        // Processar input de botões
        if(button.getOnRelease()){
            current_state = GameState::SINGLEPLAYER;
            std::cout << "Single Player pressed\n";
        }
        if(button2.getOnRelease()){
            current_state = GameState::MULTIPLAYER;
            std::cout << "Multiplayer pressed\n";
        }
        if(button3.getOnRelease()){
            window.close();
        }

        // Limpar a tela
        window.clear(sf::Color::White);

        // Desenhar baseado no estado
        if(current_state == GameState::MENU){
            button.draw_button(window);
            button2.draw_button(window);
            button3.draw_button(window);
        }
        if(current_state == GameState::SINGLEPLAYER){
            singleplayer.HandleEvents();
            singleplayer.draw_game();
        }
        
        // Exibir frame
        window.display();
    }

    return 0;
}