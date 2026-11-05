#pragma once

#include "Common.h"

namespace UQ {

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

// Fighter pose sprites from tools/make_poses.py. Each pose is drawn with its feet on the
// ground point; `anchor` is the body centre as a share of the width.
struct Pose {
    TextureHandle texture;
    float anchor = 0.5f;
    int facing = 0; // the way the render looks: -1 left, 0 front, 1 right
};

namespace Poses {
void Load();
const Pose* Get(const std::string& key);
} // namespace Poses

TextureHandle Avatar(const std::string& key);

struct Settings {
    bool sfx = true;
    bool music = true;
    bool fullscreen = false;
    bool shake = true;
    bool touchControls = false; // on-screen buttons on desktop; Android always shows them
};

namespace Progress {
void Load();
void Save();
Settings& Options();
bool ChapterCleared();
// Ranks are 0..4 for C, B, A, S, SS; returns true for a new best.
bool RecordFight(int rank, float seconds);
int BestRank();   // -1 before the first win
float BestTime(); // seconds, 0 before the first win
bool SkipIntro(); // the exploration part was completed once
void SetSkipIntro();
} // namespace Progress

} // namespace UQ
