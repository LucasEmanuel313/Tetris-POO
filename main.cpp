#include <iostream>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "block.h"
#include "table.h"
#include "Game.h"
#include "Button.h"
#include "WindowManager.cpp"
#include "MainMenu.cpp"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200
#define GRID_COLS 10
#define GRID_ROWS 22
//#define GRID_POS_X 300
#define GRID_POS_Y 50
#define BLOCK_SIZE 30.f



// Função para centralizar botão horizontalmente
inline float getCenterX(float screenWidth, float buttonWidth) {
    return (screenWidth - buttonWidth) / 2.f;
}




int main() {
    

    sf::Texture texture("Images/Background_Tetris.jpg");
    sf::Sprite background(texture);

    using clock = std::chrono::steady_clock;

    // 1. Inicialização da Janela e Lógica do Jogo
    
    WindowManager windowManager;
    sf::RenderWindow& window = windowManager.getWindow();

    GameState current_state = GameState::MENU;

    table ta;
    ta.add_block();
    


    Mouse mouse;
    sf::Font font;
    if (!font.openFromFile("Tetris.ttf")) {
        std::cerr << "Error loading Tetris.ttf\n";
        return 1;
    }

    Game singleplayer(&ta, windowManager.getWindow(), font);
    MainMenu mainMenu(font);


    // 2. Configuração do Grid Gráfico (do Test_Graphic)
    auto lastFall = clock::now();
    std::chrono::milliseconds fallInterval(500);

    // 3. Game Loop Principal
    while (window.isOpen()) {
        
        // --- A) PROCESSAMENTO DE EVENTOS ---
         while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        

        // --- B) LÓGICA DE TEMPO (QUEDA AUTOMÁTICA) ---
        auto now = clock::now();
        if(current_state == GameState::SINGLEPLAYER){
            if (now - lastFall >= fallInterval) {
                ta.block_descend();
                lastFall = now;
            }
        }
        mouse.Update(window);
        mainMenu.updateMainMenu(mouse, current_state);
        window.clear(sf::Color::White);
        window.draw(background);
        // Desenhar baseado no estado
        if(current_state == GameState::MENU){
            mainMenu.drawMainMenu(window);
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