//
// Created by lucas on 21/11/2025.
//
#include <iostream>
#include <stdlib.h>
#include <conio.h>
#include <chrono>
#include <windows.h>   // para Sleep()

#include "block.h"
#include "table.h"

int main() {
    using clock = std::chrono::steady_clock;

    table ta;
    ta.add_block();
    ta.print_table();

    auto lastFall = clock::now();
    std::chrono::milliseconds fallInterval(500); // 0,5 s por queda

    while (true) {

        // 1) Input não bloqueante
        if (_kbhit()) {
            char button_pressed = _getch();
            switch (button_pressed) {
                case 'a':
                    ta.block_left();
                    break;
                case 'd':
                    ta.block_right();
                    break;
                case 's':
                    ta.block_descend(); // acelera a queda
                    break;
                case 'w':
                    ta.rotate_block();
                    break;
                case ' ':
                    ta.block_drop();    // queda instantânea
                    break;
                default:
                    break;
            }
            system("CLS");
            ta.print_table();
        }

    // 2) Queda automática
    auto now = clock::now();
    if (now - lastFall >= fallInterval) {
        ta.block_descend();
        lastFall = now;

        system("CLS");
        ta.print_table();
    }

    // 3) Pequena pausa para não comer 100% da CPU
    Sleep(10); // em milissegundos
    }

    return 0;
}
