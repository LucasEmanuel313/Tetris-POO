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

    static bool tryOpen(sf::Music& m, const std::string& path) {
        if (m.openFromFile(path)) return true;
        // fallback for when exe is run from bin/
        if (m.openFromFile("../" + path)) return true;
        return false;
    }

public:
    MusicManager() : currentFilePath("") {}

    void updateMusic(GameState state) {
        std::string musicFilePath;
        if (isPlaying) {
            switch (state) {
                case GameState::MENU:
                    musicFilePath = "assets/Music/Main_Menu_Music.ogg";
                    music.setVolume(50.f);
                    break;
                case GameState::SINGLEPLAYER:
                    musicFilePath = "assets/Music/Tetris_music.mp3";
                    music.setVolume(50.f);
                    break;
                case GameState::MULTIPLAYER:
                    musicFilePath = "assets/Music/Tetris_music.mp3";
                    music.setVolume(50.f);
                    break;
                case GameState::GAME_OVER:
                    musicFilePath = "assets/Music/GameOver.ogg";
                    music.setLooping(false);
                    music.setVolume(50.f);
                    break;
                default:
                    stopMusic();
                    return;
            }
        }
        if (currentFilePath == musicFilePath) return;

        if (tryOpen(music, musicFilePath)) {
            currentFilePath = musicFilePath;
#if defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3
            music.setLooping(true);
#else
            music.setLoop(true);
#endif
            music.play();
        } else {
            std::cerr << "Error: Could not load " << musicFilePath << std::endl;
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
