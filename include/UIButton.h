#pragma once
#include "Core.h"

namespace RayGame {
enum class ButtonIcon { None, Left, Right, Jump, Dash, Pause, Drop, Sound, Settings, Swap };
struct ButtonStyle {
    Color base{29, 48, 59, 245}, accent{117, 225, 194, 255}, ink{239, 248, 240, 255};
    float radius = 0.28f;
};
struct ButtonVisualState {
    bool hovered = false, pressed = false, selected = false, disabled = false;
};
// Drawing only: input capture belongs to the caller. Shared by UI, menus and touch controls.
class ButtonPainter {
public:
    static void Draw(Rectangle r, ButtonVisualState state = {}, ButtonStyle style = {}) {
        float offset = state.pressed ? 2 : 0;
        DrawRectangleRounded({r.x, r.y + 5, r.width, r.height}, style.radius, 24, {0, 0, 0, 60});
        r.y += offset;
        Color base = state.disabled ? Color{40, 46, 49, 210} : style.base;
        Color rim = state.disabled ? Color{104, 117, 117, 100}
                                   : Fade(style.accent, state.pressed || state.selected ? 0.95f
                                                        : state.hovered                 ? 0.7f
                                                                                        : 0.32f);
        DrawRectangleRounded(r, style.radius, 24, rim);
        Rectangle inner{r.x + 1.5f, r.y + 1.5f, r.width - 3, r.height - 3};
        DrawRectangleRounded(inner, style.radius, 24, base);
        if (!state.disabled && (state.hovered || state.selected || state.pressed))
            DrawRectangleRounded(inner, style.radius, 24,
                                 Fade(style.accent, state.pressed ? 0.23f : 0.11f));
        if (!state.pressed) {
            Rectangle shine{r.x + r.width * 0.14f, r.y + 3, r.width * 0.72f, 2};
            DrawRectangleRounded(shine, 1, 12, Fade(WHITE, state.disabled ? 0.04f : 0.18f));
        }
    }
    static void Icon(ButtonIcon icon, Vector2 center, float size, Color color) {
        auto p = [&](float x, float y) {
            return Vector2{center.x + x * size, center.y + y * size};
        };
        auto line = [&](float x, float y, float a, float b) {
            DrawLineEx(p(x, y), p(a, b), std::max(2.0f, size * 0.09f), color);
        };
        auto triangle = [&](Vector2 a, Vector2 b, Vector2 c) {
            if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
                std::swap(b, c);
            DrawTriangle(a, b, c, color);
        };
        if (icon == ButtonIcon::Left || icon == ButtonIcon::Right) {
            float direction = icon == ButtonIcon::Left ? -1.0f : 1.0f;
            line(-0.26f * direction, -0.3f, 0.12f * direction, 0);
            line(0.12f * direction, 0, -0.26f * direction, 0.3f);
        } else if (icon == ButtonIcon::Jump || icon == ButtonIcon::Drop) {
            float direction = icon == ButtonIcon::Jump ? 1.0f : -1.0f;
            line(0, 0.3f * direction, 0, -0.28f * direction);
            line(-0.24f, -0.04f * direction, 0, -0.28f * direction);
            line(0, -0.28f * direction, 0.24f, -0.04f * direction);
            line(-0.3f, 0.4f * direction, 0.3f, 0.4f * direction);
        } else if (icon == ButtonIcon::Dash) {
            triangle(p(-0.02f, -0.4f), p(-0.23f, 0.06f), p(0.18f, -0.03f));
            triangle(p(0.02f, 0.4f), p(0.23f, -0.06f), p(-0.18f, 0.03f));
            line(-0.4f, -0.14f, -0.26f, -0.14f);
            line(-0.44f, 0.14f, -0.3f, 0.14f);
        } else if (icon == ButtonIcon::Pause) {
            for (float x : {-0.16f, 0.16f})
                DrawRectangleRounded({center.x + (x - 0.055f) * size, center.y - size * 0.3f,
                                      size * 0.11f, size * 0.6f},
                                     0.5f, 12, color);
        } else if (icon == ButtonIcon::Swap) {
            line(-0.3f, -0.15f, 0.3f, -0.15f);
            line(0.15f, -0.3f, 0.3f, -0.15f);
            line(-0.3f, 0.15f, 0.3f, 0.15f);
            line(-0.15f, 0.3f, -0.3f, 0.15f);
        } else if (icon == ButtonIcon::Settings) {
            DrawRing(center, size * 0.14f, size * 0.28f, 0, 360, 64, color);
            for (int i = 0; i < 8; ++i) {
                float a = i * PI / 4;
                line(std::cos(a) * 0.22f, std::sin(a) * 0.22f, std::cos(a) * 0.38f,
                     std::sin(a) * 0.38f);
            }
        } else if (icon == ButtonIcon::Sound) {
            triangle(p(-0.25f, -0.12f), p(0.02f, -0.3f), p(0.02f, 0.3f));
            DrawRectangleRec(
                {center.x - size * 0.33f, center.y - size * 0.12f, size * 0.15f, size * 0.24f},
                color);
            DrawRing(center, size * 0.23f, size * 0.27f, -55, 55, 32, color);
            DrawRing(center, size * 0.36f, size * 0.4f, -55, 55, 32, color);
        }
    }
};
} // namespace RayGame
