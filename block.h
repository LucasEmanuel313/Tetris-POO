#include <cstddef>
#include <cstdlib> // Para rand() e srand()
#include <ctime>   // Para time()
#include <iostream>


#ifndef BLOCK_H
#define BLOCK_H

const char PIECE_I[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', '#'}, // Quatro blocos em linha
    {' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' '}
};

// Peça J (Jota)
const char PIECE_J[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', ' ', '#', ' '}, // O 'canto' no topo direito
    {' ', ' ', ' ', ' '}
};

// Peça L (Éle)
const char PIECE_L[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {'#', ' ', ' ', ' '}, // O 'canto' no topo esquerdo
    {' ', ' ', ' ', ' '}
};

// Peça O (Quadrado)
const char PIECE_O[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {' ', '#', '#', ' '}, // 2x2 centralizado
    {' ', ' ', ' ', ' '}
};

// Peça S (Ese)
const char PIECE_S[4][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '#', '#', ' '},
    {'#', '#', ' ', ' '}, // Forma de Z (ou S)
    {' ', ' ', ' ', ' '}
};

// Peça T (Tê)
const char PIECE_T[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', '#', ' '},
    {' ', '#', ' ', ' '}, // Bloco extra no meio
    {' ', ' ', ' ', ' '}
};

// Peça Z (Zeta)
const char PIECE_Z[4][4] = {
    {' ', ' ', ' ', ' '},
    {'#', '#', ' ', ' '},
    {' ', '#', '#', ' '}, // Forma de S (ou Z)
    {' ', ' ', ' ', ' '}
};

static const char (*TETROMINOES[])[4][4] = {
    &PIECE_I,
    &PIECE_J,
    &PIECE_L,
    &PIECE_O,
    &PIECE_S,
    &PIECE_T,
    &PIECE_Z
};
class block {
    protected:
        
    public:
        char piece[4][4];
        int profile[4];
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
        void rotate(){
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
        block(const char init[4][4]){
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = init[i][j];
                }
            }
        }
        block(){
            srand(static_cast<unsigned int>(time(0))); // Inicializa a semente do gerador de números aleatórios
            int random_index = rand() % 7; // Gera um índice aleatório entre 0 e 6
            const char (*selected_piece)[4][4] = TETROMINOES[random_index];
            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++) {
                    piece[i][j] = (*selected_piece)[i][j];
                }
            }
        }
};



#endif //BLOCK_H