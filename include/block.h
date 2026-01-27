#include <cstddef>
#include <cstdlib> // rand(), srand()
#include <ctime>   // time()
#include <iostream>
#include <memory>
#include <stdexcept>


#ifndef BLOCK_H
#define BLOCK_H

inline constexpr char PIECE_I[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', '#'}, // Four blocks in a line
    {' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' '}
};

// J piece
inline constexpr char PIECE_J[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', ' ', '#', ' '}, // Corner on the top-right
    {' ', ' ', ' ', ' '}
};

// L piece
inline constexpr char PIECE_L[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {'#', ' ', ' ', ' '}, // Corner on the top-left
    {' ', ' ', ' ', ' '}
};

// O piece (square)
inline constexpr char PIECE_O[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {' ', '#', '#', ' '}, // Centered 2x2
    {' ', ' ', ' ', ' '}
};

// S piece
inline constexpr char PIECE_S[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {'#', '#', ' ', ' '}, // S/Z-style shape
    {' ', ' ', ' ', ' '}
};

// T piece
inline constexpr char PIECE_T[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', '#', ' ', ' '}, // Extra block in the middle
    {' ', ' ', ' ', ' '}
};

// Z piece
inline constexpr char PIECE_Z[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', ' ', ' '},
    {' ', '#', '#', ' '}, // S/Z-style shape
    {' ', ' ', ' ', ' '}
};

inline constexpr const char (*TETROMINOES[])[4][4] = {
    &PIECE_I,
    &PIECE_J,
    &PIECE_L,
    &PIECE_O,
    &PIECE_S,
    &PIECE_T,
    &PIECE_Z
};

inline constexpr int kTetrominoTypeCount = 7;

template<int Type>
inline constexpr const char (&tetromino_shape())[4][4];

template<>
inline constexpr const char (&tetromino_shape<0>())[4][4] { return PIECE_I; }
template<>
inline constexpr const char (&tetromino_shape<1>())[4][4] { return PIECE_J; }
template<>
inline constexpr const char (&tetromino_shape<2>())[4][4] { return PIECE_L; }
template<>
inline constexpr const char (&tetromino_shape<3>())[4][4] { return PIECE_O; }
template<>
inline constexpr const char (&tetromino_shape<4>())[4][4] { return PIECE_S; }
template<>
inline constexpr const char (&tetromino_shape<5>())[4][4] { return PIECE_T; }
template<>
inline constexpr const char (&tetromino_shape<6>())[4][4] { return PIECE_Z; }

inline constexpr const char (&tetromino_shape_by_type(int type))[4][4] {
    switch (type) {
        case 0: return PIECE_I;
        case 1: return PIECE_J;
        case 2: return PIECE_L;
        case 3: return PIECE_O;
        case 4: return PIECE_S;
        case 5: return PIECE_T;
        case 6: return PIECE_Z;
        default: throw std::out_of_range("Invalid tetromino type");
    }
}

class block {
    protected:
        int type_ = 0;
    public:
        char piece[4][4];
        int profile[4];

        virtual ~block() = default;
        void make_profile() {
    // Compute the lowest occupied cell per column (used for collision/landing).
    for (int i = 0; i < 4; i++) {
        
        // Default for an empty column: -1 means "no block in this column".
        profile[i] = -1; 
        
        // Scan from bottom to top.
        for (int j = 0; j < 4; j++) { 
            
            // If there's a block, this is the lowest one.
            if (piece[i][j] == '#') {
                
                // Store the row index of the lowest block.
                profile[i] = j; 
                
                // Stop once we found the lowest block.
                break; 
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        std::cout << "Block: Column " << i << " lowest block found at row: " << profile[i] << std::endl;
    }
}
        virtual void rotate(){
            char temp[4][4];
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    temp[i][j] = piece[i][j];
                }
            }
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = temp[3-j][i];
                }
            }
        }
        int type() const { return type_; }

        block(const char init[4][4]){
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = init[i][j];
                }
            }
        }

        // Builds a specific tetromino (0..6).
        block(int tetrominoIndex){
            // Keep compatibility with older code: normalize invalid values instead of throwing.
            if (tetrominoIndex < 0) tetrominoIndex = 0;
            if (tetrominoIndex >= kTetrominoTypeCount) tetrominoIndex = kTetrominoTypeCount - 1;
            type_ = tetrominoIndex;
            const char (*selected_piece)[4][4] = TETROMINOES[type_];
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = (*selected_piece)[i][j];
                }
            }
        }

        block(){
            static bool seeded = false;
            if (!seeded) {
                srand(static_cast<unsigned int>(time(0)));
                seeded = true;
            }
            int random_index = rand() % 7; // Random index between 0 and 6
            type_ = random_index;
            const char (*selected_piece)[4][4] = TETROMINOES[random_index];
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = (*selected_piece)[i][j];
                }
            }
        }
};

// --- Inheritance: each tetromino is a derived type ---
template<int Type>
class TetrominoBlock : public block {
public:
    TetrominoBlock() : block(tetromino_shape<Type>()) {
        static_assert(Type >= 0 && Type < kTetrominoTypeCount, "Invalid tetromino Type");
        type_ = Type;
    }
};

// Special-case: the O piece doesn't change when rotating.
template<>
class TetrominoBlock<3> : public block {
public:
    TetrominoBlock() : block(PIECE_O) { type_ = 3; }
    void rotate() override {}
};

using IBlock = TetrominoBlock<0>;
using JBlock = TetrominoBlock<1>;
using LBlock = TetrominoBlock<2>;
using OBlock = TetrominoBlock<3>;
using SBlock = TetrominoBlock<4>;
using TBlock = TetrominoBlock<5>;
using ZBlock = TetrominoBlock<6>;

inline std::unique_ptr<block> make_block(int type) {
    switch (type) {
        case 0: return std::make_unique<IBlock>();
        case 1: return std::make_unique<JBlock>();
        case 2: return std::make_unique<LBlock>();
        case 3: return std::make_unique<OBlock>();
        case 4: return std::make_unique<SBlock>();
        case 5: return std::make_unique<TBlock>();
        case 6: return std::make_unique<ZBlock>();
        default: throw std::out_of_range("Invalid tetromino type");
    }
}



#endif //BLOCK_H