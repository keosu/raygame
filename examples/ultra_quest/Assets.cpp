#include "Assets.h"
#include <sstream>
#include <unordered_map>

namespace UQ {

std::string AssetPath(const std::string& relative) {
#if defined(__ANDROID__)
    return relative; // APK assets are addressed from the assets root
#endif
    // Desktop examples keep their own assets in bin/assets/<name>/ next to the shared fonts.
    std::string path = std::string(GetApplicationDirectory()) + "assets/ultra_quest/" + relative;
    if (FileExists(path.c_str())) return path;
    return "assets/ultra_quest/" + relative;
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
    body = LoadOne("uq-body", "QuestSC-Medium.ttf", 44, points);
    bold = LoadOne("uq-bold", "QuestSC-Bold.ttf", 72, points);
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
constexpr struct { const char* name; int voices; } kSounds[] = {
    {"punch", 4}, {"kick", 3}, {"swing", 3}, {"guard", 3}, {"parry", 2}, {"dash", 2}, {"jump", 2},
    {"land", 2}, {"stomp", 2}, {"roar", 1}, {"crash", 2}, {"rock", 3}, {"charge", 1}, {"beam", 1},
    {"explode", 2}, {"timer", 1}, {"alert", 1}, {"quake", 1}, {"talk", 3}, {"click", 2}, {"clue", 1},
    {"transform", 1}, {"victory", 1}, {"defeat", 1}, {"hurt", 3}};
} // namespace

void Load() {
    for (const auto& s : kSounds) {
        AudioManager::LoadSound(s.name, AssetPath(std::string("audio/") + s.name + ".wav"), s.voices);
        AudioManager::SetSoundCooldown(s.name, 0.05f);
    }
    AudioManager::LoadMusic("title", AssetPath("audio/bgm_title.ogg"));
    AudioManager::LoadMusic("explore", AssetPath("audio/bgm_explore.ogg"));
    AudioManager::LoadMusic("battle", AssetPath("audio/bgm_battle.ogg"));
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

// ---------------------------------------------------------------- sprites

namespace {
// Photos are drawn well below their pixel size, so they get mipmaps against shimmering.
TextureHandle LoadPhoto(const std::string& name, const std::string& path) {
    if (auto existing = ResourceManager::GetTexture(name)) return existing;
    Image image = LoadImage(path.c_str());
    if (!IsImageValid(image)) return {};
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto handle = ResourceManager::CreateTextureFromImage(name, image, true);
    UnloadImage(image);
    if (handle) SetTextureWrap(*handle, TEXTURE_WRAP_CLAMP);
    return handle;
}
} // namespace

namespace Poses {
namespace {
std::unordered_map<std::string, Pose> poses;
}

void Load() {
    std::string manifest;
    FileSystem::ReadAssetText(AssetPath("poses/poses.txt"), manifest);
    std::istringstream lines(manifest);
    for (std::string line; std::getline(lines, line);) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        std::string key;
        Pose pose;
        if (!(fields >> key >> pose.anchor >> pose.facing)) continue;
        pose.texture = LoadPhoto("uq-pose-" + key, AssetPath("poses/" + key + ".png"));
        poses[key] = pose;
    }
}

const Pose* Get(const std::string& key) {
    auto it = poses.find(key);
    return it != poses.end() && it->second.texture ? &it->second : nullptr;
}
} // namespace Poses

TextureHandle Avatar(const std::string& key) {
    return LoadPhoto("uq-avatar-" + key, AssetPath("avatars/" + key + ".png"));
}

// ---------------------------------------------------------------- progress

namespace Progress {
namespace {
SaveSystem store;
Settings settings;

std::filesystem::path SavePath() {
    try {
        return FileSystem::UserDataDirectory("raygame.ultra_quest") / "progress.sav";
    } catch (...) { return std::filesystem::path("progress.sav"); }
}
} // namespace

void Load() {
    store.Load(SavePath());
    settings.sfx = store.GetBool("sfx", true);
    settings.music = store.GetBool("music", true);
    settings.fullscreen = store.GetBool("fullscreen", false);
    settings.shake = store.GetBool("shake", true);
    settings.touchControls = store.GetBool("touch", false);
}

void Save() {
    store.SetBool("sfx", settings.sfx);
    store.SetBool("music", settings.music);
    store.SetBool("fullscreen", settings.fullscreen);
    store.SetBool("shake", settings.shake);
    store.SetBool("touch", settings.touchControls);
    store.Save(SavePath());
}

Settings& Options() { return settings; }

bool ChapterCleared() { return store.GetBool("chapter1.cleared", false); }

bool RecordFight(int rank, float seconds) {
    bool best = rank > BestRank() || (rank == BestRank() && (BestTime() <= 0 || seconds < BestTime()));
    store.SetBool("chapter1.cleared", true);
    if (best) {
        store.SetInt("chapter1.rank", rank);
        store.SetInt("chapter1.time", (int)(seconds * 10));
    }
    Save();
    return best;
}

int BestRank() { return ChapterCleared() ? std::clamp(store.GetInt("chapter1.rank", 0), 0, 4) : -1; }
float BestTime() { return store.GetInt("chapter1.time", 0) / 10.0f; }

bool SkipIntro() { return store.GetBool("chapter1.explored", false); }
void SetSkipIntro() {
    store.SetBool("chapter1.explored", true);
    Save();
}
} // namespace Progress

} // namespace UQ
