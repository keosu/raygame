#pragma once
#include <raygame/SaveSystem.h>
#include "World.h"
#include <array>

namespace Forge {
struct Settings {
    float music = 0.45f, sfx = 0.75f;
    bool shake = true, fullscreen = false;
    int fps = 120;
};
class Profile {
public:
    std::filesystem::path directory;
    Settings settings;
    bool active = false, completed = false;
    int level = 0, checkpoint = -1, deaths = 0, unlocked = 0;
    std::uint64_t collected = 0;
    float elapsed = 0;
    std::array<float,3> best{};
    bool recovered = false;
    explicit Profile(std::filesystem::path path);
    void Load();
    bool SaveSettings() const;
    bool SaveProgress() const;
    bool Record(const World& world);
    bool NewRun();
};
}
