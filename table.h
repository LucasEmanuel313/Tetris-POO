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
        int lines = check_and_clear_lines();
        lastClearedEvent += lines;

        switch (lines) {
            case 1: score += 100; break;
            case 2: score += 300; break;
            case 3: score += 500; break;
            case 4: score += 800; break;
            default: break;
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

    

public:
    table() {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 22; ++j) {
                positions[i][j] = ' ';
            }
        }
        score = 0;
        lastClearedEvent = 0;
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

    

    void update_table() {
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
        update_table();
    }

    void rotate_block() {
        block_clear();
        current_block->rotate();
        update_table();
    }

    void block_descend() {
        if (can_move(0, -1)) {
            block_clear();
            block_y_pos -= 1;
            update_table();
        } else {
            handle_landing();
        }
    }

    void block_left() {
        if (can_move(-1, 0)) {
            block_clear();
            block_x_pos -= 1;
            update_table();
        }
    }

    void block_right() {
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
    }

    // --- Multiplayer hook: quantas linhas limpei desde a última leitura ---
    int pop_cleared_lines_event() {
        int v = lastClearedEvent;
        lastClearedEvent = 0;
        return v;
    }
};

#endif // TABLE_H
