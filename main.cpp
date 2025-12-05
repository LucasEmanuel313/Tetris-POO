//
// Created by lucas on 21/11/2025.
//
#include <iostream>
#include <stdlib.h>
#include <conio.h>
#include "block.h"
#include "table.h"

int main(){
    char button_pressed;
    table ta;
    //ta.make_profile_table();
    ta.add_block();
    ta.print_table();
    while(true){
        button_pressed = _getch();
        switch (button_pressed){
            case 'a':
                ta.block_left();
                break;
            case 'd':
                ta.block_right();
                break;
            case 's':
                ta.block_descend();
                break;
            case 'w':
                ta.rotate_block();
                break;
            case ' ':
                ta.block_drop();
                break;
            default:
                break;
        }
        system("CLS");
        ta.print_table();
        ta.make_profile_table();
    }

    return 0;
}