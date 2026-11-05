#include "Draw.h"
#include <rlgl.h>

namespace UG::Draw {

namespace {
TextureHandle glow;
Texture2D glowTexture{};
}

void Init() {
    const int size = 128;
    Image image = GenImageColor(size, size, BLANK);
    auto* pixels = (Color*)image.data;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (x + 0.5f) / size * 2 - 1, dy = (y + 0.5f) / size * 2 - 1;
            float d = std::sqrt(dx * dx + dy * dy);
            float a = d >= 1 ? 0 : std::pow(1 - d, 2.2f);
            pixels[y * size + x] = {255, 255, 255, (unsigned char)(a * 255)};
        }
    }
    glow = ResourceManager::CreateTextureFromImage("ug-glow", image, true);
    UnloadImage(image);
    glowTexture = glow ? *glow : Texture2D{};
}

TextureHandle GlowHandle() { return glow; }
Texture2D GlowTexture() { return glowTexture; }

void BlendNormal() { EndBlendMode(); }
void BlendAdd() { BeginBlendMode(BLEND_ADDITIVE); }

void Tri(Vector2 a, Vector2 b, Vector2 c, Color color) {
    // raylib culls clockwise 2D triangles; accept either winding.
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0) std::swap(b, c);
    DrawTriangle(a, b, c, color);
}

void Quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color) {
    Tri(a, b, c, color);
    Tri(a, c, d, color);
}

void Fan(Vector2 center, const std::vector<Vector2>& outline, Color color) {
    size_t n = outline.size();
    for (size_t i = 0; i < n; ++i) Tri(center, outline[i], outline[(i + 1) % n], color);
}

void Poly(const std::vector<Vector2>& outline, Color color) {
    if (outline.size() < 3) return;
    Vector2 c{0, 0};
    for (auto& p : outline) c = Add(c, p);
    c = Mul(c, 1.0f / outline.size());
    Fan(c, outline, color);
}

void PolyLine(const std::vector<Vector2>& outline, float width, Color color, bool closed) {
    size_t n = outline.size();
    if (n < 2) return;
    for (size_t i = 0; i + 1 < n; ++i) DrawLineEx(outline[i], outline[i + 1], width, color);
    if (closed) DrawLineEx(outline[n - 1], outline[0], width, color);
}

std::vector<Vector2> EllipsePoints(Vector2 c, float rx, float ry, float rotation, int segments) {
    std::vector<Vector2> pts(segments);
    float cr = std::cos(rotation), sr = std::sin(rotation);
    for (int i = 0; i < segments; ++i) {
        float a = 2 * PI * i / segments;
        float x = std::cos(a) * rx, y = std::sin(a) * ry;
        pts[i] = {c.x + x * cr - y * sr, c.y + x * sr + y * cr};
    }
    return pts;
}

std::vector<Vector2> EggPoints(Vector2 c, float rx, float ry, float chin, int segments) {
    std::vector<Vector2> pts(segments);
    for (int i = 0; i < segments; ++i) {
        float a = 2 * PI * i / segments;
        float s = std::sin(a);
        float pinch = s > 0 ? 1 - chin * s * s : 1 + 0.04f * s;
        pts[i] = {c.x + std::cos(a) * rx * pinch, c.y + s * ry};
    }
    return pts;
}

std::vector<Vector2> Transform(const std::vector<Vector2>& pts, Vector2 from, Vector2 to, float scale) {
    std::vector<Vector2> out(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) out[i] = Add(to, Mul(Sub(pts[i], from), scale));
    return out;
}

void Ellipse(Vector2 c, float rx, float ry, Color color, float rotation) {
    int segs = (int)Clamp(std::max(rx, ry) * 0.9f, 14.0f, 48.0f);
    Fan(c, EllipsePoints(c, rx, ry, rotation, segs), color);
}

void EllipseOutline(Vector2 c, float rx, float ry, float width, Color color, float rotation) {
    int segs = (int)Clamp(std::max(rx, ry) * 0.9f, 16.0f, 56.0f);
    PolyLine(EllipsePoints(c, rx, ry, rotation, segs), width, color, true);
}

void Shaded(Vector2 c, float rx, float ry, Color base, float rotation, float outline) {
    if (outline > 0) Ellipse(c, rx + outline, ry + outline, Darken(base, 0.62f), rotation);
    Ellipse(c, rx, ry, base, rotation);
    Vector2 off = Rotate({-rx * 0.22f, -ry * 0.28f}, rotation);
    Ellipse(Add(c, off), rx * 0.58f, ry * 0.5f, WithAlpha(Lighten(base, 0.3f), 0.55f), rotation);
}

void Gradient(const std::vector<Vector2>& outline, Vector2 center, Color outer, Color inner,
              Vector2 highlight, int steps) {
    for (int i = 0; i < steps; ++i) {
        float t = (float)i / steps;
        float scale = 1 - t * 0.92f;
        Vector2 to = Lerp(center, highlight, t);
        Fan(to, Transform(outline, center, to, scale), Mix(outer, inner, t));
    }
}

void GradientEllipse(Vector2 c, float rx, float ry, Color outer, Color inner, Vector2 highlight,
                     int steps, float rotation) {
    Gradient(EllipsePoints(c, rx, ry, rotation, 40), c, outer, inner, highlight, steps);
}

void Line(Vector2 a, Vector2 b, float width, Color color) { DrawLineEx(a, b, width, color); }

void Capsule(Vector2 a, Vector2 b, float radius, Color color) {
    DrawLineEx(a, b, radius * 2, color);
    DrawCircleV(a, radius, color);
    DrawCircleV(b, radius, color);
}

void Arc(Vector2 c, float radius, float width, float a0, float a1, Color color, int segments) {
    DrawRing(c, radius - width / 2, radius + width / 2, a0 * RAD2DEG, a1 * RAD2DEG, segments, color);
}

void Crescent(Vector2 c, float radius, float thickness, float a0, float a1, Color color, int segments) {
    for (int i = 0; i < segments; ++i) {
        float t0 = (float)i / segments, t1 = (float)(i + 1) / segments;
        float b0 = Lerp(a0, a1, t0), b1 = Lerp(a0, a1, t1);
        float w0 = thickness * std::sin(PI * t0), w1 = thickness * std::sin(PI * t1);
        Vector2 o0 = Add(c, Mul(Dir(b0), radius)), o1 = Add(c, Mul(Dir(b1), radius));
        Vector2 i0 = Add(c, Mul(Dir(b0), radius - w0)), i1 = Add(c, Mul(Dir(b1), radius - w1));
        Quad(o0, o1, i1, i0, color);
    }
}

void Star(Vector2 c, float outer, float inner, float rotation, Color color, int points) {
    std::vector<Vector2> pts;
    for (int i = 0; i < points * 2; ++i) {
        float r = i % 2 == 0 ? outer : inner;
        float a = rotation - PI / 2 + PI * i / points;
        pts.push_back(Add(c, Mul(Dir(a), r)));
    }
    Fan(c, pts, color);
}

void Diamond(Vector2 c, float w, float h, Color color) {
    Quad({c.x, c.y - h}, {c.x + w, c.y}, {c.x, c.y + h}, {c.x - w, c.y}, color);
}

void Glow(Vector2 c, float radius, Color color) {
    if (radius <= 0) return;
    Rectangle src{0, 0, (float)glowTexture.width, (float)glowTexture.height};
    DrawTexturePro(glowTexture, src, {c.x - radius, c.y - radius, radius * 2, radius * 2}, {0, 0}, 0, color);
}

void GlowStretched(Vector2 c, float rx, float ry, float rotation, Color color) {
    Rectangle src{0, 0, (float)glowTexture.width, (float)glowTexture.height};
    DrawTexturePro(glowTexture, src, {c.x, c.y, rx * 2, ry * 2}, {rx, ry}, rotation * RAD2DEG, color);
}

void Shadow(Vector2 c, float rx, float ry, float alpha) {
    GlowStretched(c, rx * 1.25f, ry * 1.25f, 0, WithAlpha({0, 0, 10, 255}, alpha));
}

void Lightning(Vector2 a, Vector2 b, float jitter, float width, Color color, std::uint32_t seed) {
    Rng rng(seed);
    Vector2 prev = a;
    int segs = std::max(3, (int)(Len(Sub(b, a)) / 10));
    Vector2 d = Sub(b, a);
    Vector2 n = Normalize({-d.y, d.x});
    for (int i = 1; i <= segs; ++i) {
        float t = (float)i / segs;
        Vector2 p = Add(a, Mul(d, t));
        if (i < segs) p = Add(p, Mul(n, rng.Range(-jitter, jitter)));
        DrawLineEx(prev, p, width, color);
        prev = p;
    }
}

void RoundedPanel(Rectangle r, float radius, Color fill, Color border, float borderWidth) {
    float roundness = Clamp(radius * 2 / std::max(1.0f, std::min(r.width, r.height)), 0.0f, 1.0f);
    DrawRectangleRounded(r, roundness, 10, fill);
    if (borderWidth > 0 && border.a > 0) DrawRectangleRoundedLinesEx(r, roundness, 10, borderWidth, border);
}

void VerticalGradientRect(Rectangle r, Color top, Color bottom) {
    DrawRectangleGradientV((int)r.x, (int)r.y, (int)std::ceil(r.width), (int)std::ceil(r.height), top, bottom);
}

} // namespace UG::Draw
