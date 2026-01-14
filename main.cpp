#include <iostream>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "table.h"
#include "Game.h"


int main() {
    using clock = std::chrono::steady_clock;

    // 1. Inicialização da Janela e Lógica do Jogo
    
    table ta;
    ta.add_block();

    Game game(&ta);

    // 2. Configuração do Grid Gráfico (do Test_Graphic)
    sf::RenderWindow& window = game.getWindow();
    auto lastFall = clock::now();
    std::chrono::milliseconds fallInterval(500);

    // 3. Game Loop Principal
    while (window.isOpen()) {
        
        // --- A) PROCESSAMENTO DE EVENTOS ---
        game.HandleEvents();

        // --- B) LÓGICA DE TEMPO (QUEDA AUTOMÁTICA) ---
        auto now = clock::now();
        if (now - lastFall >= fallInterval) {
            ta.block_descend();
            lastFall = now;
        }

        // --- C) RENDERIZAÇÃO ---
        game.draw_game();

        // Desenha os blocos do jogo 
        // Nota: Certifique-se que sua classe 'table' desenha algo na janela via ta.setGameWindow
        //ta.print_table(); 

        window.display();
    }

    return 0;
}