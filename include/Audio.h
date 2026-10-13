#pragma once

#include "Core.h"
#include <chrono>

namespace RayGame {

enum class AudioChannel { Sfx, Music };

class RAYGAME_API AudioManager {
public:
    static void Init() {
        if (!IsAudioDeviceReady()) InitAudioDevice();
        if (IsAudioDeviceReady()) ::SetMasterVolume(masterVolume);
    }

    static void Shutdown() {
        for (auto& pair : sounds) Release(pair.second);
        sounds.clear();
        for (auto& pair : music) UnloadMusicStream(pair.second.stream);
        music.clear();
        if (IsAudioDeviceReady()) CloseAudioDevice();
    }

    // `voices` > 1 lets rapid repeats overlap instead of restarting one another. Extra
    // voices are raylib sound aliases sharing the same sample data.
    static void LoadSound(const std::string& name, const std::string& filepath, int voices = 1) {
        if (sounds.count(name)) return;
        Sound sound = ::LoadSound(filepath.c_str());
        if (!IsSoundValid(sound)) return;
        SoundEntry entry;
        entry.voices.push_back(sound);
        for (int i = 1; i < std::clamp(voices, 1, kMaxVoices); ++i) {
            Sound alias = LoadSoundAlias(sound);
            if (IsSoundValid(alias)) entry.voices.push_back(alias);
        }
        sounds[name] = std::move(entry);
    }

    // Plays on a free voice, or replaces the voice that started earliest. Returns false
    // when the sound is unknown or still inside its cooldown.
    static bool PlaySound(const std::string& name, float volume = 1.0f, float pitch = 1.0f) {
        auto it = sounds.find(name);
        if (it == sounds.end() || it->second.voices.empty()) return false;
        auto& entry = it->second;
        double now = Now();
        if (entry.cooldown > 0 && now - entry.lastPlay < entry.cooldown) return false;
        entry.lastPlay = now;
        size_t voice = entry.next;
        for (size_t i = 0; i < entry.voices.size(); ++i) {
            size_t candidate = (entry.next + i) % entry.voices.size();
            if (!IsSoundPlaying(entry.voices[candidate])) { voice = candidate; break; }
        }
        entry.next = (voice + 1) % entry.voices.size();
        Sound& sound = entry.voices[voice];
        SetSoundVolume(sound, std::max(0.0f, std::isfinite(volume) ? volume : 0.0f) * sfxVolume);
        SetSoundPitch(sound, std::isfinite(pitch) && pitch > 0 ? pitch : 1.0f);
        ::PlaySound(sound);
        return true;
    }

    // Minimum seconds between two plays of the same sound; 0 disables throttling.
    static void SetSoundCooldown(const std::string& name, float seconds) {
        auto it = sounds.find(name);
        if (it != sounds.end() && std::isfinite(seconds)) it->second.cooldown = std::max(0.0f, seconds);
    }

    static int GetVoiceCount(const std::string& name) {
        auto it = sounds.find(name);
        return it == sounds.end() ? 0 : (int)it->second.voices.size();
    }

    static int GetPlayingVoices(const std::string& name) {
        auto it = sounds.find(name);
        if (it == sounds.end()) return 0;
        return (int)std::count_if(it->second.voices.begin(), it->second.voices.end(),
                                  [](const Sound& s) { return IsSoundPlaying(s); });
    }

    static void StopSound(const std::string& name) {
        auto it = sounds.find(name);
        if (it == sounds.end()) return;
        for (auto& voice : it->second.voices) ::StopSound(voice);
    }

    static void LoadMusic(const std::string& name, const std::string& filepath) {
        if (music.count(name)) return;
        Music stream = LoadMusicStream(filepath.c_str());
        if (IsMusicValid(stream)) music[name] = {stream, 1.0f};
    }

    static void PlayMusic(const std::string& name, float volume = 1.0f, bool loop = true) {
        auto it = music.find(name);
        if (it != music.end()) {
            it->second.stream.looping = loop;
            if (std::isfinite(volume)) it->second.volume = Clamp(volume, 0.0f, 1.0f);
            ApplyMusicVolume(it->second);
            PlayMusicStream(it->second.stream);
        }
    }

    static void StopMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) StopMusicStream(it->second.stream);
    }

    // Adjust an active stream without restarting its playback cursor.
    static void SetMusicVolume(const std::string& name, float volume) {
        auto it = music.find(name);
        if (it == music.end() || !std::isfinite(volume)) return;
        it->second.volume = Clamp(volume, 0.0f, 1.0f);
        ApplyMusicVolume(it->second);
    }

    static void PauseMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) PauseMusicStream(it->second.stream);
    }

    static void ResumeMusic(const std::string& name) {
        auto it = music.find(name);
        if (it != music.end()) ResumeMusicStream(it->second.stream);
    }

    static void UpdateMusic() {
        for (auto& pair : music) UpdateMusicStream(pair.second.stream);
    }

    static void SetMasterVolume(float v) {
        if (!std::isfinite(v)) return;
        masterVolume = Clamp(v, 0.0f, 1.0f);
        if (IsAudioDeviceReady()) ::SetMasterVolume(masterVolume);
    }
    static float GetMasterVolume() { return masterVolume; }

    // Channel volumes multiply per-play (effects) and per-stream (music) volumes, so a
    // settings menu can scale either group without touching individual calls.
    static void SetChannelVolume(AudioChannel channel, float v) {
        if (!std::isfinite(v)) return;
        (channel == AudioChannel::Sfx ? sfxVolume : musicVolume) = Clamp(v, 0.0f, 1.0f);
        if (channel == AudioChannel::Music)
            for (auto& pair : music) ApplyMusicVolume(pair.second);
    }
    static float GetChannelVolume(AudioChannel channel) {
        return channel == AudioChannel::Sfx ? sfxVolume : musicVolume;
    }

private:
    static constexpr int kMaxVoices = 16;
    struct SoundEntry {
        std::vector<Sound> voices; // voices[0] owns the samples; the rest are aliases
        size_t next = 0;
        float cooldown = 0;
        double lastPlay = -1e9;
    };
    struct MusicEntry {
        Music stream;
        float volume;
    };

    static double Now() {
        using namespace std::chrono;
        return duration<double>(steady_clock::now().time_since_epoch()).count();
    }
    static void Release(SoundEntry& entry) {
        for (size_t i = 1; i < entry.voices.size(); ++i) UnloadSoundAlias(entry.voices[i]);
        if (!entry.voices.empty()) UnloadSound(entry.voices[0]);
        entry.voices.clear();
    }
    static void ApplyMusicVolume(MusicEntry& entry) { ::SetMusicVolume(entry.stream, entry.volume * musicVolume); }

    static inline std::unordered_map<std::string, SoundEntry> sounds;
    static inline std::unordered_map<std::string, MusicEntry> music;
    static inline float masterVolume = 1.0f;
    static inline float sfxVolume = 1.0f;
    static inline float musicVolume = 1.0f;
};

}
