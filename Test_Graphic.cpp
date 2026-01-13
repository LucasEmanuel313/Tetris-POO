#include <SFML/Graphics.hpp>


#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NEXT_BLOCK_MENU_WIDTH 200
#define NEXT_BLOCK_MENU_HEIGHT 200
#define GRID_COLS 10
#define GRID_ROWS 22
#define GRID_POS_X 300
#define GRID_POS_Y 50
#define BLOCK_SIZE 30.f

int main()
{

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Title");

    sf::RectangleShape grid[GRID_COLS][GRID_ROWS];
    
    for (int i = 0; i < GRID_COLS; ++i)
    {
        for (int j = 0; j < GRID_ROWS; ++j)
        {
            grid[i][j].setSize({BLOCK_SIZE, BLOCK_SIZE});
            grid[i][j].setFillColor(sf::Color::White);
            grid[i][j].setOutlineThickness(2.f);
            grid[i][j].setOutlineColor(sf::Color(0, 0, 0));
            grid[i][j].setPosition({GRID_POS_X + i * BLOCK_SIZE, GRID_POS_Y + j * BLOCK_SIZE});
        }
    }
    //Draws the next block menu rectangle
    sf::RectangleShape rectangle({NEXT_BLOCK_MENU_WIDTH, NEXT_BLOCK_MENU_HEIGHT});
    sf::Color rectangleColor(150, 150, 150);
    rectangle.setPosition({GRID_POS_X + GRID_COLS * BLOCK_SIZE + 20.f, GRID_POS_Y});
    rectangle.setOutlineThickness(2.f);
    rectangle.setOutlineColor(sf::Color(0, 0, 0));
    rectangle.setFillColor(rectangleColor);
    while (window.isOpen())
    {
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear(sf::Color(255, 255, 255));
        for(int i = 0; i < 10; ++i)
        {
            for (int j = 0; j < 22; ++j)
            {
                window.draw(grid[i][j]);
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            rectangle.move({-1 * BLOCK_SIZE, 0});
        }
        window.draw(rectangle);
        window.display();
    }

    return 0;
}