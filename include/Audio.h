#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API AudioManager {
public:
    static void Init() {
        if (!IsAudioDeviceReady()) InitAudioDevice();
        if (IsAudioDeviceReady()) ::SetMasterVolume(masterVolume);
    }

    static void Shutdown() {
        for (auto& pair : sounds) UnloadSound(pair.second);
        sounds.clear();
        for (auto& pair : music) UnloadMusicStream(pair.second);
        music.clear();
        if (IsAudioDeviceReady()) CloseAudioDevice();
    }

    static void LoadSound(const std::string& name, const std::string& filepath) {
        if (sounds.count(name)) return;
        Sound sound = ::LoadSound(filepath.c_str());
        if (IsSoundValid(sound)) sounds[name] = sound;
    }

    static void PlaySound(const std::string& name, float volume = 1.0f, float pitch = 1.0f) {
        auto it = sounds.find(name);
        if (it != sounds.end()) {
            SetSoundVolume(it->second, volume);
            SetSoundPitch(it->second, pitch);
            ::PlaySound(it->second);
        }
    }

    static void LoadMusic(const std::string& name, const std::string& filepath) {
        if (music.count(name)) return;
        Music stream = LoadMusicStream(filepath.c_str());
        if (IsMusicValid(stream)) music[name] = stream;
    }

    static void PlayMusic(const std::string& name, float volume = 1.0f, bool loop = true) {
        auto it = music.find(name);
        if (it != music.end()) {
            it->second.looping = loop;
            ::SetMusicVolume(it->second, volume);
            PlayMusicStream(it->second);
        }
    }

    static void StopMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) StopMusicStream(it->second);
    }

    // Adjust an active stream without restarting its playback cursor.
    static void SetMusicVolume(const std::string& name, float volume) {
        auto it = music.find(name);
        if (it != music.end() && std::isfinite(volume)) ::SetMusicVolume(it->second, Clamp(volume, 0.0f, 1.0f));
    }

    static void PauseMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) PauseMusicStream(it->second);
    }

    static void ResumeMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) ResumeMusicStream(it->second);
    }

    static void UpdateMusic() {
        for (auto& pair : music) UpdateMusicStream(pair.second);
    }

    static void SetMasterVolume(float v) {
        masterVolume = Clamp(v, 0.0f, 1.0f);
        if (IsAudioDeviceReady()) ::SetMasterVolume(masterVolume);
    }
    static float GetMasterVolume() { return masterVolume; }

private:
    static inline std::unordered_map<std::string, Sound> sounds;
    static inline std::unordered_map<std::string, Music> music;
    static inline float masterVolume = 1.0f;
};

}
