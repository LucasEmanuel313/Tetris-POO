#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Config.hpp>
#include <iostream>
#include <string>

#include "GameState.h"

class MusicManager {
private:
    sf::Music music;
    std::string currentFilePath;
    bool isPlaying = true;

public:
    MusicManager() : currentFilePath("") {}

    void updateMusic(GameState state) {
        std::string musicFilePath;
        if (isPlaying) {
            switch (state) {
                case GameState::MENU:
                    musicFilePath = "Music/Main_Menu_Music.ogg";
                    music.setVolume(50.f);
                    break;
                case GameState::SINGLEPLAYER:
                    musicFilePath = "Music/Tetris_music.mp3";
                    music.setVolume(50.f);
                    break;
                case GameState::MULTIPLAYER:
                    musicFilePath = "Music/Tetris_music.mp3";
                    music.setVolume(50.f);
                    break;
                case GameState::GAME_OVER:
                    musicFilePath = "Music/GameOver.ogg";
                    music.setLooping(false);
                    music.setVolume(50.f);
                    break;
                default:
                    stopMusic();
                    return;
            }
        }
        if (currentFilePath == musicFilePath) return;

        if (music.openFromFile(musicFilePath)) {
            currentFilePath = musicFilePath;
#if defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3
            music.setLooping(true);
#else
            music.setLoop(true);
#endif
            music.play();
        } else {
            std::cerr << "Erro: Nao foi possivel carregar " << musicFilePath << std::endl;
        }
    }

    void setPlaying(bool play) {
        isPlaying = play;
        if (!isPlaying) {
            stopMusic();
        }
    }

    bool getIsPlaying() const {
        return isPlaying;
    }

    void stopMusic() {
        music.stop();
        currentFilePath.clear();
    }
};
