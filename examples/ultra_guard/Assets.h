#pragma once

#include "Common.h"

namespace UG {

std::string AssetPath(const std::string& relative);

namespace Fonts {
void Load();
const Font& Body();
const Font& Bold();
} // namespace Fonts

namespace Sfx {
void Load();
// Repeated triggers of one effect are throttled so dense fights stay readable.
void Play(const char* name, float volume = 1.0f, float pitchJitter = 0.06f);
void PlayMusic(const char* name);
void StopMusic();
void ApplySettings();
} // namespace Sfx

struct Settings {
    bool sfx = true;
    bool music = true;
    bool fullscreen = false;
};

namespace Progress {
void Load();
void Save();
int Stars(int level);
void RecordStars(int level, int stars);
bool Unlocked(int level);
Settings& Options();
int TotalKills();
void AddKills(int kills);
} // namespace Progress

} // namespace UG
