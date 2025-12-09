#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API Input {
public:
    // Keyboard
    static bool GetKey(int key) {
        return IsKeyDown(key);
    }
    
    static bool GetKeyDown(int key) {
        return IsKeyPressed(key);
    }
    
    static bool GetKeyUp(int key) {
        return IsKeyReleased(key);
    }
    
    // Mouse
    static bool GetMouseButton(int button) {
        return IsMouseButtonDown(button);
    }
    
    static bool GetMouseButtonDown(int button) {
        return IsMouseButtonPressed(button);
    }
    
    static bool GetMouseButtonUp(int button) {
        return IsMouseButtonReleased(button);
    }
    
    static Vector2 GetMousePosition() {
        return ::GetMousePosition();
    }
    
    static Vector2 GetMouseDelta() {
        return ::GetMouseDelta();
    }
    
    static float GetMouseWheelMove() {
        return ::GetMouseWheelMove();
    }
    
    // Axis input (WASD or Arrow keys)
    static float GetAxis(const std::string& axis) {
        if (axis == "Horizontal") {
            float h = 0;
            if (GetKey(KEY_A) || GetKey(KEY_LEFT)) h -= 1;
            if (GetKey(KEY_D) || GetKey(KEY_RIGHT)) h += 1;
            return h;
        }
        else if (axis == "Vertical") {
            float v = 0;
            if (GetKey(KEY_W) || GetKey(KEY_UP)) v -= 1;
            if (GetKey(KEY_S) || GetKey(KEY_DOWN)) v += 1;
            return v;
        }
        return 0;
    }
    
    static Vector2 GetMovementInput() {
        return {GetAxis("Horizontal"), GetAxis("Vertical")};
    }
};

class RAYGAME_API AudioManager {
public:
    static void Init() {
        InitAudioDevice();
    }
    
    static void Shutdown() {
        // Unload all sounds
        for (auto& pair : sounds) {
            UnloadSound(pair.second);
        }
        sounds.clear();
        
        // Unload all music
        for (auto& pair : music) {
            UnloadMusicStream(pair.second);
        }
        music.clear();
        
        CloseAudioDevice();
    }
    
    static void LoadSound(const std::string& name, const std::string& filepath) {
        Sound sound = ::LoadSound(filepath.c_str());
        sounds[name] = sound;
    }
    
    static void PlaySound(const std::string& name, float volume = 1.0f) {
        if (sounds.find(name) != sounds.end()) {
            SetSoundVolume(sounds[name], volume);
            ::PlaySound(sounds[name]);
        }
    }
    
    static void LoadMusic(const std::string& name, const std::string& filepath) {
        Music mus = LoadMusicStream(filepath.c_str());
        music[name] = mus;
    }
    
    static void PlayMusic(const std::string& name, float volume = 1.0f, bool loop = true) {
        if (music.find(name) != music.end()) {
            Music& mus = music[name];
            mus.looping = loop;
            SetMusicVolume(mus, volume);
            PlayMusicStream(mus);
        }
    }
    
    static void StopMusic(const std::string& name) {
        if (music.find(name) != music.end()) {
            StopMusicStream(music[name]);
        }
    }
    
    static void UpdateMusic() {
        for (auto& pair : music) {
            UpdateMusicStream(pair.second);
        }
    }
    
private:
    static inline std::unordered_map<std::string, Sound> sounds;
    static inline std::unordered_map<std::string, Music> music;
};

}

