#pragma once

#include "Core.h"
#include "ResourceHandle.h"
#include "Input.h"
#include "UIButton.h"

namespace RayGame {

// Immediate-mode UI helpers. Call per-frame during a Scene's Render (screen space).
class RAYGAME_API UI {
public:
    enum class Align { Left, Center, Right };
    static FontHandle font; // optional custom font
    static inline FontHandle headingFont = nullptr;
    static inline FontHandle unicodeFont = nullptr, unicodeHeadingFont = nullptr;
    static inline float unicodeScale = 1;
    static inline bool inputEnabled = true;
    static Color textColor;
    static Color accentColor;
    static Color panelColor;

    static void ResetFonts() {
        font = nullptr;
        headingFont = nullptr;
        unicodeFont = nullptr;
        unicodeHeadingFont = nullptr;
        unicodeScale = 1;
    }
    static void ResetInteraction() {
        hasPress = false;
        regionsThisFrame.clear();
        regionsLastFrame.clear();
    }

    // Pointer capture lets gameplay ignore clicks aimed at the UI. Panels, buttons and
    // progress bars register themselves; call Capture for custom widgets. GameEngine
    // brackets each Render with BeginFrame/EndFrame. Update tests the current pointer
    // against the last rendered regions, so moving onto a widget captures immediately.
    static void BeginFrame() {
        regionsThisFrame.clear();
    }
    static void EndFrame() {
        regionsLastFrame = regionsThisFrame;
    }
    static void Capture(Rectangle rect) {
        regionsThisFrame.push_back(rect);
    }
    static bool IsMouseOverUI() {
        if (!Input::GetFrame().focused || !Input::IsMouseInCanvas())
            return false;
        Vector2 pointer = Input::GetMousePosition();
        auto contains = [pointer](const std::vector<Rectangle>& regions) {
            return std::any_of(regions.begin(), regions.end(), [pointer](Rectangle rect) {
                return CheckCollisionPointRec(pointer, rect);
            });
        };
        return contains(regionsLastFrame) || contains(regionsThisFrame);
    }

    static void Title(float x, float y, const std::string& text, int size = 36,
                      Color color = RAYWHITE) {
        DrawTextCustom(text, x, y, size, color, true);
    }
    static void TitleCentered(float cx, float y, const std::string& text, int size = 36,
                              Color color = RAYWHITE) {
        Title(cx - MeasureWidth(text, size, true) / 2, y, text, size, color);
    }
    static void LabelRight(float right, float y, const std::string& text, int size = 20,
                           Color color = RAYWHITE) {
        Label(right - MeasureWidth(text, size), y, text, size, color);
    }
    static void TitleRight(float right, float y, const std::string& text, int size = 36,
                           Color color = RAYWHITE) {
        Title(right - MeasureWidth(text, size, true), y, text, size, color);
    }

    static void Label(float x, float y, const std::string& text, int size = 20,
                      Color color = {0, 0, 0, 0}) {
        Color c =
            (color.a == 0 && color.r == 0 && color.g == 0 && color.b == 0) ? textColor : color;
        DrawTextCustom(text, x, y, size, c);
    }

    static void LabelCentered(float cx, float y, const std::string& text, int size = 20,
                              Color color = {0, 0, 0, 0}) {
        float w = MeasureWidth(text, size);
        Label(cx - w / 2, y, text, size, color);
    }

    // Fill, border and corner roundness (0..1, as in DrawRectangleRounded) for themed HUDs.
    static void Panel(Rectangle rect, Color fill, Color border, float roundness = 0.12f) {
        Capture(rect);
        DrawRectangleRounded(rect, roundness, 10, fill);
        if (border.a > 0)
            DrawRectangleRoundedLines(rect, roundness, 10, border);
    }

    // Text at a float size with alignment and an optional outline (`outline` pixels).
    // Uses the same font selection as Label/Title, including the Unicode fonts.
    static void Text(float x, float y, const std::string& text, float size, Color color,
                     Align align = Align::Left, bool heading = false, float outline = 0,
                     Color outlineColor = {0, 0, 0, 255}) {
        auto selected = Pick(text, heading);
        bool unicode = HasUnicode(text) && unicodeFont;
        float drawSize = unicode ? size * unicodeScale : size;
        if (unicode)
            y -= size * 0.18f;
        float width = selected ? MeasureTextEx(*selected, text.c_str(), drawSize, 0.5f).x
                               : (float)MeasureText(text.c_str(), (int)size);
        if (align == Align::Center)
            x -= width / 2;
        else if (align == Align::Right)
            x -= width;
        auto draw = [&](float px, float py, Color c) {
            if (selected)
                DrawTextEx(*selected, text.c_str(), {px, py}, drawSize, 0.5f, c);
            else
                DrawText(text.c_str(), (int)px, (int)py, (int)size, c);
        };
        if (outline > 0)
            for (int i = 0; i < 8; ++i) {
                float a = i * PI / 4;
                draw(x + std::cos(a) * outline, y + std::sin(a) * outline, outlineColor);
            }
        draw(x, y, color);
    }

    static void Panel(Rectangle rect, Color color = {0, 0, 0, 0}) {
        Color c =
            (color.a == 0 && color.r == 0 && color.g == 0 && color.b == 0) ? panelColor : color;
        Capture(rect);
        DrawRectangleRounded(rect, 0.12f, 8, c);
        DrawRectangleRoundedLines(rect, 0.12f, 8, {66, 86, 119, 100});
    }

    static void ProgressBar(Rectangle rect, float value01, Color fill = {0, 0, 0, 0}) {
        Color f = (fill.a == 0 && fill.r == 0 && fill.g == 0 && fill.b == 0) ? accentColor : fill;
        Capture(rect);
        DrawRectangleRounded(rect, 0.4f, 6, Fade(DARKGRAY, 0.6f));
        Rectangle inner = rect;
        inner.width = rect.width * Clamp(value01, 0.0f, 1.0f);
        if (inner.width > 2)
            DrawRectangleRounded(inner, 0.4f, 6, f);
    }

    // Returns true on click (mouse released inside while pressed inside).
    static bool Button(Rectangle rect, const std::string& label, int size = 20) {
        return Button(rect, label, ButtonStyle{panelColor, accentColor, textColor}, size);
    }

    // Themed button; a disabled button still draws and captures the pointer but never clicks.
    static bool Button(Rectangle rect, const std::string& label, const ButtonStyle& style,
                       int size = 20, bool enabled = true) {
        Vector2 m = Input::GetMousePosition();
        bool acceptsInput = enabled && inputEnabled && Input::GetFrame().focused;
        // A disabled background widget must not cancel a foreground menu's press.
        if (!Input::GetFrame().focused)
            hasPress = false;
        if (acceptsInput && Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            pressPosition = m;
            hasPress = true;
        }
        bool hover = acceptsInput && Input::IsMouseInCanvas() && CheckCollisionPointRec(m, rect);
        Capture(rect);
        bool down = hasPress && hover && Input::GetMouseButton(MOUSE_BUTTON_LEFT);
        ButtonPainter::Draw(rect, {hover, down, false, !enabled}, style);
        float w = MeasureWidth(label, size);
        DrawTextCustom(label, rect.x + (rect.width - w) / 2,
                       rect.y + (rect.height - size) / 2 + (down ? 2.0f : 0.0f), size,
                       enabled ? style.ink : Fade(style.ink, 0.45f));
        bool clicked = hasPress && hover && CheckCollisionPointRec(pressPosition, rect) &&
                       Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT);
        if (clicked)
            hasPress = false;
        return clicked;
    }

    static float TextWidth(const std::string& text, int size, bool heading = false) {
        return MeasureWidth(text, size, heading);
    }

private:
    static inline Vector2 pressPosition = {0, 0};
    static inline bool hasPress = false;
    static inline std::vector<Rectangle> regionsThisFrame, regionsLastFrame;
    static float MeasureWidth(const std::string& text, int size, bool heading = false) {
        bool unicode = HasUnicode(text) && unicodeFont;
        auto selected = unicode ? (heading && unicodeHeadingFont ? unicodeHeadingFont : unicodeFont)
                                : (heading && headingFont ? headingFont : font);
        float drawSize = unicode ? size * unicodeScale : static_cast<float>(size);
        if (selected && selected->texture.id != 0)
            return MeasureTextEx(*selected, text.c_str(), drawSize, 0.5f).x;
        return (float)MeasureText(text.c_str(), size);
    }

    static void DrawTextCustom(const std::string& text, float x, float y, int size, Color color,
                               bool heading = false) {
        bool unicode = HasUnicode(text) && unicodeFont;
        auto selected = unicode ? (heading && unicodeHeadingFont ? unicodeHeadingFont : unicodeFont)
                                : (heading && headingFont ? headingFont : font);
        float drawSize = unicode ? size * unicodeScale : static_cast<float>(size);
        if (unicode)
            y -= size * 0.18f;
        if (selected && selected->texture.id != 0)
            DrawTextEx(*selected, text.c_str(), {x, y}, drawSize, 0.5f, color);
        else
            DrawText(text.c_str(), (int)x, (int)y, size, color);
    }
    static const Font* Pick(const std::string& text, bool heading) {
        bool unicode = HasUnicode(text) && unicodeFont;
        auto selected = unicode ? (heading && unicodeHeadingFont ? unicodeHeadingFont : unicodeFont)
                                : (heading && headingFont ? headingFont : font);
        return selected && selected->texture.id != 0 ? selected.Get() : nullptr;
    }
    static bool HasUnicode(const std::string& text) {
        return std::any_of(text.begin(), text.end(), [](unsigned char c) { return c >= 128; });
    }
};

inline FontHandle UI::font = nullptr;
inline Color UI::textColor = {226, 234, 248, 255};
inline Color UI::accentColor = {80, 180, 255, 255};
inline Color UI::panelColor = {12, 19, 34, 235};

} // namespace RayGame
