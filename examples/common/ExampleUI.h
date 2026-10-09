#pragma once

#include "RayGame.h"

namespace ExampleUI {

inline constexpr Color muted = {143, 163, 193, 255};
inline constexpr Color cyan = {97, 218, 255, 255};
inline constexpr Color pink = {255, 115, 173, 255};
inline constexpr Color gold = {255, 207, 112, 255};

inline void ConfigureLogging() {
#ifdef RAYGAME_QUIET_EXAMPLE
    SetTraceLogLevel(LOG_NONE);
#endif
}

inline void Triangle(Vector2 a, Vector2 b, Vector2 c, Color color) {
    // raylib's 2D triangles still use face culling; accept either winding here.
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0) std::swap(b, c);
    DrawTriangle(a, b, c, color);
}

inline Font* LoadFont(const std::string& file, int size) {
    std::string path = std::string(GetApplicationDirectory()) + "assets/fonts/" + file;
    if (!FileExists(path.c_str())) path = "assets/fonts/" + file;
    if (!FileExists(path.c_str())) return nullptr;
    auto* font = RayGame::ResourceManager::LoadFontResource(file, path, size);
    if (font) SetTextureFilter(font->texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

inline void Initialize(Color accent = cyan) {
    RayGame::UI::font = LoadFont("Barlow-Medium.ttf", 64);
    RayGame::UI::headingFont = LoadFont("Rajdhani-SemiBold.ttf", 96);
    RayGame::UI::accentColor = accent;
}

inline void Header(float width, const std::string& title, const std::string& subtitle) {
    using RayGame::UI;
    UI::Panel({16, 14, width - 32, 60});
    DrawRectangle(16, 29, 3, 30, UI::accentColor);
    UI::Label(32, 21, subtitle, 12, muted);
    UI::Title(30, 35, title, 30);
}

inline void Stat(float right, const std::string& label, const std::string& value, Color color = cyan) {
    RayGame::UI::LabelRight(right, 21, label, 12, muted);
    RayGame::UI::TitleRight(right, 36, value, 30, color);
}

inline void Lives(float x, int count) {
    RayGame::UI::Label(x, 21, "SHIELDS", 12, muted);
    for (int i = 0; i < 3; ++i) {
        Color color = i < count ? pink : Color{50, 61, 84, 255};
        float px = x + i * 23;
        DrawRectangleRounded({px, 44, 16, 9}, 0.6f, 8, color);
        if (i < count) DrawRectangleRounded({px - 2, 42, 20, 13}, 0.6f, 8, Fade(color, 0.12f));
    }
}

inline void Footer(float width, float height, const std::string& controls) {
    RayGame::UI::Panel({16, height - 40, width - 32, 26}, {9, 15, 28, 224});
    RayGame::UI::Label(28, height - 35, controls, 16, muted);
    RayGame::UI::LabelRight(width - 28, height - 35, "P  PAUSE     F11  FULLSCREEN", 16, muted);
}

inline bool Result(float width, float height, const std::string& title, int score, bool victory = false) {
    using RayGame::UI;
    DrawRectangle(0, 0, (int)width, (int)height, {4, 7, 16, 200});
    float x = width / 2 - 220, y = height / 2 - 130;
    UI::Panel({x, y, 440, 260}, {12, 19, 34, 250});
    Color accent = victory ? Color{123, 238, 174, 255} : pink;
    DrawRectangleRounded({x + 170, y, 100, 3}, 1, 8, accent);
    UI::LabelCentered(width / 2, y + 27, victory ? "MISSION COMPLETE" : "RUN COMPLETE", 14, muted);
    UI::TitleCentered(width / 2, y + 51, title, 52, accent);
    UI::LabelCentered(width / 2, y + 120, TextFormat("FINAL SCORE   %06d", score), 24);
    return UI::Button({x + 96, y + 186, 248, 44}, "PLAY AGAIN  /  R", 20);
}

}
