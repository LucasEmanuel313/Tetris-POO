#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <string>

#include "table.h"

struct OpponentState {
    bool hasBoard = false;
    int score = 0;
    std::string data; // 220 chars: '.' or '#', y=0..21 then x=0..9

    OpponentState();
};

void flushConsoleInputEvents();
void menuClear();
bool confirmLeaveToMenu();

// 1 = revanche / nova partida, 0 = menu
int postGameChoiceWithTitle(const char* title);

void renderFrame(table& ta);
void renderFrameMultiplayer(const table& local, const OpponentState& opp, int pendingIncomingGarbage);

#endif
