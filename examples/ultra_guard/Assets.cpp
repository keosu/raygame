#include "Assets.h"

namespace UG {

std::string AssetPath(const std::string& relative) {
#if defined(__ANDROID__)
    return relative; // APK assets are addressed from the assets root
#endif
    // Desktop examples keep their own assets in bin/assets/<name>/ next to the shared fonts.
    std::string path = std::string(GetApplicationDirectory()) + "assets/ultra_guard/" + relative;
    if (FileExists(path.c_str())) return path;
    return "assets/ultra_guard/" + relative;
}

// ---------------------------------------------------------------- fonts

namespace Fonts {
namespace {
FontHandle body, bold;
Font fallback{};
bool fallbackReady = false;

FontHandle LoadOne(const char* name, const char* file, int size, const std::vector<int>& points) {
    ResourceManager::SetFontOptions(name, {true, TEXTURE_FILTER_TRILINEAR});
    return ResourceManager::LoadFontResource(name, AssetPath(std::string("fonts/") + file), size, points);
}
} // namespace

void Load() {
    std::string charset;
    FileSystem::ReadAssetText(AssetPath("fonts/charset.txt"), charset);
    auto points = ResourceManager::CodepointsFromText(charset);
    body = LoadOne("ug-body", "UltraSC-Medium.ttf", 44, points);
    bold = LoadOne("ug-bold", "UltraSC-Bold.ttf", 72, points);
    UI::font = body;
    UI::headingFont = bold;
}

const Font& Body() {
    if (auto* f = body.Get()) return *f;
    if (!fallbackReady) { fallback = GetFontDefault(); fallbackReady = true; }
    return fallback;
}

const Font& Bold() {
    if (auto* f = bold.Get()) return *f;
    return Body();
}
} // namespace Fonts

// ---------------------------------------------------------------- audio

namespace Sfx {
namespace {
std::string currentMusic;
// Rapid combat sounds get extra voices so they overlap instead of cutting each other off.
constexpr struct { const char* name; int voices; } kSounds[] = {
    {"beam", 3}, {"slugger", 3}, {"snipe", 2}, {"pulse", 2}, {"fire", 3}, {"zero", 3}, {"hit", 4}, {"explode", 4},
    {"coin", 2}, {"build", 1}, {"upgrade", 1}, {"sell", 1}, {"wave", 1}, {"boss", 1}, {"leak", 2}, {"victory", 1},
    {"defeat", 1}, {"click", 2}, {"skill", 1}, {"jet", 1}, {"error", 1}, {"split", 2}, {"shield", 2}, {"teleport", 2}};
} // namespace

void Load() {
    for (const auto& s : kSounds) {
        AudioManager::LoadSound(s.name, AssetPath(std::string("audio/") + s.name + ".wav"), s.voices);
        AudioManager::SetSoundCooldown(s.name, 0.06f);
    }
    AudioManager::LoadMusic("menu", AssetPath("audio/bgm_menu.wav"));
    AudioManager::LoadMusic("battle", AssetPath("audio/bgm_battle.wav"));
    ApplySettings();
}

void Play(const char* name, float volume, float pitchJitter) {
    float pitch = 1.0f + (pitchJitter > 0 ? Rand(-pitchJitter, pitchJitter) : 0.0f);
    AudioManager::PlaySound(name, volume, pitch);
}

void PlayMusic(const char* name) {
    if (currentMusic == name) return;
    if (!currentMusic.empty()) AudioManager::StopMusic(currentMusic);
    currentMusic = name;
    AudioManager::PlayMusic(currentMusic, 0.42f, true);
}

void StopMusic() {
    if (!currentMusic.empty()) AudioManager::StopMusic(currentMusic);
    currentMusic.clear();
}

void ApplySettings() {
    AudioManager::SetChannelVolume(AudioChannel::Sfx, Progress::Options().sfx ? 1.0f : 0.0f);
    AudioManager::SetChannelVolume(AudioChannel::Music, Progress::Options().music ? 1.0f : 0.0f);
}
} // namespace Sfx
// ---------------------------------------------------------------- progress

namespace Progress {
namespace {
SaveSystem store;
Settings settings;
bool loaded = false;

std::filesystem::path SavePath() {
    try {
        return FileSystem::UserDataDirectory("raygame.ultra_guard") / "progress.sav";
    } catch (...) {
        return std::filesystem::path("progress.sav");
    }
}
} // namespace

void Load() {
    store.Load(SavePath());
    settings.sfx = store.GetBool("sfx", true);
    settings.music = store.GetBool("music", true);
    settings.fullscreen = store.GetBool("fullscreen", false);
    loaded = true;
}

void Save() {
    store.SetBool("sfx", settings.sfx);
    store.SetBool("music", settings.music);
    store.SetBool("fullscreen", settings.fullscreen);
    store.Save(SavePath());
}

int Stars(int level) { return std::clamp(store.GetInt("stars" + std::to_string(level), 0), 0, 3); }

void RecordStars(int level, int stars) {
    if (stars > Stars(level)) store.SetInt("stars" + std::to_string(level), stars);
    Save();
}

bool Unlocked(int level) { return level <= 0 || Stars(level - 1) > 0; }

Settings& Options() { return settings; }

int TotalKills() { return store.GetInt("kills", 0); }

void AddKills(int kills) { store.SetInt("kills", TotalKills() + kills); }
} // namespace Progress

} // namespace UG
