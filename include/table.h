#ifndef TABLE_H
#define TABLE_H

#include "block.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <random>
#include <string>
#include <vector>

class table {

private:
    static constexpr int kCols = 10;
    static constexpr int kRows = 22;              // visible rows (0..21)
    static constexpr int kSpawnBufferRows = 4;    // hidden spawn buffer above the visible top
    static constexpr int kMaxActiveY = kRows + kSpawnBufferRows - 1; // 25
    static constexpr int kSpawnX = 3;
    static constexpr int kSpawnY = kRows;         // spawns above the visible top

    char positions[kCols][kRows];
    int  profile[kCols];
    std::unique_ptr<block> current_block;

    // For UI (SFML): fixed cell type in each grid position.
    // -1 = empty; 0..6 = tetromino; 7 = garbage.
    static constexpr unsigned char kEmptyType = 255;
    static constexpr unsigned char kGarbageType = 7;
    unsigned char fixedType[kCols][kRows];

    int block_x_pos = 0;
    int block_y_pos = 0;

    int score = 0;

    // Next/Hold (modern Tetris style)
    static constexpr int kNextPreviewCount = 3;
    std::mt19937 rng;
    std::vector<int> bag;
    std::deque<int> nextQueue;
    int holdType = -1;          // -1 = empty
    bool holdUsedThisTurn = false; // can only hold once per active piece

    // When a piece locks, delay spawning the next one (so we can apply incoming garbage between pieces).
    bool needsSpawn = false;

    // Event: cleared lines since last read (used by multiplayer/server).
    int lastClearedEvent = 0;

    // Event: piece locked (landing) since last read.
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

    // Fixed cell type for UI: -1 empty, 0..6 tetromino, 7 garbage.
    int get_fixed_type(int x, int y) const;

    // --- Rendering helpers (SFML) ---
    const block* get_current_block() const;
    int get_block_x_pos() const;
    int get_block_y_pos() const;

    // Returns only the fixed board (without the active piece), so the renderer can draw the piece separately.
    char get_fixed_cell(int x, int y) const;

    // Computes the ghost Y position without mutating game state.
    int get_ghost_y() const;

    int get_hold_type() const;
    std::vector<int> get_next_types(int count) const;

    // Multiplayer: serializes the board. Includes the active piece because it is drawn into positions[].
    // Format: 22 rows (y=0..21) * 10 cols (x=0..9), '.' empty, '0'..'6' filled type.
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

    // --- Multiplayer hook: receives garbage from the server ---
    void apply_garbage(int n);

    // --- Multiplayer hook: how many lines were cleared since last read ---
    int pop_cleared_lines_event();
    int pop_landed_event();

    // Resets the board state. If spawn=true, spawns a piece immediately.
    void reset(bool spawn = true);
};

#endif // TABLE_H
