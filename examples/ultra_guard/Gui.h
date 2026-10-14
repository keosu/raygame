#pragma once

#include "Common.h"

namespace UG::Gui {

enum class Align { Left, Center, Right };
enum class Style { Gold, Blue, Red, Dark, Green };

float Measure(const std::string& text, float size, bool bold = false);
void Text(const std::string& text, float x, float y, float size, Color color, Align align = Align::Left,
          bool bold = false);
void TextShadow(const std::string& text, float x, float y, float size, Color color, Align align = Align::Left,
                bool bold = true, float offset = 2.0f);
void TextOutline(const std::string& text, float x, float y, float size, Color color, Color outline,
                 float thickness, Align align = Align::Center, bool bold = true);
// Wraps on character boundaries (Chinese text has no spaces); returns height used.
float Paragraph(const std::string& text, float x, float y, float width, float size, Color color,
                float lineGap = 1.45f);

// Records presses; call once per frame before any widget.
void BeginFrame();
bool Hover(Rectangle r);
bool Clicked(Rectangle r);
bool Button(Rectangle r, const std::string& label, Style style = Style::Gold, bool enabled = true,
            float size = 18.0f);
void Panel(Rectangle r, Color fill = Palette::panel, float radius = 10.0f);
void Header(Rectangle r, const std::string& title, Color accent);

void Coin(Vector2 c, float r);
void Heart(Vector2 c, float r, Color color = {255, 82, 104, 255});
void StarIcon(Vector2 c, float r, bool filled);
void PlayIcon(Vector2 c, float r, Color color);
void PauseIcon(Vector2 c, float r, Color color);
void FastIcon(Vector2 c, float r, Color color, int arrows);
void LockIcon(Vector2 c, float r, Color color);
void SoundIcon(Vector2 c, float r, Color color, bool on);
void NoteIcon(Vector2 c, float r, Color color, bool on);
void WarningIcon(Vector2 c, float r, Color color);
void SwordIcon(Vector2 c, float r, Color color);
void RangeIcon(Vector2 c, float r, Color color);
void ClockIcon(Vector2 c, float r, Color color);

} // namespace UG::Gui
