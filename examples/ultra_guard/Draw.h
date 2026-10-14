#pragma once

#include "Common.h"

namespace UG::Draw {

void Init();

void BlendNormal();
void BlendAdd();

void Tri(Vector2 a, Vector2 b, Vector2 c, Color color);
void Quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color);
// Fan from `center`; the outline must be star-shaped around it.
void Fan(Vector2 center, const std::vector<Vector2>& outline, Color color);
void Poly(const std::vector<Vector2>& outline, Color color);
void PolyLine(const std::vector<Vector2>& outline, float width, Color color, bool closed = true);

std::vector<Vector2> EllipsePoints(Vector2 c, float rx, float ry, float rotation = 0, int segments = 36);
// Ultra-style head: broad crown narrowing toward the chin.
std::vector<Vector2> EggPoints(Vector2 c, float rx, float ry, float chin, int segments = 48);
std::vector<Vector2> Transform(const std::vector<Vector2>& pts, Vector2 from, Vector2 to, float scale);

void Ellipse(Vector2 c, float rx, float ry, Color color, float rotation = 0);
void EllipseOutline(Vector2 c, float rx, float ry, float width, Color color, float rotation = 0);
// Outline, base color and a soft top-left highlight.
void Shaded(Vector2 c, float rx, float ry, Color base, float rotation = 0, float outline = 1.4f);
void Gradient(const std::vector<Vector2>& outline, Vector2 center, Color outer, Color inner,
              Vector2 highlight, int steps = 10);
void GradientEllipse(Vector2 c, float rx, float ry, Color outer, Color inner, Vector2 highlight,
                     int steps = 10, float rotation = 0);

void Line(Vector2 a, Vector2 b, float width, Color color);
void Capsule(Vector2 a, Vector2 b, float radius, Color color);
void Arc(Vector2 c, float radius, float width, float a0, float a1, Color color, int segments = 24);
void Crescent(Vector2 c, float radius, float thickness, float a0, float a1, Color color, int segments = 20);
void Star(Vector2 c, float outer, float inner, float rotation, Color color, int points = 5);
void Diamond(Vector2 c, float w, float h, Color color);

// Soft radial glow. Call BlendAdd() first for light, or use in normal mode for shadows.
void Glow(Vector2 c, float radius, Color color);
void GlowStretched(Vector2 c, float rx, float ry, float rotation, Color color);
void Shadow(Vector2 c, float rx, float ry, float alpha = 0.35f);
void Lightning(Vector2 a, Vector2 b, float jitter, float width, Color color, std::uint32_t seed);

void RoundedPanel(Rectangle r, float radius, Color fill, Color border, float borderWidth = 1.5f);
void VerticalGradientRect(Rectangle r, Color top, Color bottom);

Texture2D GlowTexture();
TextureHandle GlowHandle();

} // namespace UG::Draw
