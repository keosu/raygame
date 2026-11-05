#pragma once

#include "RayGame.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace UQ {

using namespace RayGame;

constexpr const char* kVersion = "v0.1"; // shown on the title page; keep in step with the Android versionName

constexpr int kCanvasW = 1280;
constexpr int kCanvasH = 720;

inline Color Mix(Color a, Color b, float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
            (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}
inline Color Darken(Color c, float k) {
    Color d = Mix(c, BLACK, k);
    d.a = c.a;
    return d;
}
inline Color Lighten(Color c, float k) {
    Color d = Mix(c, WHITE, k);
    d.a = c.a;
    return d;
}
inline Color WithAlpha(Color c, float a) {
    c.a = (unsigned char)(Clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}
inline Color MulAlpha(Color c, float a) {
    c.a = (unsigned char)(c.a * Clamp(a, 0.0f, 1.0f));
    return c;
}

inline Vector2 V(float x, float y) { return {x, y}; }
inline Vector2 Add(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vector2 Sub(Vector2 a, Vector2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vector2 Mul(Vector2 a, float s) { return {a.x * s, a.y * s}; }
inline float Len(Vector2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }
inline Vector2 Dir(float radians) { return {std::cos(radians), std::sin(radians)}; }
inline Vector2 Rotate(Vector2 v, float radians) {
    float c = std::cos(radians), s = std::sin(radians);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}
inline float Approach(float value, float target, float step) {
    return value < target ? std::min(target, value + step) : std::max(target, value - step);
}
inline float EaseOut(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return 1 - (1 - t) * (1 - t);
}
inline float EaseInOut(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

// Deterministic generator for scenery and simulation; fights must replay identically for the
// balance report, so nothing in the model touches raylib's global random sequence.
struct Rng {
    std::uint32_t state;
    explicit Rng(std::uint32_t seed = 1) : state(seed ? seed : 1) {}
    std::uint32_t Next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float Float() { return (Next() & 0xFFFFFF) / float(0x1000000); }
    float Range(float a, float b) { return a + (b - a) * Float(); }
    int Int(int a, int b) { return a + (int)(Next() % (std::uint32_t)(b - a + 1)); }
    bool Chance(float p) { return Float() < p; }
};

inline float Rand(float a, float b) { return RayGame::Random(a, b); }

inline float RealTime() { return (float)GameTime::UnscaledTime(); }

namespace Palette {
inline Color ink = {8, 12, 26, 255};
inline Color panel = {14, 22, 44, 236};
inline Color panelLight = {30, 44, 78, 255};
inline Color line = {82, 112, 170, 150};
inline Color text = {234, 240, 252, 255};
inline Color muted = {150, 166, 200, 255};
inline Color gold = {255, 204, 84, 255};
inline Color red = {236, 56, 64, 255};
inline Color blue = {70, 150, 255, 255};
inline Color timer = {70, 190, 255, 255};
inline Color green = {104, 228, 140, 255};
inline Color cyan = {110, 220, 255, 255};
inline Color orange = {255, 140, 50, 255};
} // namespace Palette

} // namespace UQ
