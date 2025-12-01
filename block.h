#include <cstddef>

#ifndef BLOCK_H
#define BLOCK_H

class block {
    protected:
        
    public:
        char piece[4][4];
        int profile[4];
        void make_profile(){
            for(int i=0; i<4; i++){
                profile[i]=0;
                for(int j=3; j>=0 && profile[i]==0; j--){
                    if(piece[i][j]=='#'){
                        profile[i]=j;
                        break;
                    }
                }
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

};

#endif //BLOCK_H