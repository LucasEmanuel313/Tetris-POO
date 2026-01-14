#include <SFML/Graphics.hpp>
#include "table.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200
#define GRID_COLS 10
#define GRID_ROWS 22
#define GRID_POS_X 300
#define GRID_POS_Y 50
#define BLOCK_SIZE 30.f

sf::Font font("Tetris.ttf");
sf::Text scoreText(font, "Score: 0", 24);
class Game {
    // Game class implementation
    sf::RectangleShape grid[GRID_COLS][GRID_ROWS];
    sf::RectangleShape rectangle;
    sf::RenderWindow window;
    
    table* game_table;
    private:

    void create_grid() {
        for (int i = 0; i < GRID_COLS; ++i)
        {
            for (int j = 0; j < GRID_ROWS; ++j)
            {
                grid[i][j].setSize({BLOCK_SIZE, BLOCK_SIZE});
                grid[i][j].setFillColor(sf::Color::White);
                grid[i][j].setOutlineThickness(2.f);
                grid[i][j].setOutlineColor(sf::Color(0, 0, 0));
                grid[i][j].setPosition({GRID_POS_X + i * BLOCK_SIZE, GRID_POS_Y + j * BLOCK_SIZE});
            }
        }
    }
    void create_next_block_menu() {
        sf::Color rectangleColor(100, 100, 100);
        rectangle.setSize({NEXT_BLOCK_MENU_WIDTH, NEXT_BLOCK_MENU_HEIGHT});
        rectangle.setPosition({GRID_POS_X + GRID_COLS * BLOCK_SIZE + 20.f, GRID_POS_Y});
        rectangle.setOutlineThickness(2.f);
        rectangle.setOutlineColor(sf::Color(0, 0, 0));
        rectangle.setFillColor(rectangleColor);
    }

    
    public:
    void draw_grid() {
    window.clear(sf::Color(100, 100, 100)); // Fundo cinza médio
    
    for (int j = 0; j < GRID_ROWS; j++) {
        for (int i = 0; i < GRID_COLS; i++) {
            // 1. Acessa o conteúdo da célula na lógica
            char cell = game_table->get_cell(i, j); 

            // 2. Define a cor baseada no conteúdo
            if (cell == '#') {
                grid[i][j].setFillColor(sf::Color::Blue);
            } else {
                grid[i][j].setFillColor(sf::Color::White);
            }

            // 3. CORREÇÃO DA POSIÇÃO (Inversão de Y)
            // SFML (0,0) é o topo. Se j=0 (chão), queremos desenhar no fundo da tela.
            // O cálculo (GRID_ROWS - 1 - j) inverte o eixo vertical.
            float x_pos = GRID_POS_X + i * BLOCK_SIZE;
            float y_pos = GRID_POS_Y + (GRID_ROWS - 1 - j) * BLOCK_SIZE;

            grid[i][j].setPosition({x_pos, y_pos});

            // 4. Desenha o quadrado na janela
            window.draw(grid[i][j]);
        }
    }
}
    void draw_next_block_menu() {
        window.draw(rectangle);
        block* nextBlock = game_table->get_next_block();
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                if(nextBlock->piece[i][j] == '#') {
                    sf::RectangleShape blockShape;
                    blockShape.setSize({BLOCK_SIZE, BLOCK_SIZE});
                    blockShape.setFillColor(sf::Color::Red);
                    blockShape.setOutlineThickness(2.f);
                    blockShape.setOutlineColor(sf::Color(0, 0, 0));
                    float x_pos = GRID_POS_X + GRID_COLS * BLOCK_SIZE + 20.f + i * BLOCK_SIZE;
                    float y_pos = GRID_POS_Y + j * BLOCK_SIZE;
                    blockShape.setPosition({x_pos, y_pos});
                    window.draw(blockShape);
                }
            }
        }
    }

    void draw_score(int score) {
        scoreText.setString("Score: " + std::to_string(score));
        scoreText.setPosition({GRID_POS_X + BLOCK_SIZE * 10 + 20.f, GRID_POS_Y + 250.f});
        scoreText.setFillColor(sf::Color::Black);
        window.draw(scoreText);
    }
    
    void draw_game() {
        draw_grid();
        draw_next_block_menu();
        draw_score(game_table->get_score());
    }

    void HandleEvents() {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Captura de teclas únicas (pressionou uma vez, executa uma vez)
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::Left:     game_table->block_left();    break;
                    case sf::Keyboard::Key::Right:     game_table->block_right();   break;
                    case sf::Keyboard::Key::Down:     game_table->block_descend(); break;
                    case sf::Keyboard::Key::Up:     game_table->rotate_block();  break;
                    case sf::Keyboard::Key::Space: game_table->block_drop();    break;
                    default: break;
                }
            }
        }
    }

    Game(table* t) : 
    game_table(t), 
    window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tetris SFML") // Inicializa direto
{
    window.setFramerateLimit(60);
    create_grid();
    create_next_block_menu();
}

    Game() {
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tetris SFML");
        window.setFramerateLimit(60); // Limita o FPS para não sobrecarregar a CPU
        create_grid();
        create_next_block_menu();
    }

    sf::RenderWindow& getWindow() {
        return window;
    }
};