#pragma once

#include "Data.h"

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
    std::string skin; // folder under assets/skins; empty = built-in look
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
// The player's lineup for a level, or the level's recommended one until they pick.
std::vector<HeroId> Roster(const LevelDef& level);
void SetRoster(const LevelDef& level, const std::vector<HeroId>& roster);
float SurvivalBestTime(); // seconds
int SurvivalBestWave();
bool RecordSurvival(float seconds, int wave); // true for a new best time
} // namespace Progress

} // namespace UG
