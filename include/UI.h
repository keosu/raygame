#pragma once

#include "Core.h"
#include "Input.h"

namespace RayGame {

// Immediate-mode UI helpers. Call per-frame during a Scene's Render (screen space).
class RAYGAME_API UI {
public:
    static Font* font;              // optional custom font
    static Color textColor;
    static Color accentColor;
    static Color panelColor;

    static void Label(float x, float y, const std::string& text, int size = 20, Color color = {0,0,0,0}) {
        Color c = (color.a == 0 && color.r == 0 && color.g == 0 && color.b == 0) ? textColor : color;
        DrawTextCustom(text, x, y, size, c);
    }

    static void LabelCentered(float cx, float y, const std::string& text, int size = 20, Color color = {0,0,0,0}) {
        float w = MeasureWidth(text, size);
        Label(cx - w / 2, y, text, size, color);
    }

    static void Panel(Rectangle rect, Color color = {0,0,0,0}) {
        Color c = (color.a == 0 && color.r == 0 && color.g == 0 && color.b == 0) ? panelColor : color;
        DrawRectangleRounded(rect, 0.15f, 8, c);
    }

    static void ProgressBar(Rectangle rect, float value01, Color fill = {0,0,0,0}) {
        Color f = (fill.a == 0 && fill.r == 0 && fill.g == 0 && fill.b == 0) ? accentColor : fill;
        DrawRectangleRounded(rect, 0.4f, 6, Fade(DARKGRAY, 0.6f));
        Rectangle inner = rect;
        inner.width = rect.width * Clamp(value01, 0.0f, 1.0f);
        if (inner.width > 2) DrawRectangleRounded(inner, 0.4f, 6, f);
    }

    // Returns true on click (mouse released inside while pressed inside).
    static bool Button(Rectangle rect, const std::string& label, int size = 20) {
        Vector2 m = Input::GetMousePosition();
        if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            pressPosition = m;
            hasPress = true;
        }
        bool hover = CheckCollisionPointRec(m, rect);
        Color bg = hover ? accentColor : panelColor;
        DrawRectangleRounded(rect, 0.25f, 8, bg);
        DrawRectangleRoundedLines(rect, 0.25f, 8, Fade(accentColor, hover ? 1.0f : 0.4f));
        float w = MeasureWidth(label, size);
        DrawTextCustom(label, rect.x + (rect.width - w) / 2,
                       rect.y + (rect.height - size) / 2, size, textColor);
        bool clicked = hasPress && hover && CheckCollisionPointRec(pressPosition, rect)
                       && Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT);
        if (clicked) hasPress = false;
        return clicked;
    }

private:
    static inline Vector2 pressPosition = {0, 0};
    static inline bool hasPress = false;
    static float MeasureWidth(const std::string& text, int size) {
        if (font && font->texture.id != 0)
            return MeasureTextEx(*font, text.c_str(), (float)size, 1.0f).x;
        return (float)MeasureText(text.c_str(), size);
    }

    static void DrawTextCustom(const std::string& text, float x, float y, int size, Color color) {
        if (font && font->texture.id != 0)
            DrawTextEx(*font, text.c_str(), {x, y}, (float)size, 1.0f, color);
        else
            DrawText(text.c_str(), (int)x, (int)y, size, color);
    }
};

inline Font* UI::font = nullptr;
inline Color UI::textColor = RAYWHITE;
inline Color UI::accentColor = {80, 180, 255, 255};
inline Color UI::panelColor = {30, 30, 40, 220};

}
