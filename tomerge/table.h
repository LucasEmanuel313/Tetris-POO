//
// Created by lucas on 21/11/2025.
//
#ifndef TABLE_H
#define TABLE_H

#include "block.h"
#include <iostream>
#include <SFML/Graphics.hpp>

class table {

private:
    char positions[10][22];   // 10 colunas, 22 linhas (0..21)
    int  profile[10];
    block* current_block;
    block* next_block;
    int block_x_pos;          // posição (coluna) do canto inferior-esquerdo da peça 4x4
    int block_y_pos;          // posição (linha) do canto inferior-esquerdo da peça 4x4

    int score = 0;            // pontuação simples

    // coluna (0..3) mais à direita que contém bloco na peça atual
    int right_most_col_of_piece() const {
        int maxCol = 0;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] != ' ') {
                    if (i > maxCol) maxCol = i;
                }
            }
        }
        return maxCol;
    }

    // testa se a peça pode se mover de (block_x_pos, block_y_pos) para ( +dx, +dy )
    bool can_move(int dx, int dy) {
        // removemos temporariamente a peça do tabuleiro
        block_clear();

        bool ok = true;

        for (int i = 0; i < 4 && ok; ++i) {
            for (int j = 0; j < 4 && ok; ++j) {
                if (current_block->piece[i][j] != ' ') {
                    int newX = block_x_pos + dx + i;
                    int newY = block_y_pos + dy + j;

                    // parede / chão / topo
                    if (newX < 0 || newX >= 10 || newY < 0 || newY >= 22) {
                        ok = false;
                        break;
                    }

                    // colisão com blocos já existentes
                    if (positions[newX][newY] != ' ') {
                        ok = false;
                        break;
                    }
                }
            }
        }

        // redesenha a peça na posição original
        update_table();
        return ok;
    }

    // apaga uma linha e "desce" todas acima
    void clear_line(int line) {
        // desce tudo o que está acima da linha
        for (int j = line; j < 21; ++j) {
            for (int i = 0; i < 10; ++i) {
                positions[i][j] = positions[i][j + 1];
            }
        }
        // linha superior vira vazia
        for (int i = 0; i < 10; ++i) {
            positions[i][21] = ' ';
        }
    }

    // varre o tabuleiro, limpa linhas cheias e devolve quantas foram removidas
    int check_and_clear_lines() {
        int linesCleared = 0;

        for (int y = 0; y < 22; ++y) {
            bool full = true;
            for (int x = 0; x < 10; ++x) {
                if (positions[x][y] == ' ') {
                    full = false;
                    break;
                }
            }
            if (full) {
                clear_line(y);
                ++linesCleared;
                --y; // re-checa a mesma linha porque tudo desceu
            }
        }
        return linesCleared;
    }

    // trata o "pouso" da peça: limpa linhas, atualiza score e cria nova peça
    void handle_landing() {
        int lines = check_and_clear_lines();

        switch (lines) {
            case 1: score += 100; break;
            case 2: score += 300; break;
            case 3: score += 500; break;
            case 4: score += 800; break;
            default: break;
        }

        // a peça atual já está "fundida" no tabuleiro;
        // liberamos o objeto e criamos outra
        delete current_block;
        add_block();
    }


    bool can_simulate_move(int dx, int dy) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] != ' ') {
                    int newX = block_x_pos + dx + i;
                    int newY = block_y_pos + dy + j;

                    if (newX < 0 || newX >= 10 || newY < 0 || newY >= 22) return false;
                    if (positions[newX][newY] != ' ') return false;
                }
            }
        }
        return true;
    }
    
public:

    int get_ghost_y() {
        int ghost_y = block_y_pos;
        block_clear();

        // Simulamos a descida até encontrar uma colisão
        while (can_simulate_move(0, (ghost_y - block_y_pos) - 1)) {
            ghost_y--;
        }

        update_table();
        
        return ghost_y;
    }

    block* get_current_block() const {
        return current_block;
    }

    int get_block_x_pos() const {
        return block_x_pos;
    }
   // Retorna o que existe em uma posição específica para a classe gráfica ler
    char get_cell(int x, int y) const {
        if (x >= 0 && x < 10 && y >= 0 && y < 22) {
            return positions[x][y];
        }
        return ' ';
    }

    int get_score() const { return score; }

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

    void make_profile_table() {
        for (int i = 0; i < 10; i++) {
            profile[i] = 0;
            for (int j = 21; j >= 0 && profile[i] == 0; j--) {
                if (positions[i][j] != ' ') {
                    profile[i] = j;
                    break;
                }
            }
        }
        for (int i = 0; i < 10; i++) {
            std::cout << "Column " << i << " height: " << profile[i] << std::endl;
        }
        current_block->make_profile();
    }

    // desenha a peça atual no tabuleiro
    void update_table() {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] != ' ') {
                    positions[block_x_pos + i][block_y_pos + j] = current_block->piece[i][j];
                }
            }
        }
    }

    // remove a peça atual da posição em que está
    void block_clear() {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                // só limpa as células onde há bloco da peça
                if (current_block->piece[i][j] != ' ') {
                    positions[block_x_pos + i][block_y_pos + j] = ' ';
                }
            }
        }
    }

    // cria uma nova peça no topo
    void add_block() {
        current_block = next_block;
        next_block = new block();
        block_x_pos   = 3;
        block_y_pos   = 18; // 18..21 dentro da área 4x4
        // se já houver blocos lá, você pode adicionar lógica de "game over"
        update_table();
    }

    block* get_next_block() const {
        return next_block;
    }

    void rotate_block() {
        block_clear();
        current_block->rotate();
        update_table();
    }

    // queda automática / manual de uma linha
    void block_descend() {
        if (can_move(0, -1)) {
            block_clear();
            block_y_pos -= 1;
            update_table();
        } else {
            // não pode descer -> pousou
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
        // impede ultrapassar a parede direita
        int rightCol  = right_most_col_of_piece();
        int globalRight = block_x_pos + rightCol;
        if (globalRight >= 9) {
            return; // já encostou na parede
        }

        if (can_move(1, 0)) {
            block_clear();
            block_x_pos += 1;
            update_table();
        }
    }

    // queda até o chão
    void block_drop() {
        // desce enquanto puder
        while (can_move(0, -1)) {
            block_clear();
            block_y_pos -= 1;
            update_table();
        }
        // pousou
        handle_landing();
    }

    table* get_table_pointer() {
        return this;
    }

    table() {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 22; ++j) {
                positions[i][j] = ' ';
            }
        }
        current_block = new block();
        next_block = new block();
        block_x_pos = 0;
        block_y_pos = 0;
        score = 0;
    }

    ~table() {
        delete current_block;
    }
};

#endif //TABLE_H
