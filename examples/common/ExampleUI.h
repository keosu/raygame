#pragma once

#include "RayGame.h"

namespace ExampleUI {

inline constexpr Color muted = {143, 163, 193, 255};
inline constexpr Color cyan = {97, 218, 255, 255};
inline constexpr Color pink = {255, 115, 173, 255};
inline constexpr Color gold = {255, 207, 112, 255};

inline float Width() { return RayGame::Display::GetWidth(); }
inline float Height() { return RayGame::Display::GetHeight(); }

inline Vector2 Remap(Vector2 position, Vector2 previous, Vector2 current) {
    return {position.x * current.x / previous.x, position.y * current.y / previous.y};
}

inline void ConfigureLogging(const std::string& appId) {
#ifdef RAYGAME_QUIET_EXAMPLE
    constexpr bool console = false;
#else
    constexpr bool console = true;
#endif
    try {
        auto path = RayGame::FileSystem::UserDataDirectory(appId) / "logs/game.log";
        RayGame::Diagnostics::Initialize(path, console);
        RayGame::Diagnostics::InstallRaylibCallback();
        RayGame::Diagnostics::Write(RayGame::LogLevel::Info, "Starting " + appId);
    } catch (...) {
        if (!console) SetTraceLogLevel(LOG_NONE);
    }
}

template<class Game>
int Run(const std::string& appId) {
    ConfigureLogging(appId);
    int result = 0;
    try {
        Game game;
        game.SetCanvasScaleMode(RayGame::CanvasScaleMode::Expand);
        game.Run();
    } catch (const std::exception& error) {
        RayGame::Diagnostics::Write(RayGame::LogLevel::Error, error.what());
        result = 1;
    } catch (...) {
        RayGame::Diagnostics::Write(RayGame::LogLevel::Error, "Unhandled game exception");
        result = 1;
    }
    RayGame::Diagnostics::Shutdown();
    return result;
}

inline void Triangle(Vector2 a, Vector2 b, Vector2 c, Color color) {
    // raylib's 2D triangles still use face culling; accept either winding here.
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0) std::swap(b, c);
    DrawTriangle(a, b, c, color);
}

inline RayGame::FontHandle LoadFont(const std::string& file, int size) {
#if defined(__ANDROID__)
    std::string path = "fonts/" + file;
#else
    std::string path = std::string(GetApplicationDirectory()) + "assets/fonts/" + file;
    if (!FileExists(path.c_str())) path = "assets/fonts/" + file;
#endif
    auto font = RayGame::ResourceManager::LoadFontResource(file, path, size);
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

inline bool TouchInPlayfield() {
    return RayGame::Input::GetTouchCount() > 0 && CheckCollisionPointRec(
        RayGame::Input::GetTouchPosition(0), {16, 84, Width() - 32, Height() - 138});
}

inline bool MoveWithTouch(Vector2& position, float speed) {
    if (!TouchInPlayfield()) return false;
    Vector2 target = RayGame::Input::GetTouchPosition(0);
    target.y -= 56; // Keep the ship visible above the finger.
    Vector2 offset = {target.x - position.x, target.y - position.y};
    float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
    float factor = distance > 0 ? std::min(1.0f, speed * RayGame::GameTime::DeltaTime() / distance) : 0;
    position.x += offset.x * factor; position.y += offset.y * factor;
    return true;
}

inline void Footer(float width, float height, const std::string& controls,
                   const std::string& touchControls = "TAP / DRAG TO PLAY") {
    RayGame::UI::Panel({16, height - 40, width - 32, 26}, {9, 15, 28, 224});
#if defined(__ANDROID__)
    (void)controls;
    RayGame::UI::Label(28, height - 35, touchControls, 16, muted);
#else
    (void)touchControls;
    RayGame::UI::Label(28, height - 35, controls, 16, muted);
    RayGame::UI::LabelRight(width - 28, height - 35, "P  PAUSE     F11  FULLSCREEN", 16, muted);
#endif
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
#if defined(__ANDROID__)
    return UI::Button({x + 96, y + 186, 248, 44}, "PLAY AGAIN", 20);
#else
    return UI::Button({x + 96, y + 186, 248, 44}, "PLAY AGAIN  /  R", 20);
#endif
}

}
