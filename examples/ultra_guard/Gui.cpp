#include "Gui.h"
#include "Assets.h"
#include "Draw.h"

namespace UG::Gui {

namespace {
Vector2 pressPosition{-1, -1};
bool hasPress = false;
constexpr float kSpacing = 0.6f;

const Font& Pick(bool bold) { return bold ? Fonts::Bold() : Fonts::Body(); }

UI::Align ToUI(Align align) {
    return align == Align::Center ? UI::Align::Center : align == Align::Right ? UI::Align::Right : UI::Align::Left;
}
} // namespace

float Measure(const std::string& text, float size, bool bold) {
    return MeasureTextEx(Pick(bold), text.c_str(), size, kSpacing).x;
}

void Text(const std::string& text, float x, float y, float size, Color color, Align align, bool bold) {
    UI::Text(std::round(x), std::round(y), text, size, color, ToUI(align), bold);
}

void TextShadow(const std::string& text, float x, float y, float size, Color color, Align align, bool bold,
                float offset) {
    Text(text, x + offset * 0.6f, y + offset, size, WithAlpha({0, 0, 12, 255}, 0.6f * color.a / 255.0f), align,
         bold);
    Text(text, x, y, size, color, align, bold);
}

void TextOutline(const std::string& text, float x, float y, float size, Color color, Color outline,
                 float thickness, Align align, bool bold) {
    UI::Text(x, y, text, size, color, ToUI(align), bold, thickness, outline);
}

float Paragraph(const std::string& text, float x, float y, float width, float size, Color color, float lineGap) {
    std::string line;
    float cy = y;
    size_t i = 0;
    auto flush = [&] {
        Text(line, x, cy, size, color);
        cy += size * lineGap;
        line.clear();
    };
    while (i < text.size()) {
        unsigned char c = (unsigned char)text[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : 4;
        std::string ch = text.substr(i, len);
        i += len;
        if (ch == "\n") {
            flush();
            continue;
        }
        if (!line.empty() && Measure(line + ch, size) > width) flush();
        line += ch;
    }
    if (!line.empty()) flush();
    return cy - y;
}

void BeginFrame() {
    if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        pressPosition = Input::GetMousePosition();
        hasPress = true;
    }
    if (!Input::GetFrame().focused) hasPress = false;
}

bool Hover(Rectangle r) {
    return Input::IsMouseInCanvas() && CheckCollisionPointRec(Input::GetMousePosition(), r);
}

bool Clicked(Rectangle r) {
    bool clicked = hasPress && Hover(r) && CheckCollisionPointRec(pressPosition, r) &&
                   Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT);
    if (clicked) {
        hasPress = false;
        Sfx::Play("click", 0.5f, 0.02f);
    }
    return clicked;
}

bool Button(Rectangle r, const std::string& label, Style style, bool enabled, float size) {
    static const ButtonStyle themes[] = {
        {{196, 118, 26, 245}, {255, 214, 110, 255}, WHITE, 0.3f}, // Gold
        {{36, 92, 190, 245}, {140, 200, 255, 255}, WHITE, 0.3f},  // Blue
        {{168, 38, 52, 245}, {255, 130, 140, 255}, WHITE, 0.3f},  // Red
        {{30, 40, 70, 245}, {130, 160, 220, 255}, WHITE, 0.3f},   // Dark
        {{34, 128, 80, 245}, {140, 240, 170, 255}, WHITE, 0.3f},  // Green
    };
    bool clicked = UI::Button(r, label, themes[(int)style], (int)size, enabled);
    if (clicked) Sfx::Play("click", 0.5f, 0.02f);
    return clicked;
}

void Panel(Rectangle r, Color fill, float radius) {
    UI::Capture(r);
    Draw::RoundedPanel({r.x + 2, r.y + 4, r.width, r.height}, radius, WithAlpha(BLACK, 0.35f), BLANK, 0);
    Draw::RoundedPanel(r, radius, fill, Palette::line, 1.5f);
}

void Header(Rectangle r, const std::string& title, Color accent) {
    DrawRectangleRec({r.x, r.y + r.height - 2, r.width, 2}, WithAlpha(accent, 0.6f));
    Draw::Diamond({r.x + 8, r.y + r.height / 2}, 4, 6, accent);
    Text(title, r.x + 20, r.y + (r.height - 18) / 2 - 1, 18, Palette::text, Align::Left, true);
}

// ---------------------------------------------------------------- icons

void Coin(Vector2 c, float r) {
    DrawCircleV({c.x, c.y + r * 0.18f}, r, {150, 92, 10, 255});
    DrawCircleV(c, r, {255, 196, 54, 255});
    DrawCircleV(c, r * 0.72f, {240, 160, 30, 255});
    Draw::Star(c, r * 0.52f, r * 0.22f, 0, {255, 236, 150, 255});
    DrawCircleV({c.x - r * 0.38f, c.y - r * 0.42f}, r * 0.22f, WithAlpha(WHITE, 0.7f));
}

void Heart(Vector2 c, float r, Color color) {
    DrawCircleV({c.x - r * 0.48f, c.y - r * 0.2f}, r * 0.55f, color);
    DrawCircleV({c.x + r * 0.48f, c.y - r * 0.2f}, r * 0.55f, color);
    Draw::Tri({c.x - r * 1.0f, c.y - r * 0.05f}, {c.x + r * 1.0f, c.y - r * 0.05f}, {c.x, c.y + r * 0.95f}, color);
    DrawCircleV({c.x - r * 0.55f, c.y - r * 0.35f}, r * 0.2f, WithAlpha(WHITE, 0.6f));
}

void StarIcon(Vector2 c, float r, bool filled) {
    if (filled) {
        Draw::Star({c.x, c.y + r * 0.08f}, r * 1.08f, r * 0.5f, 0, {150, 90, 10, 255});
        Draw::Star(c, r, r * 0.46f, 0, {255, 206, 60, 255});
        Draw::Star({c.x - r * 0.08f, c.y - r * 0.1f}, r * 0.5f, r * 0.22f, 0, {255, 240, 170, 255});
    } else {
        Draw::Star(c, r, r * 0.46f, 0, {54, 64, 92, 255});
        Draw::Star(c, r * 0.72f, r * 0.33f, 0, {34, 42, 66, 255});
    }
}

void PlayIcon(Vector2 c, float r, Color color) {
    Draw::Tri({c.x - r * 0.6f, c.y - r * 0.8f}, {c.x - r * 0.6f, c.y + r * 0.8f}, {c.x + r * 0.8f, c.y}, color);
}

void PauseIcon(Vector2 c, float r, Color color) {
    DrawRectangleRounded({c.x - r * 0.7f, c.y - r * 0.8f, r * 0.5f, r * 1.6f}, 0.3f, 4, color);
    DrawRectangleRounded({c.x + r * 0.2f, c.y - r * 0.8f, r * 0.5f, r * 1.6f}, 0.3f, 4, color);
}

void FastIcon(Vector2 c, float r, Color color, int arrows) {
    float w = r * 0.9f;
    float start = c.x - w * arrows / 2;
    for (int i = 0; i < arrows; ++i) {
        float x = start + i * w;
        Draw::Tri({x, c.y - r * 0.7f}, {x, c.y + r * 0.7f}, {x + w, c.y}, color);
    }
}

void LockIcon(Vector2 c, float r, Color color) {
    Draw::Arc({c.x, c.y - r * 0.2f}, r * 0.55f, r * 0.22f, PI, 2 * PI, color);
    DrawRectangleRounded({c.x - r * 0.8f, c.y - r * 0.2f, r * 1.6f, r * 1.2f}, 0.25f, 4, color);
    DrawCircleV({c.x, c.y + r * 0.3f}, r * 0.18f, Darken(color, 0.6f));
}

void SoundIcon(Vector2 c, float r, Color color, bool on) {
    DrawRectangleRec({c.x - r * 0.9f, c.y - r * 0.3f, r * 0.4f, r * 0.6f}, color);
    Draw::Quad({c.x - r * 0.5f, c.y - r * 0.3f}, {c.x + r * 0.1f, c.y - r * 0.8f}, {c.x + r * 0.1f, c.y + r * 0.8f},
               {c.x - r * 0.5f, c.y + r * 0.3f}, color);
    if (on) {
        Draw::Arc({c.x + r * 0.1f, c.y}, r * 0.45f, r * 0.14f, -PI / 3, PI / 3, color);
        Draw::Arc({c.x + r * 0.1f, c.y}, r * 0.8f, r * 0.14f, -PI / 3, PI / 3, color);
    } else {
        Draw::Line({c.x + r * 0.35f, c.y - r * 0.4f}, {c.x + r * 0.95f, c.y + r * 0.4f}, r * 0.16f, color);
        Draw::Line({c.x + r * 0.35f, c.y + r * 0.4f}, {c.x + r * 0.95f, c.y - r * 0.4f}, r * 0.16f, color);
    }
}

void NoteIcon(Vector2 c, float r, Color color, bool on) {
    Draw::Ellipse({c.x - r * 0.45f, c.y + r * 0.55f}, r * 0.32f, r * 0.24f, color, -0.4f);
    Draw::Ellipse({c.x + r * 0.45f, c.y + r * 0.4f}, r * 0.32f, r * 0.24f, color, -0.4f);
    Draw::Line({c.x - r * 0.18f, c.y + r * 0.5f}, {c.x - r * 0.18f, c.y - r * 0.7f}, r * 0.14f, color);
    Draw::Line({c.x + r * 0.72f, c.y + r * 0.35f}, {c.x + r * 0.72f, c.y - r * 0.85f}, r * 0.14f, color);
    Draw::Line({c.x - r * 0.18f, c.y - r * 0.7f}, {c.x + r * 0.72f, c.y - r * 0.85f}, r * 0.26f, color);
    if (!on) Draw::Line({c.x - r, c.y - r}, {c.x + r, c.y + r}, r * 0.16f, Palette::red);
}

void WarningIcon(Vector2 c, float r, Color color) {
    Draw::Tri({c.x, c.y - r}, {c.x + r * 1.1f, c.y + r * 0.85f}, {c.x - r * 1.1f, c.y + r * 0.85f}, color);
    Draw::Tri({c.x, c.y - r * 0.62f}, {c.x + r * 0.78f, c.y + r * 0.66f}, {c.x - r * 0.78f, c.y + r * 0.66f},
              Darken(color, 0.75f));
    DrawRectangleRec({c.x - r * 0.1f, c.y - r * 0.3f, r * 0.2f, r * 0.55f}, color);
    DrawCircleV({c.x, c.y + r * 0.45f}, r * 0.11f, color);
}

void SwordIcon(Vector2 c, float r, Color color) {
    Draw::Line({c.x - r * 0.7f, c.y + r * 0.7f}, {c.x + r * 0.7f, c.y - r * 0.7f}, r * 0.28f, color);
    Draw::Tri({c.x + r * 0.55f, c.y - r * 0.85f}, {c.x + r * 0.95f, c.y - r * 0.95f}, {c.x + r * 0.85f, c.y - r * 0.55f},
              color);
    Draw::Line({c.x - r * 0.65f, c.y + r * 0.1f}, {c.x - r * 0.1f, c.y + r * 0.65f}, r * 0.22f, color);
}

void RangeIcon(Vector2 c, float r, Color color) {
    DrawRing(c, r * 0.75f, r * 0.95f, 0, 360, 24, color);
    DrawRing(c, r * 0.32f, r * 0.5f, 0, 360, 18, color);
    DrawCircleV(c, r * 0.12f, color);
}

void ClockIcon(Vector2 c, float r, Color color) {
    DrawRing(c, r * 0.75f, r * 0.95f, 0, 360, 24, color);
    Draw::Line(c, {c.x, c.y - r * 0.55f}, r * 0.16f, color);
    Draw::Line(c, {c.x + r * 0.42f, c.y + r * 0.12f}, r * 0.16f, color);
}

} // namespace UG::Gui
