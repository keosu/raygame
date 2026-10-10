#include "Profile.h"

namespace Forge {
Profile::Profile(std::filesystem::path path) : directory(std::move(path)) {}
void Profile::Load() {
    settings = {};
    active = false;
    completed = false;
    level = 0;
    checkpoint = -1;
    deaths = 0;
    unlocked = 0;
    collected = 0;
    elapsed = 0;
    best = {};
    recovered = false;
    RayGame::SaveSystem save;
    if (save.Load(directory / "settings.sav")) {
        settings.language = ParseLanguage(save.GetString("language", "zh-CN"));
        settings.music = Clamp(save.GetFloat("music", 0.45f), 0.0f, 1.0f);
        settings.sfx = Clamp(save.GetFloat("sfx", 0.75f), 0.0f, 1.0f);
        settings.shake = save.GetBool("shake", true);
        settings.fullscreen = save.GetBool("fullscreen", false);
        int fps = save.GetInt("fps", settings.fps);
        if (fps == 60 || fps == 120 || fps == 144)
            settings.fps = fps;
    }
    if (save.Load(directory / "campaign.sav") && save.GetInt("version") == 1) {
        level = save.GetInt("level", 0);
        checkpoint = save.GetInt("checkpoint", -1);
        if (level < 0 || level > 2 || checkpoint < -1 || checkpoint > 31) {
            active = false;
            return;
        }
        active = save.GetBool("active");
        completed = save.GetBool("completed");
        deaths = std::max(0, save.GetInt("deaths"));
        elapsed = std::max(0.0f, save.GetFloat("elapsed"));
        unlocked = std::max(0, std::min(2, save.GetInt("unlocked")));
        try {
            collected = std::stoull(save.GetString("collected", "0"));
        } catch (...) {
            collected = 0;
        }
        for (int i = 0; i < 3; ++i)
            best[i] = std::max(0.0f, save.GetFloat("best" + std::to_string(i)));
        recovered = save.WasRecovered();
    }
}
bool Profile::SaveSettings() const {
    RayGame::SaveSystem save;
    save.SetString("language", LanguageCode(settings.language));
    save.SetFloat("music", settings.music);
    save.SetFloat("sfx", settings.sfx);
    save.SetBool("shake", settings.shake);
    save.SetBool("fullscreen", settings.fullscreen);
    save.SetInt("fps", settings.fps);
    return save.Save(directory / "settings.sav");
}
bool Profile::SaveProgress() const {
    RayGame::SaveSystem save;
    save.SetInt("version", 1);
    save.SetBool("active", active);
    save.SetBool("completed", completed);
    save.SetInt("level", level);
    save.SetInt("checkpoint", checkpoint);
    save.SetString("collected", std::to_string(collected));
    save.SetInt("deaths", deaths);
    save.SetFloat("elapsed", elapsed);
    save.SetInt("unlocked", unlocked);
    for (int i = 0; i < 3; ++i)
        save.SetFloat("best" + std::to_string(i), best[i]);
    return save.Save(directory / "campaign.sav");
}
bool Profile::Record(const World& world) {
    if (world.level.index < 0 || world.level.index >= 3)
        return false;
    active = true;
    level = world.level.index;
    checkpoint = world.checkpoint;
    collected = world.collected;
    deaths = world.deaths;
    elapsed = world.elapsed;
    if (world.complete) {
        auto& value = best[level];
        if (value == 0 || elapsed < value)
            value = elapsed;
        unlocked = std::max(unlocked, std::min(2, level + 1));
        completed = completed || level == 2;
        if (level < 2) {
            ++level;
            checkpoint = -1;
            collected = 0;
            deaths = 0;
            elapsed = 0;
        } else
            active = false;
    }
    return SaveProgress();
}
bool Profile::NewRun() {
    active = true;
    level = 0;
    checkpoint = -1;
    deaths = 0;
    collected = 0;
    elapsed = 0;
    return SaveProgress();
}
} // namespace Forge
