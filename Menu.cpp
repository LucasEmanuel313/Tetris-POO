#include "Menu.h"

Menu::Menu(){
  window = new sf::RenderWindow();
  winclose = new sf::RectangleShape();
  font = new sf::Font();
  image = new sf::Texture();
  bg = new sf::Sprite(*image);

  set_values();
}

Menu::~Menu(){
  delete window;
  delete winclose;
  delete font;
  delete image;
  delete bg;
}

void Menu::set_values(){
  window->create(sf::VideoMode({1280,720}), "Menu SFML");
  window->setPosition(sf::Vector2i(0,0));

  pos = 0;
  pressed = theselect = false;
  font->openFromFile("Tetris.ttf");
  image->loadFromFile("Images\\Background_Tetris.jpg");

  bg->setTexture(*image);

  pos_mouse = {0,0};
  mouse_coord = {0, 0};

  options = {"War Game", "Play", "Options", "About", "Quit"};
  texts.clear();
  texts.reserve(options.size());
  coords = {{590,40},{610,191},{590,282},{600,370},{623,457}};
  sizes = {20,28,24,24,24};

  for (std::size_t i{}; i < options.size(); ++i){
    sf::Text t(*font, options[i], static_cast<unsigned int>(sizes[i]));
    t.setOutlineColor(sf::Color::Black);
    t.setPosition(coords[i]);
    texts.push_back(std::move(t));
  }
  texts.at(1).setOutlineThickness(4);
  pos = 1;

  winclose->setSize(sf::Vector2f(23,26));
  winclose->setPosition(sf::Vector2f{1178,39});
  winclose->setFillColor(sf::Color::Transparent);

}

void Menu::loop_events(){
  
  while (const auto event = window->pollEvent()){
    (void)event;
    

    pos_mouse = sf::Mouse::getPosition(*window);
    mouse_coord = window->mapPixelToCoords(pos_mouse);

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && !pressed){
      if( pos < 4){
        ++pos;
        pressed = true;
        texts[pos].setOutlineThickness(4);
        texts[pos - 1].setOutlineThickness(0);
        pressed = false;
        theselect = false;
      }
    }

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && !pressed){
      if( pos > 1){
        --pos;
        pressed = true;
        texts[pos].setOutlineThickness(4);
        texts[pos + 1].setOutlineThickness(0);
        pressed = false;
        theselect = false;
      }
    }

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter) && !theselect){
      theselect = true;
      if( pos == 4){
        window->close();
      }
      std::cout << options[pos] << '\n';
    }

    if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
      if(winclose->getGlobalBounds().contains(mouse_coord)){
        //std::cout << "Close the window!" << '\n';
        window->close();
      }
    }
  }
}

void Menu::draw_all(){
  window->clear();
  window->draw(*bg);
  for(auto t : texts){
   window->draw(t); 
  }
  window->display();
}

void Menu::run_menu(){
  while(window->isOpen()){
    loop_events();
    draw_all();
  }
}