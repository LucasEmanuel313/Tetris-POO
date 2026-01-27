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
    static constexpr int kCols = 10;
    static constexpr int kRows = 22;              // linhas visíveis (0..21)
    static constexpr int kSpawnBufferRows = 4;    // "linhas ocultas" para spawn acima do topo
    static constexpr int kMaxActiveY = kRows + kSpawnBufferRows - 1; // 25
    static constexpr int kSpawnX = 3;
    static constexpr int kSpawnY = kRows;         // nasce acima do topo visível

    char positions[kCols][kRows];
    int  profile[kCols];
    block* current_block = nullptr;

    // Para UI (SFML): tipo do bloco FIXO em cada célula.
    // -1 = vazio; 0..6 = tetromino; 7 = garbage.
    static constexpr unsigned char kEmptyType = 255;
    static constexpr unsigned char kGarbageType = 7;
    unsigned char fixedType[kCols][kRows];

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

    int right_most_col_of_piece() const;
    void clear_line(int line);
    int check_and_clear_lines();
    void handle_landing();
    bool gameOver = false;

    bool collides_here() const;
    bool top_reached() const;
    void update_table_no_overwrite();
    void refill_bag_if_needed();
    int draw_from_bag();
    void ensure_next_queue();
    int pop_next_type();
    static std::string mini_line_for_type(int type, int miniRowFromTop);
    std::string side_panel_line_single(int rowIndexFromTop) const;

    

public:
    table();
    ~table();

    void print_table();

    int get_score() const;

    // Tipo do bloco fixo para UI: -1 vazio, 0..6 tetromino, 7 garbage
    int get_fixed_type(int x, int y) const;

    // --- Helpers para renderização (SFML) ---
    const block* get_current_block() const;
    int get_block_x_pos() const;
    int get_block_y_pos() const;

    // Retorna apenas o tabuleiro FIXO (sem a peça atual), para o renderer desenhar a peça separadamente.
    char get_fixed_cell(int x, int y) const;

    // Calcula onde a peça cairia (ghost) sem alterar o estado do jogo.
    int get_ghost_y() const;

    int get_hold_type() const;
    std::vector<int> get_next_types(int count) const;

    // Multiplayer: serializa o tabuleiro (inclui a peça atual, pois ela está em positions)
    // Formato: 22 linhas (y=0..21) * 10 colunas (x=0..9), '.' vazio, '#' preenchido
    std::string serialize_board() const;

    void update_table();
    void block_clear();
    bool can_move(int dx, int dy);

    void add_block();
    void spawn_if_needed();
    void hold_block();

    void rotate_block();
    void block_descend();
    void block_left();
    void block_right();
    void block_drop();

    bool is_game_over() const;

    // --- Multiplayer hook: recebe lixo do servidor ---
    void apply_garbage(int n);

    // --- Multiplayer hook: quantas linhas limpei desde a última leitura ---
    int pop_cleared_lines_event();
    int pop_landed_event();

    // Reseta o estado do tabuleiro. Se spawn=true, já nasce uma peça.
    void reset(bool spawn = true);
};

#endif // TABLE_H
