#pragma once
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include "GameState.h"

class MusicManager {
private:
    sf::Music music; // O manager é o dono do recurso
    std::string currentFilePath;

public:
    MusicManager() : currentFilePath("") {}

    void updateMusic(GameState state) {
        std::string musicFilePath;
        
        switch (state) {
            case GameState::MENU:
                musicFilePath = "Music/Main_Menu_Music.ogg";
                break;
            case GameState::SINGLEPLAYER:
                musicFilePath = "Music/Tetris_music.mp3"; // Recomendado .ogg
                break;
            default:
                stopMusic();
                return;
        }

        // Evita recarregar se a música já for a mesma
        if (currentFilePath == musicFilePath) return;

        if (music.openFromFile(musicFilePath)) {
            currentFilePath = musicFilePath;
            music.setLooping(true); // No SFML 2.5+, use setLoop(true) em vez de setLooping
            music.play();
        } else {
            std::cerr << "Erro: Nao foi possivel carregar " << musicFilePath << std::endl;
        }
    }

    void stopMusic() {
        music.stop();
        currentFilePath = "";
    }
};