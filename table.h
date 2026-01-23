#ifndef TABLE_H
#define TABLE_H

#include "block.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <deque>
#include <random>
#include <string>
#include <vector>

class table {

private:
    char positions[10][22];
    int  profile[10];
    block* current_block = nullptr;

    int block_x_pos = 0;
    int block_y_pos = 0;

    int score = 0;

    // Next/Hold (estilo Tetris)
    static constexpr int kNextPreviewCount = 3;
    std::mt19937 rng;
    std::vector<int> bag;
    std::deque<int> nextQueue;
    int holdType = -1;          // -1 = vazio
    bool holdUsedThisTurn = false; // só pode segurar 1x por peça

    // Quando uma peça trava, deixamos a próxima nascer depois (p/ aplicar garbage entre peças)
    bool needsSpawn = false;

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
        if (lines < 0) lines = 0;
        if (lines > 4) lines = 4; // por jogada, o máximo em Tetris é 4
        lastClearedEvent += lines;

        switch (lines) {
            case 1: score += 40; break;
            case 2: score += 100; break;
            case 3: score += 300; break;
            case 4: score += 1200; break;
            default: break;
        }

        if (top_reached()) {
            gameOver = true;
            return;
        }

        // Nova peça
        holdUsedThisTurn = false;
        delete current_block;
        current_block = nullptr;
        needsSpawn = true;
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

    void update_table_no_overwrite() {
        if (!current_block) return;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (current_block->piece[i][j] == '#') {
                    int x = block_x_pos + i;
                    int y = block_y_pos + j;
                    if (x >= 0 && x < 10 && y >= 0 && y < 22) {
                        if (positions[x][y] == ' ') positions[x][y] = '#';
                    }
                }
            }
        }
    }

    void refill_bag_if_needed() {
        if (!bag.empty()) return;
        bag.clear();
        bag.reserve(7);
        for (int i = 0; i < 7; ++i) bag.push_back(i);
        std::shuffle(bag.begin(), bag.end(), rng);
    }

    int draw_from_bag() {
        refill_bag_if_needed();
        int t = bag.back();
        bag.pop_back();
        return t;
    }

    void ensure_next_queue() {
        while ((int)nextQueue.size() < kNextPreviewCount) {
            nextQueue.push_back(draw_from_bag());
        }
    }

    int pop_next_type() {
        ensure_next_queue();
        int t = nextQueue.front();
        nextQueue.pop_front();
        ensure_next_queue();
        return t;
    }

    static std::string mini_line_for_type(int type, int miniRowFromTop) {
        // miniRowFromTop: 0..3 (0 = topo)
        if (type < 0 || type > 6) return "        ";
        const char (*p)[4][4] = TETROMINOES[type];
        std::string out;
        out.reserve(8);
        int y = 3 - miniRowFromTop; // converte para o sistema usado no piece (y cresce pra cima)
        for (int x = 0; x < 4; ++x) {
            out += ((*p)[x][y] == '#') ? "[]" : "  ";
        }
        return out;
    }

    std::string side_panel_line_single(int rowIndexFromTop) const {
        // rowIndexFromTop: 0..21 (0 = topo da tela)
        // Layout (22 linhas): HOLD(1+4) + blank(1) + NEXT(1 + 3*4 + 2 blanks) = 22
        if (rowIndexFromTop == 0) return "HOLD";
        if (rowIndexFromTop >= 1 && rowIndexFromTop <= 4) {
            if (holdType < 0) return (rowIndexFromTop == 1) ? "(none)" : "";
            return mini_line_for_type(holdType, rowIndexFromTop - 1);
        }
        if (rowIndexFromTop == 5) return "";
        if (rowIndexFromTop == 6) return "NEXT";

        int base = 7;
        for (int n = 0; n < kNextPreviewCount; ++n) {
            int start = base + n * 5; // 4 linhas + 1 blank entre peças
            int end = start + 3;
            if (rowIndexFromTop >= start && rowIndexFromTop <= end) {
                int t = (n < (int)nextQueue.size()) ? nextQueue[n] : -1;
                return mini_line_for_type(t, rowIndexFromTop - start);
            }
            if (rowIndexFromTop == start + 4) return "";
        }
        return "";
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

        std::random_device rd;
        rng.seed(rd());
        bag.clear();
        nextQueue.clear();
        ensure_next_queue();
    }

    ~table() {
        delete current_block;
    }

    void print_table() {
        for (int i = 21; i >= 0; --i) {
            for (int j = 0; j < 10; ++j) {
                std::cout << '|' << positions[j][i] << '|';
            }
            int rowIndexFromTop = 21 - i;
            std::string panel = side_panel_line_single(rowIndexFromTop);
            if (!panel.empty()) {
                std::cout << "   " << panel;
            }
            std::cout << '\n';
        }
        std::cout << "-------------------------------\n";
        std::cout << "Score: " << score << "   C=HOLD";
        std::cout << std::string(30, ' ') << "\n";
    }

    int get_score() const { return score; }

    // --- Helpers para renderização (SFML) ---
    const block* get_current_block() const { return current_block; }
    int get_block_x_pos() const { return block_x_pos; }
    int get_block_y_pos() const { return block_y_pos; }

    // Retorna apenas o tabuleiro FIXO (sem a peça atual), para o renderer desenhar a peça separadamente.
    char get_fixed_cell(int x, int y) const {
        if (x < 0 || x >= 10 || y < 0 || y >= 22) return ' ';
        if (!current_block) return positions[x][y];

        // Se esta célula faz parte da peça atual (na posição atual), então não é "fixa".
        int lx = x - block_x_pos;
        int ly = y - block_y_pos;
        if (lx >= 0 && lx < 4 && ly >= 0 && ly < 4) {
            if (current_block->piece[lx][ly] == '#') return ' ';
        }
        return positions[x][y];
    }

    // Calcula onde a peça cairia (ghost) sem alterar o estado do jogo.
    int get_ghost_y() const {
        if (!current_block) return block_y_pos;

        auto occupied_fixed = [&](int x, int y) -> bool {
            if (x < 0 || x >= 10 || y < 0 || y >= 22) return true;
            if (positions[x][y] != '#') return false;

            // Ignora a peça atual desenhada em positions[] na posição atual.
            int lx = x - block_x_pos;
            int ly = y - block_y_pos;
            if (lx >= 0 && lx < 4 && ly >= 0 && ly < 4) {
                if (current_block->piece[lx][ly] == '#') return false;
            }
            return true;
        };

        int ghostY = block_y_pos;
        while (true) {
            // tenta descer 1
            bool can = true;
            for (int i = 0; i < 4 && can; ++i) {
                for (int j = 0; j < 4 && can; ++j) {
                    if (current_block->piece[i][j] == '#') {
                        int x = block_x_pos + i;
                        int yNext = (ghostY - 1) + j;
                        if (yNext < 0) {
                            can = false;
                            break;
                        }
                        if (occupied_fixed(x, yNext)) {
                            can = false;
                            break;
                        }
                    }
                }
            }

            if (!can) break;
            ghostY -= 1;
        }

        return ghostY;
    }

    int get_hold_type() const { return holdType; }
    std::vector<int> get_next_types(int count) const {
        if (count < 0) count = 0;
        if (count > kNextPreviewCount) count = kNextPreviewCount;
        std::vector<int> out;
        out.reserve((size_t)count);
        for (int i = 0; i < count && i < (int)nextQueue.size(); ++i) out.push_back(nextQueue[i]);
        return out;
    }

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
        int type = pop_next_type();
        current_block = new block(type);
        block_x_pos = 3;
        block_y_pos = 18;
        needsSpawn = false;

        // se já nasce colidindo com o topo/stack, game over imediato
        if (collides_here()) {
            gameOver = true;
            // ainda desenha a peça que tentou nascer (pra não "sumir")
            update_table_no_overwrite();
            return;
        }
        update_table();
    }

    // Chame depois de aplicar garbage (ou a cada frame) para garantir que a próxima peça nasça.
    void spawn_if_needed() {
        if (gameOver) return;
        if (!needsSpawn) return;
        if (current_block != nullptr) return;
        add_block();
    }

    // Hold (troca de peça) - regra: só 1 hold por peça (até ela travar)
    void hold_block() {
        if (!current_block || gameOver) return;
        if (holdUsedThisTurn) return;

        int curType = current_block->type();

        block_clear();
        delete current_block;
        current_block = nullptr;

        if (holdType < 0) {
            holdType = curType;
            add_block();
        } else {
            int swapType = holdType;
            holdType = curType;
            current_block = new block(swapType);
            block_x_pos = 3;
            block_y_pos = 18;

            if (collides_here()) {
                gameOver = true;
                update_table_no_overwrite();
            } else {
                update_table();
            }
        }

        holdUsedThisTurn = true;
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
        if (n <= 0) return;

        bool hadActive = (current_block != nullptr);
        if (hadActive) block_clear();

        for (int k = 0; k < n; ++k) {
            // Se já existe algo no topo, subir mais significa estourar o teto.
            if (top_reached()) {
                gameOver = true;
                return;
            }

            std::uniform_int_distribution<int> dist(0, 9);
            int hole = dist(rng);

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

        if (hadActive) {
            // checa colisão antes de redesenhar a peça
            // (se redesenhar primeiro, vai colidir com ela mesma)
            bool overlap = collides_here();
            if (overlap) {
                gameOver = true;
                update_table_no_overwrite();
            } else {
                update_table();
            }
        }

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

        holdType = -1;
        holdUsedThisTurn = false;
        needsSpawn = false;
        bag.clear();
        nextQueue.clear();
        ensure_next_queue();

        if (spawn) add_block();
    }
};

#endif // TABLE_H
