#include "Assets.h"
#include <sstream>

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
    {"defeat", 1}, {"click", 2}, {"skill", 1}, {"jet", 1}, {"fusion", 1}, {"error", 1}, {"split", 2}, {"shield", 2},
    {"teleport", 2}};
} // namespace

void Load() {
    for (const auto& s : kSounds) {
        AudioManager::LoadSound(s.name, AssetPath(std::string("audio/") + s.name + ".wav"), s.voices);
        AudioManager::SetSoundCooldown(s.name, 0.06f);
    }
    AudioManager::LoadMusic("menu", AssetPath("audio/bgm_menu.ogg"));
    AudioManager::LoadMusic("battle", AssetPath("audio/bgm_battle.ogg"));
    ApplySettings();
}

namespace {
float effectScale = 1;
} // namespace

void Play(const char* name, float volume, float pitchJitter) {
    float pitch = 1.0f + (pitchJitter > 0 ? Rand(-pitchJitter, pitchJitter) : 0.0f);
    AudioManager::PlaySound(name, volume * effectScale, pitch);
}

void SetEffectScale(float scale) { effectScale = scale; }

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
    } catch (...) { return std::filesystem::path("progress.sav"); }
}
} // namespace

void Load() {
    store.Load(SavePath());
    settings.sfx = store.GetBool("sfx", true);
    settings.music = store.GetBool("music", true);
    settings.fullscreen = store.GetBool("fullscreen", false);
    settings.skin = store.GetString("skin", "");
    const char* legacy[] = {"tokyo", "fuji", "nebula"}; // saves before level files used indexes
    for (int i = 0; i < 3; ++i) {
        std::string old = "stars" + std::to_string(i), key = std::string("stars.") + legacy[i];
        if (!store.Has(old)) continue;
        store.SetInt(key, std::max(store.GetInt(key, 0), store.GetInt(old, 0)));
        store.Remove(old);
    }
    loaded = true;
}

void Save() {
    store.SetBool("sfx", settings.sfx);
    store.SetBool("music", settings.music);
    store.SetBool("fullscreen", settings.fullscreen);
    store.SetString("skin", settings.skin);
    store.Save(SavePath());
}

std::string StarsKey(int level) {
    const auto& levels = Levels();
    return level >= 0 && level < (int)levels.size() ? "stars." + levels[level].id : std::string();
}

int Stars(int level) {
    std::string key = StarsKey(level);
    return key.empty() ? 0 : std::clamp(store.GetInt(key, 0), 0, 3);
}

void RecordStars(int level, int stars) {
    if (stars > Stars(level) && !StarsKey(level).empty()) store.SetInt(StarsKey(level), stars);
    Save();
}

bool Unlocked(int level) { return level <= 0 || Stars(level - 1) > 0; }

Settings& Options() { return settings; }

int TotalKills() { return store.GetInt("kills", 0); }

void AddKills(int kills) { store.SetInt("kills", TotalKills() + kills); }

std::vector<HeroId> Roster(const LevelDef& level) {
    std::istringstream keys(store.GetString("roster." + level.id, ""));
    std::vector<HeroId> roster;
    for (std::string key; keys >> key;) {
        HeroId id{};
        if (HeroByKey(key, id) && std::find(roster.begin(), roster.end(), id) == roster.end()) roster.push_back(id);
    }
    if (roster.size() != level.heroes.size()) return level.heroes; // stale or hand-edited save
    return roster;
}

void SetRoster(const LevelDef& level, const std::vector<HeroId>& roster) {
    std::string keys;
    for (HeroId id : roster) keys += std::string(keys.empty() ? "" : " ") + HeroInfo(id).key;
    store.SetString("roster." + level.id, keys);
    Save();
}

float SurvivalBestTime() { return store.GetInt("survival.time", 0) / 10.0f; }
int SurvivalBestWave() { return store.GetInt("survival.wave", 0); }

bool RecordSurvival(float seconds, int wave) {
    bool best = seconds > SurvivalBestTime();
    if (best) store.SetInt("survival.time", (int)(seconds * 10));
    store.SetInt("survival.wave", std::max(SurvivalBestWave(), wave));
    Save();
    return best;
}
} // namespace Progress

} // namespace UG
