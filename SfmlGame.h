#pragma once

#include <SFML/Graphics.hpp>

#include "block.h"
#include "table.h"
#include "UiTextBox.h"

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
    MusicManager * musicManager;
    sf::RectangleShape cells[GRID_COLS][GRID_ROWS];
    table* game_table = nullptr;
    sf::RenderWindow* window = nullptr;
    const sf::Font* font = nullptr;

    bool prevKeyStates[7] = {false, false, false, false, false, false, false};
    // 0=Left, 1=Right, 2=Down, 3=Up, 4=Space, 5=C(hold), 6=P(mute)

    static sf::Color colorForType(int type) {
        switch (type) {
            case 0: return sf::Color(0, 150, 150);   // I
            case 1: return sf::Color(0, 128, 0);     // J
            case 2: return sf::Color(180, 100, 0);   // L
            case 3: return sf::Color(180, 180, 0);   // O
            case 4: return sf::Color(150, 0, 0);     // S
            case 5: return sf::Color(150, 0, 150);   // T
            case 6: return sf::Color(0, 0, 150);     // Z
            default: return sf::Color(200, 200, 200);
        }
    }

    static sf::Color fixedBlockColor() {
        return sf::Color(80, 80, 80);
    }

    void initGrid() {
        for (int x = 0; x < GRID_COLS; ++x) {
            for (int y = 0; y < GRID_ROWS; ++y) {
                cells[x][y].setSize({BLOCK_SIZE, BLOCK_SIZE});
                cells[x][y].setOutlineThickness(2.f);
                cells[x][y].setOutlineColor(sf::Color::Black);
            }
        }
    }

    void drawCell(int x, int y, sf::Color color) {
        float sx = GRID_POS_X + x * BLOCK_SIZE;
        float sy = GRID_POS_Y + (GRID_ROWS - 1 - y) * BLOCK_SIZE;
        cells[x][y].setPosition({sx, sy});
        cells[x][y].setFillColor(color);
        window->draw(cells[x][y]);
    }

    void drawPieceAt(const block* b, int baseX, int baseY, sf::Color color, bool ghost = false) {
        if (!b) return;
        if (ghost) {
            color.a = 90;
        }

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (b->piece[i][j] == '#') {
                    int x = baseX + i;
                    int y = baseY + j;
                    if (x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS) {
                        drawCell(x, y, color);
                    }
                }
            }
        }
    }

    void drawBoard() {
        // 1) blocos fixos (sem a peça atual)
        for (int y = 0; y < GRID_ROWS; ++y) {
            for (int x = 0; x < GRID_COLS; ++x) {
                int t = game_table->get_fixed_type(x, y);
                if (t >= 0 && t <= 6) drawCell(x, y, colorForType(t));
                else if (t == 7) drawCell(x, y, fixedBlockColor());
                else drawCell(x, y, sf::Color::White);
            }
        }

        // 2) ghost + peça atual colorida
        const block* cur = game_table->get_current_block();
        if (!cur) return;
        int type = cur->type();
        sf::Color c = colorForType(type);

        int ghostY = game_table->get_ghost_y();
        sf::Color ghostColor = c;
        ghostColor.a = 80;
        drawPieceAt(cur, game_table->get_block_x_pos(), ghostY, ghostColor, true);
        drawPieceAt(cur, game_table->get_block_x_pos(), game_table->get_block_y_pos(), c, false);
    }

    void drawMiniPiece(int type, float x0, float y0) {
        if (type < 0 || type > 6) return;
        const char (*p)[4][4] = TETROMINOES[type];

        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if ((*p)[x][y] == '#') {
                    sf::RectangleShape r;
                    r.setSize({BLOCK_SIZE, BLOCK_SIZE});
                    r.setOutlineThickness(2.f);
                    r.setOutlineColor(sf::Color::Black);
                    r.setFillColor(colorForType(type));
                    r.setPosition({x0 + x * BLOCK_SIZE, y0 + (3 - y) * BLOCK_SIZE});
                    window->draw(r);
                }
            }
        }
    }

    void drawSidePanels() {
        if (!font) return;

        // UI: mostrar apenas peças futuras.
        // Topo ("HOLD"): próxima peça (next[0])
        // Lista ("NEXT"): próximas seguintes (next[1], next[2])
        static constexpr int kTotalPreviewCount = 3;
        static constexpr int kNextListCount = kTotalPreviewCount - 1;

        const float miniX = PANEL_POS_X + PANEL_WIDTH / 2.f - BLOCK_SIZE * 2.f;
        const float holdMiniY = GRID_POS_Y + 35.f;
        const float nextLabelY = holdMiniY + 4.f * BLOCK_SIZE + 45.f;
        const float nextMiniY0 = nextLabelY + 35.f;
        const float nextSpacingY = 4.f * BLOCK_SIZE + 35.f;
        const float scoreY = nextMiniY0 + (float)kNextListCount * nextSpacingY + 25.f;

        // Score
        sf::Text scoreText(*font);
        scoreText.setString("Score: " + std::to_string(game_table->get_score()));
        scoreText.setPosition({PANEL_POS_X + 10.f, scoreY});
        scoreText.setFillColor(sf::Color::Black);
        scoreText.setCharacterSize(20);
        drawTextWithBox(*window, scoreText);

        // Hold
        sf::Text holdText(*font);
        holdText.setString("HOLD");
        holdText.setPosition({PANEL_POS_X + 10.f, GRID_POS_Y});
        holdText.setFillColor(sf::Color::Black);
        holdText.setCharacterSize(18);
        drawTextWithBox(*window, holdText);

        auto preview = game_table->get_next_types(kTotalPreviewCount);
        int next0 = preview.empty() ? -1 : preview[0];
        drawMiniPiece(next0, miniX, holdMiniY);
        // Next
        sf::Text nextText(*font);
        nextText.setString("NEXT");
        nextText.setPosition({PANEL_POS_X + 10.f, nextLabelY});
        nextText.setFillColor(sf::Color::Black);
        nextText.setCharacterSize(18);
        drawTextWithBox(*window, nextText);

        for (size_t i = 1; i < preview.size(); ++i) {
            drawMiniPiece(preview[i], miniX, nextMiniY0 + (float)(i - 1) * nextSpacingY);
        }
    }

    void drawPanelBox() {
        sf::RectangleShape box;
        box.setPosition({PANEL_POS_X, GRID_POS_Y});
        box.setSize({PANEL_WIDTH, GRID_HEIGHT});
        box.setFillColor(sf::Color(220, 220, 220));
        box.setOutlineThickness(3.f);
        box.setOutlineColor(sf::Color::Black);
        window->draw(box);
    }

public:
    SfmlGame(table& t, sf::RenderWindow& w, const sf::Font& f, MusicManager& mm)
        : game_table(&t), window(&w), font(&f), musicManager(&mm) {
        initGrid();
        window->setFramerateLimit(60);
    }

    SfmlGame(table& t, sf::RenderWindow& w, const sf::Font& f)
        : game_table(&t), window(&w), font(&f){
        initGrid();
        window->setFramerateLimit(60);
    }

    void HandleEvents() {
        bool currentKeyStates[7] = {
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P)
        };

        if (!prevKeyStates[0] && currentKeyStates[0]) game_table->block_left();
        if (!prevKeyStates[1] && currentKeyStates[1]) game_table->block_right();
        if (!prevKeyStates[2] && currentKeyStates[2]) game_table->block_descend();
        if (!prevKeyStates[3] && currentKeyStates[3]) game_table->rotate_block();
        if (!prevKeyStates[4] && currentKeyStates[4]) game_table->block_drop();
        if (!prevKeyStates[5] && currentKeyStates[5]) game_table->hold_block();
        if (!prevKeyStates[6] && currentKeyStates[6]) musicManager->setPlaying(!(musicManager->getIsPlaying()));

        for (int i = 0; i < 7; ++i) prevKeyStates[i] = currentKeyStates[i];

        // se a peça travou, a mesa marca needsSpawn
        game_table->spawn_if_needed();
    }

    int getScore() const {
        if (!game_table) return 0;
        return game_table->get_score();
    }
    
    void draw_game() {
        drawBoard();
        drawPanelBox();
        drawSidePanels();
    }
};
