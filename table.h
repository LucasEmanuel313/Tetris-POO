//
// Created by lucas on 21/11/2025.
//
#ifndef TABLE_H
#define TABLE_H
#include "block.h" 
#include <iostream>

class table {

private:
    char positions [10][22];
    int profile[10];
    block* current_block;
    int block_x_pos;
    int block_y_pos;

public:
    
    void clear_line(int line){
        for (int i = 0; i <= 9; i++){
            positions [i][line] = ' ';
        }
        for (int j = line; j < 21; j++){
            for (int i = 0; i <= 9; i++){
                positions [i][line] = positions [i][line+1];
            }
        }
        for (int i = 0; i <= 9; i++){
            positions [i][21] = ' ';
        }
    
    }
    void print_table(){
        for (int i = 21; i >=0; i--) {
            //std::cout<<"-------------------------------\n";
            for (int j = 0; j < 10; j++) {
                std::cout<< '|'<<positions[j][i] << '|';
                
            }
            std:: cout<<'\n';
        }
        std::cout<<"-------------------------------\n";
    }
    void make_profile_table(){
        for(int i=0; i<10; i++){
            profile[i]=0;
            for(int j=21; j>=0 && profile[i]==0; j--){
                if(positions[i][j]=='#'){
                    profile[i]=j;
                    break;
                }
            }
        }
        for(int i=0; i<10; i++){
            std::cout<<"Column "<<i<<" height: "<<profile[i]<<std::endl;
        }
        current_block->make_profile();
    }
    void update_table(){
        for(int i=0; i<4; i++){
            for(int j=0; j<4; j++){
                positions [block_x_pos+i][block_y_pos+j] = current_block->piece[i][j]; 
            }
        }
    }
    void block_clear(){
        for(int i=0; i<4; i++){
            for(int j=0; j<4; j++){
                positions [block_x_pos+i][block_y_pos+j] = ' '; 
            }
        }
    }
    void add_block(){ 
        block* ptr = new block();
        current_block = ptr;
        block_x_pos = 3;
        block_y_pos = 18;
        update_table();
    }
    void rotate_block(){
        block_clear();
        current_block->rotate();
        update_table();
    }
    void block_descend(){
        block_clear();
        block_y_pos -= 1;
        update_table();
    }
    void block_left(){
        block_clear();
        if(block_x_pos > 0){
            block_x_pos -= 1;
        }
        update_table();
    }
    void block_right(){
        block_clear();
        block_x_pos += 1;
        update_table();
    }
    void block_drop(){
        block_clear();
        block_y_pos = 0;
        update_table();
        delete current_block;
        add_block();
    }
    table(){
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 22; j++) {
                positions [i][j] = ' ';
            }
        }
    }    
};


#endif //TABLE_H