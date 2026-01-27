#include <cstddef>
#include <cstdlib> // Para rand() e srand()
#include <ctime>   // Para time()
#include <iostream>
#include <memory>
#include <stdexcept>


#ifndef BLOCK_H
#define BLOCK_H

inline constexpr char PIECE_I[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', '#'}, // Quatro blocos em linha
    {' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' '}
};

// Peça J (Jota)
inline constexpr char PIECE_J[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', ' ', '#', ' '}, // O 'canto' no topo direito
    {' ', ' ', ' ', ' '}
};

// Peça L (Éle)
inline constexpr char PIECE_L[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {'#', ' ', ' ', ' '}, // O 'canto' no topo esquerdo
    {' ', ' ', ' ', ' '}
};

// Peça O (Quadrado)
inline constexpr char PIECE_O[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {' ', '#', '#', ' '}, // 2x2 centralizado
    {' ', ' ', ' ', ' '}
};

// Peça S (Ese)
inline constexpr char PIECE_S[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {'#', '#', ' ', ' '}, // Forma de Z (ou S)
    {' ', ' ', ' ', ' '}
};

// Peça T (Tê)
inline constexpr char PIECE_T[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', '#', ' ', ' '}, // Bloco extra no meio
    {' ', ' ', ' ', ' '}
};

// Peça Z (Zeta)
inline constexpr char PIECE_Z[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', ' ', ' '},
    {' ', '#', '#', ' '}, // Forma de S (ou Z)
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
    // Itera por COLUNAS (i)
    for (int i = 0; i < 4; i++) {
        
        // Define o valor padrão para uma coluna vazia
        // (Ex: -1 se nenhuma peça for encontrada, ou 4 se você quiser a altura real)
        // Usarei -1 para indicar "vazio"
        profile[i] = -1; 
        
        // Itera por colunas de BAIXO para CIMA (de 3 a 0)
        for (int j = 0; j < 4; j++) { 
            
            // Verifica se a posição atual [Linha j][Coluna i] tem um bloco
            if (piece[i][j] == '#') {
                
                // Armazena a linha j (o bdloco MAIS BAIXO) como o perfil
                profile[i] = j; 
                
                // Como encontramos o bloco mais baixo, paramos a iteração desta coluna
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

        // Constrói um tetromino específico (0..6)
        block(int tetrominoIndex){
            // Mantém compatibilidade com o código antigo: se vier inválido,
            // vamos normalizar (sem lançar) para não quebrar o jogo.
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
            int random_index = rand() % 7; // Gera um índice aleatório entre 0 e 6
            type_ = random_index;
            const char (*selected_piece)[4][4] = TETROMINOES[random_index];
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = (*selected_piece)[i][j];
                }
            }
        }
};

// --- Herança: cada tetromino é um tipo derivado ---
template<int Type>
class TetrominoBlock : public block {
public:
    TetrominoBlock() : block(tetromino_shape<Type>()) {
        static_assert(Type >= 0 && Type < kTetrominoTypeCount, "Invalid tetromino Type");
        type_ = Type;
    }
};

// Exemplo de especialização via herança: O não muda ao rotacionar.
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