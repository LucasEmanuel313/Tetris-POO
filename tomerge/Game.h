#include <SFML/Graphics.hpp>
#include "table.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200
#define GRID_COLS 10
#define GRID_ROWS 22
#define BLOCK_SIZE 30.f
// Posições centralizadas dinamicamente
#define GRID_WIDTH (GRID_COLS * BLOCK_SIZE)  // 300
#define GRID_POS_X ((WINDOW_WIDTH - GRID_WIDTH) / 2.f)  // (1200 - 300) / 2 = 450
#define GRID_POS_Y 50
#define NEXT_BLOCK_POS_X (GRID_POS_X + GRID_WIDTH + 20.f)

class Game {
    // Game class implementation
    sf::RectangleShape grid[GRID_COLS][GRID_ROWS];
    sf::RenderWindow * window;
    sf::RectangleShape rectangle;
    const sf::Font* font;
    table* game_table;
    
    // Rastrear estado anterior das teclas
    bool prevKeyStates[5] = {false, false, false, false, false};
    // 0=Left, 1=Right, 2=Down, 3=Up, 4=Space
    
    private:

    sf::Color get_color_for_cell(char cell) {
        switch (cell) {
            case 'C': return sf::Color(0, 150, 150);     // Cyan Escuro
            case 'Y': return sf::Color(180, 180, 0);     // Amarelo Escuro
            case 'M': return sf::Color(150, 0, 150);     // Magenta Escuro
            case 'P': return sf::Color(0, 128, 0);       // Verde Escuro (Dark Green)
            case 'R': return sf::Color(150, 0, 0);       // Vermelho Escuro
            case 'B': return sf::Color(0, 0, 150);       // Azul Escuro
            case 'O': return sf::Color(180, 100, 0);     // Laranja Escuro
            case '*': return sf::Color(100, 100, 100);   // Ghost Block: Cinza
            default:  return sf::Color(40, 40, 40);      // Vazio: Cinza muito escuro (melhor que branco para o tema)
        }
    }

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

    void render_ghost_block(int ghost_y) {
        block* currentBlock = game_table->get_current_block();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (currentBlock->piece[i][j] != ' ') {
                    int x = game_table->get_block_x_pos() + i;
                    int y = ghost_y + j;
                    if (x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS) {
                        grid[x][y].setFillColor(get_color_for_cell('*')); // Cor do bloco fantasma
                        window->draw(grid[x][y]);
                    }
                }
            }
        }

    }

    void create_next_block_menu() {
        //Draws the next block menu rectangle
        sf::Color rectangleColor(150, 150, 150);
        rectangle.setPosition({NEXT_BLOCK_POS_X, GRID_POS_Y});
        rectangle.setOutlineThickness(2.f);
        rectangle.setOutlineColor(sf::Color(0, 0, 0));
        rectangle.setFillColor(rectangleColor);
        window->draw(rectangle);
    }

    
    public:
    void draw_grid() {
        
        for (int j = 0; j < GRID_ROWS; j++) {
            for (int i = 0; i < GRID_COLS; i++) {
                // 1. Acessa o conteúdo da célula na lógica
                char cell = game_table->get_cell(i, j); 

                // 2. Define a cor baseada no conteúdo
                if (cell != ' ') {
                    grid[i][j].setFillColor(get_color_for_cell(cell));
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
                window->draw(grid[i][j]);
            }
        }
        render_ghost_block(game_table->get_ghost_y());
    }
    void draw_next_block_menu() {
        window->draw(rectangle);
        block* nextBlock = game_table->get_next_block();
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                if(nextBlock->piece[i][j] != ' ') {
                    sf::RectangleShape blockShape;
                    blockShape.setSize({BLOCK_SIZE, BLOCK_SIZE});
                    // Define a cor baseada no tipo de peça
                    blockShape.setFillColor(get_color_for_cell(nextBlock->piece[i][j]));
                    blockShape.setOutlineThickness(2.f);
                    blockShape.setOutlineColor(sf::Color(0, 0, 0));
                    float x_pos = NEXT_BLOCK_POS_X + i * BLOCK_SIZE;
                    float y_pos = GRID_POS_Y + j * BLOCK_SIZE;
                    blockShape.setPosition({x_pos, y_pos});
                    window->draw(blockShape);
                }
            }
        }
    }

    //TESTE -----------------------------------------------------------------------------

    void HandleEvents() {
        // Detectar mudança de estado (transição de não-pressionado para pressionado)
        bool currentKeyStates[5] = {
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)
        };
        
        // Verificar transição: anterior=false, atual=true
        if (!prevKeyStates[0] && currentKeyStates[0]) {
            game_table->block_left();
        }
        if (!prevKeyStates[1] && currentKeyStates[1]) {
            game_table->block_right();
        }
        if (!prevKeyStates[2] && currentKeyStates[2]) {
            game_table->block_descend();
        }
        if (!prevKeyStates[3] && currentKeyStates[3]) {
            game_table->rotate_block();
        }
        if (!prevKeyStates[4] && currentKeyStates[4]) {
            game_table->block_drop();
        }
        
        // Atualizar estado anterior para próximo frame
        for (int i = 0; i < 5; ++i) {
            prevKeyStates[i] = currentKeyStates[i];
        }
    }

    void draw_score(int score) {
        sf::Text scoreText(*font);
        scoreText.setString("Score: " + std::to_string(score));
        scoreText.setPosition({NEXT_BLOCK_POS_X, GRID_POS_Y + 250.f});
        scoreText.setFillColor(sf::Color::Red);
        scoreText.setCharacterSize(20);
        window->draw(scoreText);
    }
    
    void draw_game() {
        draw_grid();
        create_next_block_menu();
        draw_next_block_menu();
        draw_score(game_table->get_score());
    }

    Game(table* t, sf::RenderWindow& win, const sf::Font& f) : window(&win), game_table(t), font(&f) {
        create_next_block_menu();
        window->setFramerateLimit(60);
        create_grid();
    }

    sf::RenderWindow& getWindow() {
        return *window;
    }
};