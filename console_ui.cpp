#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <conio.h>
#include <iostream>
#include <string>
#include <vector>

#include "console_ui.h"

// -------------------- Console rendering helpers --------------------
static HANDLE g_hOut = INVALID_HANDLE_VALUE;
static bool g_consoleInited = false;

static void clearConsoleWinAPI() {
    if (g_hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(g_hOut, &csbi)) return;

    DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    DWORD count;
    COORD home = {0, 0};

    FillConsoleOutputCharacter(g_hOut, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(g_hOut, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(g_hOut, home);
}

void flushConsoleInputEvents() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        FlushConsoleInputBuffer(hIn);
    }
}

static void initGameConsoleOnce() {
    if (g_consoleInited) return;
    g_consoleInited = true;

    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // esconder cursor (reduz flicker)
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 25;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(g_hOut, &ci);

    // acelera i/o do cout
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    clearConsoleWinAPI(); // limpa uma vez
}

void renderFrame(table& ta) {
    initGameConsoleOnce();
    COORD home{0, 0};
    SetConsoleCursorPosition(g_hOut, home);
    ta.print_table();
    std::cout.flush();
}

void menuClear() {
    // Para menus: limpar a tela inteira (sem flicker absurdo, porque não é por frame)
    if (g_hOut == INVALID_HANDLE_VALUE) g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    clearConsoleWinAPI();
}

static void drainKb() {
    while (_kbhit()) {
        (void)_getch();
    }
}

bool confirmLeaveToMenu() {
    drainKb();
    menuClear();
    std::cout << "Return to menu? (Y/N) ";
    std::cout.flush();
    while (true) {
        char c = _getch();
        if (c == 'y' || c == 'Y') return true;
        if (c == 'n' || c == 'N' || c == 27) return false; // ESC = cancelar
    }
}

// 1 = revanche / nova partida, 0 = menu
// Não limpa a tela: use após renderizar o(s) tabuleiro(s)
static int postGameChoiceInline() {
    drainKb();
    std::cout << "1) Play again\n";
    std::cout << "2) Back to menu\n";
    std::cout << "> ";
    std::cout.flush();
    while (true) {
        char c = _getch();
        if (c == '1') return 1;
        if (c == '2' || c == 27) return 0;
    }
}

int postGameChoiceWithTitle(const char* title) {
    std::cout << "\n" << title << "\n\n";
    return postGameChoiceInline();
}

// -------------------- Multiplayer rendering (two boards) --------------------
OpponentState::OpponentState() : data(10 * 22, '.') {}

static std::string miniLineForType(int type, int miniRowFromTop) {
    if (type < 0 || type > 6) return "        ";
    const char (*p)[4][4] = TETROMINOES[type];
    std::string out;
    out.reserve(8);
    int y = 3 - miniRowFromTop;
    for (int x = 0; x < 4; ++x) {
        out += ((*p)[x][y] == '#') ? "[]" : "  ";
    }
    return out;
}

static std::string sidePanelLineMultiplayer(const table& local, int rowIndexFromTop, int pendingIncomingGarbage) {
    // Layout (22 linhas): HOLD(1+4) + blank(1) + NEXT(1 + 3*4 + 2 blanks) + Garbage(1)
    const int hold = local.get_hold_type();
    const std::vector<int> next = local.get_next_types(3);

    if (rowIndexFromTop == 0) return "HOLD";
    if (rowIndexFromTop >= 1 && rowIndexFromTop <= 4) {
        if (hold < 0) return (rowIndexFromTop == 1) ? "(none)" : "";
        return miniLineForType(hold, rowIndexFromTop - 1);
    }
    if (rowIndexFromTop == 5) return "";
    if (rowIndexFromTop == 6) return "NEXT";

    int base = 7;
    for (int n = 0; n < 3; ++n) {
        int start = base + n * 5;
        int end = start + 3;
        if (rowIndexFromTop >= start && rowIndexFromTop <= end) {
            int t = (n < (int)next.size()) ? next[n] : -1;
            return miniLineForType(t, rowIndexFromTop - start);
        }
        if (rowIndexFromTop == start + 4) return "";
    }

    if (rowIndexFromTop == 21) {
        if (pendingIncomingGarbage > 0) return "Garbage: +" + std::to_string(pendingIncomingGarbage);
        return "Garbage: 0";
    }
    return "";
}

void renderFrameMultiplayer(const table& local, const OpponentState& opp, int pendingIncomingGarbage) {
    initGameConsoleOnce();
    COORD home{0, 0};
    SetConsoleCursorPosition(g_hOut, home);

    const std::string my = local.serialize_board();
    static const std::string kEmptyBoard(10 * 22, '.');
    const std::string& op = opp.hasBoard ? opp.data : kEmptyBoard;

    // títulos
    std::cout << "   YOU";
    std::cout << "                          ";
    std::cout << "OPPONENT";
    std::cout << "\n\n";

    // 22 linhas do tabuleiro (imprime de cima para baixo)
    for (int y = 21; y >= 0; --y) {
        // board local
        for (int x = 0; x < 10; ++x) {
            char c = my[y * 10 + x];
            std::cout << '|' << (c == '#' ? '#' : ' ') << '|';
        }

        std::cout << "      ";

        // board oponente
        for (int x = 0; x < 10; ++x) {
            char c = op[y * 10 + x];
            std::cout << '|' << (c == '#' ? '#' : ' ') << '|';
        }

        // painel à direita (hold/next/garbage)
        int rowIndexFromTop = 21 - y;
        std::string panel = sidePanelLineMultiplayer(local, rowIndexFromTop, pendingIncomingGarbage);
        if (!panel.empty()) {
            std::cout << "   " << panel;
        }

        std::cout << '\n';
    }

    std::cout << "-------------------------------      -------------------------------\n";
    std::cout << "Score: " << local.get_score() << "   C=HOLD";
    std::cout << "   Garbage: " << (pendingIncomingGarbage > 0 ? ("+" + std::to_string(pendingIncomingGarbage)) : "0");
    std::cout << std::string(10, ' ');
    std::cout << "                         ";
    std::cout << "Score: " << (opp.hasBoard ? opp.score : 0) << "\n";
    std::cout.flush();
}
