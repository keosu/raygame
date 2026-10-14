#include "Skins.h"
#include "Art.h"
#include "Assets.h"
#include "Map.h"
#include <cstdlib>
#include <map>
#include <sstream>

namespace UG::Skins {

namespace {
struct PaletteSlot {
    const char* name;
    Color* color;
    Color base;
};

// Captured before any skin changes the palette, so switching back restores the defaults.
std::vector<PaletteSlot>& Slots() {
    static std::vector<PaletteSlot> slots = {
        {"ink", &Palette::ink, Palette::ink},          {"panel", &Palette::panel, Palette::panel},
        {"panelLight", &Palette::panelLight, Palette::panelLight},
        {"line", &Palette::line, Palette::line},       {"text", &Palette::text, Palette::text},
        {"muted", &Palette::muted, Palette::muted},    {"gold", &Palette::gold, Palette::gold},
        {"red", &Palette::red, Palette::red},          {"silver", &Palette::silver, Palette::silver},
        {"timer", &Palette::timer, Palette::timer},    {"green", &Palette::green, Palette::green},
        {"cyan", &Palette::cyan, Palette::cyan},       {"purple", &Palette::purple, Palette::purple},
    };
    return slots;
}

struct State {
    std::vector<Skin> skins;
    int current = 0;
    std::array<KaijuSprite, kEnemyCount> kaiju{};
    std::array<bool, kEnemyCount> hasKaiju{};
    std::vector<std::string> textures; // names to unload on the next switch
    std::map<std::string, Texture2D> grounds;
    bool mapPaths = true, mapDecor = true;
    std::vector<std::string> indexErrors; // from skins.txt
    std::vector<std::string> errors;      // indexErrors plus the current skin's skin.txt
};
State state;

std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> Tokens(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::string> out;
    for (std::string t; in >> t;) out.push_back(t);
    return out;
}

bool Number(const std::string& s, float& out) {
    char* end = nullptr;
    out = std::strtof(s.c_str(), &end);
    return !s.empty() && end && *end == '\0' && std::isfinite(out);
}

// Non-empty, non-comment lines with their 1-based numbers; strips a UTF-8 BOM.
std::vector<std::pair<int, std::string>> Lines(const std::string& text) {
    std::vector<std::pair<int, std::string>> out;
    std::istringstream in(text);
    int number = 0;
    for (std::string raw; std::getline(in, raw);) {
        if (++number == 1 && raw.rfind("\xEF\xBB\xBF", 0) == 0) raw = raw.substr(3);
        std::string line = Trim(raw);
        if (!line.empty() && line[0] != '#') out.push_back({number, line});
    }
    return out;
}

std::string Dir(int index) {
    return index > 0 && index < (int)state.skins.size() ? "skins/" + state.skins[index].folder + "/" : "";
}

bool Flag(const std::string& value, bool& out) {
    if (value == "on") out = true;
    else if (value == "off") out = false;
    else return false;
    return true;
}

void Release() {
    for (const auto& name : state.textures) ResourceManager::UnloadTextureNamed(name);
    state.textures.clear();
    state.grounds.clear();
    state.kaiju = {};
    state.hasKaiju = {};
    state.mapPaths = state.mapDecor = true;
    for (auto& slot : Slots()) *slot.color = slot.base;
}

// Mipmapped: sprites and grounds are drawn well below their source size in battle and previews.
Texture2D LoadNamed(const std::string& name, const std::string& path) {
    Image image = LoadImage(path.c_str());
    if (!IsImageValid(image)) return {};
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto handle = ResourceManager::CreateTextureFromImage(name, image, true);
    UnloadImage(image);
    if (!handle) return {};
    state.textures.push_back(name);
    return *handle;
}

// Reads skin.txt into the palette and per-kaiju sprite options.
void ParseSkinFile(int index, std::array<KaijuSprite, kEnemyCount>& options) {
    std::string file = Dir(index) + "skin.txt", text;
    if (!FileSystem::ReadAssetText(AssetPath(file), text)) return;
    for (const auto& entry : Lines(text)) {
        int number = entry.first;
        auto t = Tokens(entry.second);
        auto fail = [&](const std::string& message) {
            state.errors.push_back(file + ":" + std::to_string(number) + ": " + message);
        };
        if (t[0] == "name") {
            continue; // read by Load for the skin list
        } else if (t[0] == "color") {
            auto slot = std::find_if(Slots().begin(), Slots().end(),
                                     [&](const PaletteSlot& s) { return t.size() > 1 && t[1] == s.name; });
            float v[4] = {0, 0, 0, 255};
            bool ok = slot != Slots().end() && (t.size() == 5 || t.size() == 6);
            for (size_t i = 2; ok && i < t.size(); ++i) ok = Number(t[i], v[i - 2]) && v[i - 2] >= 0 && v[i - 2] <= 255;
            if (!ok) {
                fail("写法为 color 名称 R G B [A]（0-255），名称：ink panel panelLight line text muted gold red "
                     "silver timer green cyan purple");
                continue;
            }
            *slot->color = {(unsigned char)v[0], (unsigned char)v[1], (unsigned char)v[2], (unsigned char)v[3]};
        } else if (t[0] == "kaiju") {
            int id = -1;
            for (int i = 0; i < kEnemyCount; ++i)
                if (t.size() > 1 && t[1] == EnemyInfo((EnemyId)i).key) id = i;
            if (id < 0) {
                fail("未知怪兽：" + (t.size() > 1 ? t[1] : std::string()));
                continue;
            }
            KaijuSprite& o = options[id];
            for (size_t i = 2; i < t.size(); i += 2) {
                float v = 0;
                if (i + 1 >= t.size() || !Number(t[i + 1], v) || v <= 0) {
                    fail("“" + t[i] + "”需要一个正数");
                    break;
                }
                if (t[i] == "frames") o.frames = std::max(1, (int)v);
                else if (t[i] == "fps") o.fps = v;
                else if (t[i] == "size") o.size = v;
                else if (t[i] == "feet" && v <= 1) o.feet = v;
                else {
                    fail("无法识别“" + t[i] + "”，可用：frames fps size feet(0-1)");
                    break;
                }
            }
        } else if (t[0] == "map_paths" || t[0] == "map_decor") {
            if (t.size() != 2 || !Flag(t[1], t[0] == "map_paths" ? state.mapPaths : state.mapDecor))
                fail(t[0] + " 应为 on 或 off");
        } else {
            fail("未知设置“" + t[0] + "”");
        }
    }
}

void Apply(int index) {
    Release();
    state.errors = state.indexErrors;
    state.current = index;
    std::array<KaijuSprite, kEnemyCount> options{};
    if (index > 0) ParseSkinFile(index, options);
    for (int i = 0; i < kEnemyCount; ++i) {
        std::string key = EnemyInfo((EnemyId)i).key, path = Find("kaiju/" + key + ".png");
        if (path.empty()) continue;
        options[i].texture = LoadNamed("skin-kaiju-" + key, path);
        if (!options[i].texture.id) continue;
        state.kaiju[i] = options[i];
        state.hasKaiju[i] = true;
    }
    for (const auto& e : state.errors) Diagnostics::Write(LogLevel::Warning, "Skin: " + e);
    Art::ReloadPortraits();
}
} // namespace

void Load(const std::string& folder) {
    state.skins = {{"", "经典"}};
    state.indexErrors.clear();
    Slots();
    std::string index;
    if (FileSystem::ReadAssetText(AssetPath("skins/skins.txt"), index)) {
        for (const auto& [number, line] : Lines(index)) {
            Skin skin{line, line};
            std::string text;
            if (!FileSystem::ReadAssetText(AssetPath("skins/" + line + "/skin.txt"), text)) {
                state.indexErrors.push_back("skins/skins.txt:" + std::to_string(number) + ": 找不到 skins/" + line +
                                            "/skin.txt");
                continue;
            }
            for (const auto& [n, l] : Lines(text)) {
                size_t split = l.find_first_of(" \t");
                if (l.substr(0, split) == "name" && split != std::string::npos) skin.name = Trim(l.substr(split));
            }
            state.skins.push_back(skin);
        }
    }
    // With no saved choice, fall back to the photo skin so the real pictures are
    // what players see first; "经典" (index 0) stays selectable and keeps the drawn art.
    std::string wanted = folder.empty() ? Progress::Options().skin : folder;
    if (wanted.empty()) wanted = "real";
    int chosen = 0;
    for (int i = 1; i < (int)state.skins.size(); ++i)
        if (state.skins[i].folder == wanted) chosen = i;
    Apply(chosen);
}

const std::vector<Skin>& All() { return state.skins; }
int Current() { return state.current; }

void Select(int index, bool save) {
    if (index < 0 || index >= (int)state.skins.size()) return;
    Apply(index);
    if (!save) return;
    Progress::Options().skin = state.skins[index].folder;
    Progress::Save();
}

std::string Find(const std::string& relative) {
    if (state.current <= 0) return "";
    std::string path = AssetPath(Dir(state.current) + relative);
    return FileSystem::AssetExists(path) ? path : "";
}

const KaijuSprite* Kaiju(EnemyId id) {
    int i = (int)id;
    return i >= 0 && i < kEnemyCount && state.hasKaiju[i] ? &state.kaiju[i] : nullptr;
}

Texture2D MapGround(const LevelDef& level) {
    static const char* themes[] = {"city", "mountain", "nebula"};
    for (const std::string& name : {level.id, std::string(themes[(int)level.theme])}) {
        std::string path = Find("maps/" + name + ".png");
        if (path.empty()) continue;
        auto cached = state.grounds.find(name);
        if (cached != state.grounds.end()) return cached->second;
        Texture2D texture = LoadNamed("skin-map-" + name, path);
        state.grounds[name] = texture;
        if (texture.id) return texture;
    }
    return {};
}

bool MapPaths() { return state.mapPaths; }
bool MapDecor() { return state.mapDecor; }
const std::vector<std::string>& Errors() { return state.errors; }

bool ExportTemplate(const std::string& dir) {
    bool ok = true;
    for (const char* sub : {"", "/avatars", "/kaiju", "/maps"}) MakeDirectory((dir + sub).c_str());
    auto save = [&](Image& image, const std::string& path) {
        ok &= ExportImage(image, path.c_str());
        UnloadImage(image);
    };
    std::string text = "# 皮肤说明（UTF-8）。复制整个目录到 assets/skins/<目录名>/，并把目录名加到 assets/skins/skins.txt。\n"
                       "# 只保留想替换的文件即可；缺少的图片使用内置画面。修改中文后需运行 tools/make_font.py。\n"
                       "name 我的皮肤\n\n"
                       "# 界面颜色：color 名称 R G B [A]\n";
    for (const auto& slot : Slots())
        text += TextFormat("color %-10s %3d %3d %3d %3d\n", slot.name, slot.base.r, slot.base.g, slot.base.b,
                           slot.base.a);
    text += "\n# avatars/<英雄>.png：正方形头像，头部居中偏上，透明背景。\n"
            "# kaiju/<怪兽>.png：从左到右的动画帧条。frames 帧数，fps 播放速度，size 缩放 1 时的绘制高度（画布像素），\n"
            "#   feet 脚底在图片高度中的位置（0-1）。导出的帧条为 12 帧、每帧 384x320（4 倍），与下列数值对应。\n";
    for (int i = 0; i < kEnemyCount; ++i)
        text += TextFormat("kaiju %-12s frames 12 fps 12 size 80 feet 0.94\n", EnemyInfo((EnemyId)i).key);
    text += "\n# maps/<关卡 id>.png 或 maps/<city|mountain|nebula>.png：地面（20x12 格，导出为 2160x1296）。\n"
            "# 是否在替换的地面上继续绘制内置道路与装饰：\n"
            "map_paths on\nmap_decor on\n";
    ok &= SaveFileText((dir + "/skin.txt").c_str(), text.data());

    for (int i = 0; i < kHeroCount; ++i) {
        Image image = LoadImageFromTexture(Art::Portrait((HeroId)i));
        save(image, dir + "/avatars/" + HeroInfo((HeroId)i).key + ".png");
    }

    constexpr int kFrames = 12, kCellW = 384, kCellH = 320; // 4x: the gallery enlarges kaiju
    for (int i = 0; i < kEnemyCount; ++i) {
        RenderTexture2D target = LoadRenderTexture(kCellW * kFrames, kCellH);
        BeginTextureMode(target);
        ClearBackground(BLANK);
        Art::BeginBakeBlend();
        for (int f = 0; f < kFrames; ++f) {
            Art::KaijuPose pose;
            pose.feet = {kCellW * (f + 0.5f), kCellH * 0.94f};
            pose.scale = 4;
            pose.time = f / 12.0f;
            Art::DrawBuiltinKaiju((EnemyId)i, pose);
        }
        EndBlendMode();
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        save(image, dir + "/kaiju/" + EnemyInfo((EnemyId)i).key + ".png");
        UnloadRenderTexture(target);
    }

    for (const auto& level : Levels()) {
        MapData map;
        map.Build(level);
        RenderTexture2D target = RenderMap(map, 2.0f, true);
        Image image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        save(image, dir + "/maps/" + level.id + ".png");
        UnloadRenderTexture(target);
    }
    Diagnostics::Write(ok ? LogLevel::Info : LogLevel::Error, "Skin template exported to " + dir);
    return ok;
}

} // namespace UG::Skins
