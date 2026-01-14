#ifndef TABLE_H
#define TABLE_H

#include "block.h"
#include <iostream>
#include <cstdlib> // rand()

class table {

private:
    char positions[10][22];
    int  profile[10];
    block* current_block = nullptr;

    int block_x_pos = 0;
    int block_y_pos = 0;

    int score = 0;

    // evento: linhas limpas desde a última leitura (para mandar ao servidor)
    int lastClearedEvent = 0;

    // evento: peça travou (landing) desde a última leitura
    int lastLandedEvent = 0;

    int right_most_col_of_piece() const {
        int maxCol = 0;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] == '#') {
                    if (i > maxCol) maxCol = i;
                }
            }
        }
        return maxCol;
    }

    void clear_line(int line) {
        for (int j = line; j < 21; ++j) {
            for (int i = 0; i < 10; ++i) {
                positions[i][j] = positions[i][j + 1];
            }
        }
        for (int i = 0; i < 10; ++i) {
            positions[i][21] = ' ';
        }
    }

    int check_and_clear_lines() {
        int linesCleared = 0;

        for (int y = 0; y < 22; ++y) {
            bool full = true;
            for (int x = 0; x < 10; ++x) {
                if (positions[x][y] != '#') {
                    full = false;
                    break;
                }
            }
            if (full) {
                clear_line(y);
                ++linesCleared;
                --y;
            }
        }
        return linesCleared;
    }

    void handle_landing() {
        // landing ocorreu (a peça não conseguiu descer mais)
        lastLandedEvent += 1;

        int lines = check_and_clear_lines();
        lastClearedEvent += lines;

        switch (lines) {
            case 1: score += 100; break;
            case 2: score += 300; break;
            case 3: score += 500; break;
            case 4: score += 800; break;
            default: break;
        }

        if (top_reached()) {
            gameOver = true;
            return;
        }

        delete current_block;
        current_block = nullptr;
        add_block();
    }
    bool gameOver = false;

    bool collides_here() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] == '#') {
                    int x = block_x_pos + i;
                    int y = block_y_pos + j;

                    // se sair dos limites, é game over também (spawn inválido)
                    if (x < 0 || x >= 10 || y < 0 || y >= 22) return true;

                    // colisão com bloco já fixado
                    if (positions[x][y] == '#') return true;
                }
            }
        }
        return false;
    }

    bool top_reached() const {
        for (int x = 0; x < 10; ++x) {
            if (positions[x][21] == '#') return true;
        }
        return false;
    }

    

public:
    table() {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 22; ++j) {
                positions[i][j] = ' ';
            }
        }
        score = 0;
        lastClearedEvent = 0;
        lastLandedEvent = 0;
    }

    ~table() {
        delete current_block;
    }

    void print_table() {
        for (int i = 21; i >= 0; --i) {
            for (int j = 0; j < 10; ++j) {
                std::cout << '|' << positions[j][i] << '|';
            }
            std::cout << '\n';
        }
        std::cout << "-------------------------------\n";
        std::cout << "Score: " << score << "\n";
    }

    int get_score() const { return score; }

    // Multiplayer: serializa o tabuleiro (inclui a peça atual, pois ela está em positions)
    // Formato: 22 linhas (y=0..21) * 10 colunas (x=0..9), '.' vazio, '#' preenchido
    std::string serialize_board() const {
        std::string out;
        out.reserve(10 * 22);
        for (int y = 0; y < 22; ++y) {
            for (int x = 0; x < 10; ++x) {
                out.push_back(positions[x][y] == '#' ? '#' : '.');
            }
        }
        return out;
    }

    

    void update_table() {
        if (!current_block) return;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] == '#') {
                    int x = block_x_pos + i;
                    int y = block_y_pos + j;
                    if (x >= 0 && x < 10 && y >= 0 && y < 22) {
                        positions[x][y] = '#';
                    }
                }
            }
        }
    }

    void block_clear() {
        if (!current_block) return;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] == '#') {
                    int x = block_x_pos + i;
                    int y = block_y_pos + j;
                    if (x >= 0 && x < 10 && y >= 0 && y < 22) {
                        positions[x][y] = ' ';
                    }
                }
            }
        }
    }

    bool can_move(int dx, int dy) {
        if (!current_block) return false;
        block_clear();

        bool ok = true;
        for (int i = 0; i < 4 && ok; ++i) {
            for (int j = 0; j < 4 && ok; ++j) {
                if (current_block->piece[i][j] == '#') {
                    int newX = block_x_pos + dx + i;
                    int newY = block_y_pos + dy + j;

                    if (newX < 0 || newX >= 10 || newY < 0 || newY >= 22) {
                        ok = false;
                        break;
                    }
                    if (positions[newX][newY] == '#') {
                        ok = false;
                        break;
                    }
                }
            }
        }

        update_table();
        return ok;
    }

    void add_block() {
        current_block = new block();
        block_x_pos = 3;
        block_y_pos = 18;

        // se já nasce colidindo com o topo/stack, game over imediato
        if (collides_here()) {
            gameOver = true;
            // ainda desenha a peça que tentou nascer (pra não "sumir")
            update_table();
            return;
        }
        update_table();
    }

    // table.h  (substituir rotate_block das linhas 193–197)
    void rotate_block() {
        if (!current_block || gameOver) return;

        // remove a peça atual do grid
        block_clear();

        // backup da peça e posição
        char backup[4][4];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                backup[i][j] = current_block->piece[i][j];

        int oldX = block_x_pos;
        int oldY = block_y_pos;

        // rotaciona
        current_block->rotate();

        // tenta "kicks" (0, empurra pra esquerda, depois direita)
        const int kicks[] = { 0, -1, -2, -3, 1, 2, 3 };
        bool ok = false;

        for (int k = 0; k < (int)(sizeof(kicks)/sizeof(kicks[0])); ++k) {
            block_x_pos = oldX + kicks[k];
            block_y_pos = oldY;

            if (!collides_here()) { // agora só checa colisão/borda
                ok = true;
                break;
            }
        }

        if (!ok) {
            // desfaz rotação + posição
            block_x_pos = oldX;
            block_y_pos = oldY;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    current_block->piece[i][j] = backup[i][j];
        }

        // redesenha a peça (válida ou revertida)
        update_table();
    }


    void block_descend() {
        if (!current_block || gameOver) return;
        if (can_move(0, -1)) {
            block_clear();
            block_y_pos -= 1;
            update_table();
        } else {
            handle_landing();
        }
    }

    void block_left() {
        if (!current_block || gameOver) return;
        if (can_move(-1, 0)) {
            block_clear();
            block_x_pos -= 1;
            update_table();
        }
    }

    void block_right() {
        if (!current_block || gameOver) return;
        int rightCol = right_most_col_of_piece();
        int globalRight = block_x_pos + rightCol;
        if (globalRight >= 9) return;

        if (can_move(1, 0)) {
            block_clear();
            block_x_pos += 1;
            update_table();
        }
    }

    void block_drop() {
        if (!current_block || gameOver) return;
        while (can_move(0, -1)) {
            block_clear();
            block_y_pos -= 1;
            update_table();
        }
        handle_landing();
    }

    bool is_game_over() const { return gameOver; }


    // --- Multiplayer hook: recebe lixo do servidor ---
    void apply_garbage(int n) {
        if (!current_block) return;

        block_clear();

        for (int k = 0; k < n; ++k) {
            int hole = rand() % 10;

            // shift up: y = 21 <- 20 <- ... <- 0
            for (int y = 21; y > 0; --y) {
                for (int x = 0; x < 10; ++x) {
                    positions[x][y] = positions[x][y - 1];
                }
            }

            // linha lixo em y=0
            for (int x = 0; x < 10; ++x) {
                positions[x][0] = (x == hole) ? ' ' : '#';
            }
        }

        update_table();

        if (top_reached()) gameOver = true;
    }

    // --- Multiplayer hook: quantas linhas limpei desde a última leitura ---
    int pop_cleared_lines_event() {
        int v = lastClearedEvent;
        lastClearedEvent = 0;
        return v;
    }

    int pop_landed_event() {
        int v = lastLandedEvent;
        lastLandedEvent = 0;
        return v;
    }

    // Reseta o estado do tabuleiro. Se spawn=true, já nasce uma peça.
    void reset(bool spawn = true) {
        delete current_block;
        current_block = nullptr;

        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 22; ++j) {
                positions[i][j] = ' ';
            }
        }

        block_x_pos = 0;
        block_y_pos = 0;
        score = 0;
        lastClearedEvent = 0;
        lastLandedEvent = 0;
        gameOver = false;

        if (spawn) add_block();
    }
};

#endif // TABLE_H
