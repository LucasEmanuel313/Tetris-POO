#pragma once

#include <SFML/Graphics.hpp>

#include "block.h"
#include "table.h"
#include "UiTextBox.h"

class MusicManager;

class SfmlGame {
public:
    static constexpr int GRID_COLS = 10;
    static constexpr int GRID_ROWS = 22;

    static constexpr float WINDOW_WIDTH = 1200.f;
    static constexpr float WINDOW_HEIGHT = 800.f;

    static constexpr float BLOCK_SIZE = 30.f;
    static constexpr float GRID_WIDTH = GRID_COLS * BLOCK_SIZE;
    static constexpr float GRID_HEIGHT = GRID_ROWS * BLOCK_SIZE;

    static constexpr float GRID_POS_X = (WINDOW_WIDTH - GRID_WIDTH) / 2.f;
    static constexpr float GRID_POS_Y = 50.f;

    static constexpr float PANEL_POS_X = GRID_POS_X + GRID_WIDTH + 20.f;
    static constexpr float PANEL_WIDTH = 140.f;
    static constexpr float PANEL_HEIGHT = 400.f;

private:
    MusicManager* musicManager = nullptr;
    sf::RectangleShape cells[GRID_COLS][GRID_ROWS];
    table* game_table = nullptr;
    sf::RenderWindow* window = nullptr;
    const sf::Font* font = nullptr;

    bool prevKeyStates[7] = {false, false, false, false, false, false, false};
    // 0=Left, 1=Right, 2=Down, 3=Up, 4=Space, 5=C(hold), 6=P(mute)

    static sf::Color colorForType(int type);
    static sf::Color fixedBlockColor();

    void initGrid();
    void drawCell(int x, int y, sf::Color color);
    void drawPieceAt(const block* b, int baseX, int baseY, sf::Color color, bool ghost = false);
    void drawBoard();
    void drawMiniPiece(int type, float x0, float y0);
    void drawSidePanels();
    void drawPanelBox();

public:
    SfmlGame(table& t, sf::RenderWindow& w, const sf::Font& f, MusicManager& mm);
    SfmlGame(table& t, sf::RenderWindow& w, const sf::Font& f);

    void HandleEvents();
    int getScore() const;
    void draw_game();
};
