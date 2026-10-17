#include "Art.h"
#include "Assets.h"
#include "Draw.h"
#include "Skins.h"
#include <map>
#include <rlgl.h>

namespace UG::Art {

namespace {
std::array<TextureHandle, kHeroCount> portraits;
std::array<bool, kHeroCount> custom{};

constexpr Vector2 H{128, 108};
constexpr Color kSilverHi{250, 252, 255, 255};
constexpr Color kSilver{198, 206, 218, 255};
constexpr Color kSilverLo{112, 122, 142, 255};
constexpr Color kOutline{22, 24, 36, 255};
constexpr Color kGold{232, 184, 72, 255};

Vector2 Centroid(const std::vector<Vector2>& pts) {
    Vector2 c{0, 0};
    for (auto& p : pts) c = Add(c, p);
    return Mul(c, 1.0f / pts.size());
}

std::vector<Vector2> Mirror(const std::vector<Vector2>& pts) {
    std::vector<Vector2> out(pts.rbegin(), pts.rend());
    for (auto& p : out) p.x = 256 - p.x;
    return out;
}

// Grows the outline by roughly `px` around its centroid, drawn underneath as an ink line.
void Ink(const std::vector<Vector2>& pts, float px) {
    Vector2 c = Centroid(pts);
    float r = 0;
    for (auto& p : pts) r += Len(Sub(p, c));
    r /= pts.size();
    Draw::Fan(c, Draw::Transform(pts, c, c, 1 + px / std::max(1.0f, r)), kOutline);
}

void Silver(const std::vector<Vector2>& pts, Vector2 highlight, float ink = 2.6f) {
    if (ink > 0) Ink(pts, ink);
    Draw::Gradient(pts, Centroid(pts), kSilverLo, kSilverHi, highlight, 14);
}

void Paint(const std::vector<Vector2>& pts, Color base, Vector2 highlight, float ink = 2.6f) {
    if (ink > 0) Ink(pts, ink);
    Draw::Gradient(pts, Centroid(pts), Darken(base, 0.42f), Lighten(base, 0.32f), highlight, 12);
}

void PaintBoth(const std::vector<Vector2>& left, Color base, float ink = 2.0f) {
    auto right = Mirror(left);
    Vector2 cl = Centroid(left), cr = Centroid(right);
    Paint(left, base, Add(cl, {-8, -10}), ink);
    Paint(right, base, Add(cr, {-4, -10}), ink);
}

void SilverBoth(const std::vector<Vector2>& left, float ink = 2.0f) {
    auto right = Mirror(left);
    Silver(left, Add(Centroid(left), {-8, -10}), ink);
    Silver(right, Add(Centroid(right), {-4, -10}), ink);
}

std::vector<Vector2> EyeShape(Vector2 c, float rx, float ry, float rot, float narrow, float side) {
    const int n = 40;
    std::vector<Vector2> pts(n);
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * i / n;
        float x = std::cos(a) * rx, y = std::sin(a) * ry;
        float inward = side < 0 ? std::cos(a) : -std::cos(a);
        if (inward > 0) y *= 1 - narrow * inward;
        pts[i] = Add(c, Rotate({x, y}, rot));
    }
    return pts;
}

void Eye(Vector2 c, float rx, float ry, float tilt, float narrow, float side, Color outer, Color inner) {
    float rot = side < 0 ? tilt : -tilt;
    auto pts = EyeShape(c, rx, ry, rot, narrow, side);
    Draw::Fan(c, Draw::Transform(pts, c, c, 1.2f), {54, 58, 76, 255});
    Draw::Fan(c, Draw::Transform(pts, c, c, 1.08f), kOutline);
    Draw::GlowStretched(c, rx * 2.1f, ry * 2.6f, rot, WithAlpha(inner, 0.32f));
    Draw::Gradient(pts, c, outer, inner, Add(c, Rotate({-rx * 0.1f, -ry * 0.3f}, rot)), 12);
    Draw::Ellipse(Add(c, Rotate({-rx * 0.32f, -ry * 0.38f}, rot)), rx * 0.32f, ry * 0.2f, WithAlpha(WHITE, 0.85f), rot);
    Draw::Ellipse(Add(c, Rotate({rx * 0.25f, ry * 0.2f}, rot)), rx * 0.12f, ry * 0.1f, WithAlpha(WHITE, 0.6f), rot);
}

void Eyes(float spacing, float y, float rx, float ry, float tilt, float narrow,
          Color outer = {236, 170, 46, 255}, Color inner = {255, 252, 214, 255}) {
    Eye({H.x - spacing, y}, rx, ry, tilt, narrow, -1, outer, inner);
    Eye({H.x + spacing, y}, rx, ry, tilt, narrow, 1, outer, inner);
}

void Mouth(float y, float width = 12) {
    Draw::Ellipse({H.x, y + 2}, width + 3, 6.5f, WithAlpha(kSilverLo, 0.45f));
    Draw::Ellipse({H.x, y}, width + 1, 4.6f, Mix(kSilver, kSilverHi, 0.55f));
    Draw::Ellipse({H.x, y - 1.5f}, width * 0.8f, 1.6f, WithAlpha(WHITE, 0.6f));
    Draw::Line({H.x - width * 0.62f, y + 1}, {H.x + width * 0.62f, y + 1}, 1.6f, {70, 76, 96, 255});
    Draw::Line({H.x - width * 0.62f, y + 1}, {H.x - width * 0.9f, y - 0.8f}, 1.3f, {90, 96, 114, 255});
    Draw::Line({H.x + width * 0.62f, y + 1}, {H.x + width * 0.9f, y - 0.8f}, 1.3f, {90, 96, 114, 255});
}

void NoseRidge(float top, float bottom) {
    Draw::Quad({H.x - 1.5f, top}, {H.x + 1.5f, top}, {H.x + 4, bottom}, {H.x - 4, bottom}, WithAlpha(kSilverHi, 0.9f));
    Draw::Line({H.x + 3.5f, top + 6}, {H.x + 5, bottom}, 1.4f, WithAlpha(kSilverLo, 0.6f));
}

void Ears(float y, float dx, Color color) {
    for (float side : {-1.0f, 1.0f}) {
        Vector2 c{H.x + side * dx, y};
        Draw::Ellipse(c, 6.5f, 14, kOutline);
        Draw::GradientEllipse(c, 5, 12.5f, Darken(color, 0.35f), Lighten(color, 0.25f), Add(c, {-1.5f, -4}), 6);
        Draw::Line({c.x, c.y - 7}, {c.x, c.y + 7}, 1.2f, WithAlpha(kOutline, 0.5f));
    }
}

void CheekShade() {
    Draw::GlowStretched({H.x - 34, H.y + 30}, 22, 30, 0, WithAlpha(kOutline, 0.16f));
    Draw::GlowStretched({H.x + 34, H.y + 30}, 22, 30, 0, WithAlpha(kOutline, 0.2f));
}

std::vector<Vector2> HeadShape(float rx = 50, float ry = 60) { return Draw::EggPoints(H, rx, ry, 0.3f, 56); }
std::vector<Vector2> FaceMask() { return Draw::EggPoints({H.x, H.y + 20}, 39, 44, 0.32f, 48); }
std::vector<Vector2> Shoulders() { return Draw::EllipsePoints({128, 266}, 124, 84, 0, 56); }

void Neck(Color color) {
    std::vector<Vector2> neck = {{98, 146}, {158, 146}, {164, 198}, {92, 198}};
    Paint(neck, color, {114, 166}, 2);
    Draw::GlowStretched({128, 176}, 30, 12, 0, WithAlpha(kOutline, 0.35f));
}

// Soft bottom edge so busts blend into cards and pedestals instead of ending in a hard cut.
void FadeBottom(Image& image) {
    auto* px = (Color*)image.data;
    int start = image.height * 236 / 256;
    for (int y = start; y < image.height; ++y) {
        float k = 1.0f - (float)(y - start) / (image.height - start);
        for (int x = 0; x < image.width; ++x) px[y * image.width + x].a = (unsigned char)(px[y * image.width + x].a * k);
    }
}

void ColorTimer(Vector2 c, float r) {
    DrawCircleV(c, r + 6, kOutline);
    Draw::GradientEllipse(c, r + 4.5f, r + 4.5f, kSilverLo, kSilverHi, Add(c, {-2, -3}), 8);
    DrawCircleV(c, r + 0.8f, Darken(kOutline, 0.2f));
    Draw::Glow(c, r * 3.2f, WithAlpha({90, 200, 255, 255}, 0.5f));
    Draw::GradientEllipse(c, r, r, {16, 96, 206, 255}, {206, 246, 255, 255}, Add(c, {-r * 0.3f, -r * 0.35f}), 12);
    Draw::Ellipse(Add(c, {-r * 0.35f, -r * 0.4f}), r * 0.32f, r * 0.2f, WithAlpha(WHITE, 0.9f), -0.5f);
}

void Lens(Vector2 c, float rx, float ry, float rot) {
    auto pts = Draw::EllipsePoints(c, rx, ry, rot, 36);
    Ink(pts, 2.4f);
    Draw::Gradient(pts, c, kSilverLo, kSilverHi, Add(c, Rotate({-rx * 0.4f, -ry * 0.2f}, rot)), 10);
    Vector2 a = Add(c, Rotate({0, -ry * 0.92f}, rot)), b = Add(c, Rotate({0, ry * 0.9f}, rot));
    Draw::Line(a, b, 1.3f, WithAlpha(kSilverLo, 0.9f));
}

void Lamp(Vector2 c, float rx, float ry, Color color) {
    Draw::Ellipse(c, rx + 2.4f, ry + 2.4f, kOutline);
    Draw::Glow(c, rx * 3.2f, WithAlpha(color, 0.55f));
    Draw::GradientEllipse(c, rx, ry, Darken(color, 0.3f), Lighten(color, 0.75f), Add(c, {-rx * 0.3f, -ry * 0.3f}), 8);
}

void Crest(float top, float bottom, float topHalf, float bottomHalf, Color lit, Color shade) {
    Draw::Quad({H.x - topHalf - 2, top + 2}, {H.x + topHalf + 2, top + 2}, {H.x + bottomHalf + 2, bottom + 1},
               {H.x - bottomHalf - 2, bottom + 1}, kOutline);
    Draw::Quad({H.x, top - 2}, {H.x - topHalf, top + 4}, {H.x - bottomHalf, bottom}, {H.x, bottom + 3}, lit);
    Draw::Quad({H.x, top - 2}, {H.x + topHalf, top + 4}, {H.x + bottomHalf, bottom}, {H.x, bottom + 3}, shade);
    Draw::Line({H.x, top - 1}, {H.x, bottom + 2}, 1.2f, WithAlpha(WHITE, 0.8f));
}

std::vector<Vector2> Horn(Vector2 base, Vector2 control, Vector2 tip, float width) {
    std::vector<Vector2> left, right;
    const int n = 16;
    for (int i = 0; i <= n; ++i) {
        float t = (float)i / n;
        Vector2 p = Add(Add(Mul(base, (1 - t) * (1 - t)), Mul(control, 2 * (1 - t) * t)), Mul(tip, t * t));
        Vector2 d = Add(Mul(Sub(control, base), 2 * (1 - t)), Mul(Sub(tip, control), 2 * t));
        Vector2 nrm = Normalize({-d.y, d.x});
        float w = width * (1 - t * 0.85f) * 0.5f;
        left.push_back(Add(p, Mul(nrm, w)));
        right.push_back(Sub(p, Mul(nrm, w)));
    }
    std::vector<Vector2> out = left;
    out.insert(out.end(), right.rbegin(), right.rend());
    return out;
}

void HornFill(const std::vector<Vector2>& pts) {
    // Horns curve, so fill as strips rather than a single fan.
    size_t half = pts.size() / 2;
    Draw::PolyLine(pts, 4.0f, kOutline, true);
    for (size_t i = 0; i + 1 < half; ++i) {
        Vector2 a = pts[i], b = pts[i + 1];
        Vector2 c = pts[pts.size() - 2 - i], d = pts[pts.size() - 1 - i];
        float t = (float)i / half;
        Draw::Quad(a, b, c, d, Mix(kSilverHi, kSilverLo, 0.25f + 0.5f * t));
    }
    for (size_t i = 0; i + 1 < half; ++i)
        Draw::Line(pts[i], pts[i + 1], 1.6f, WithAlpha(WHITE, 0.7f));
}

void Vents(float x0, float x1, float y0, int rows, Color color) {
    for (int r = 0; r < rows; ++r) {
        float y = y0 + r * 9.5f;
        for (float x = x0; x < x1; x += 9) {
            DrawRectangleRounded({x, y, 6, 3.2f}, 0.5f, 4, Darken(color, 0.45f));
            DrawRectangleRounded({x, y - 0.6f, 6, 2.2f}, 0.5f, 4, color);
        }
    }
}

// ---------------------------------------------------------------- heroes

void DrawUltraman() {
    const Color red{208, 36, 44, 255};
    auto sh = Shoulders();
    Silver(sh, {96, 214});
    PaintBoth({{6, 264}, {12, 226}, {38, 202}, {76, 190}, {100, 196}, {98, 214}, {80, 234}, {70, 264}}, red);
    PaintBoth({{104, 250}, {110, 240}, {116, 256}, {108, 262}}, red, 1);
    ColorTimer({128, 232}, 12);
    Neck(kSilver);
    auto head = HeadShape();
    Silver(head, {H.x - 16, H.y - 30}, 3);
    Crest(42, 96, 8, 3.5f, kSilverHi, kSilver);
    Ears(H.y + 4, 50, kSilver);
    CheekShade();
    Eyes(23, H.y + 2, 17.5f, 10.5f, 0.42f, 0.35f);
    NoseRidge(H.y + 10, H.y + 30);
    Mouth(H.y + 40);
}

void DrawSeven() {
    const Color red{204, 30, 40, 255};
    auto sh = Shoulders();
    Paint(sh, red, {96, 210});
    SilverBoth({{24, 236}, {34, 212}, {70, 196}, {110, 198}, {114, 230}, {106, 262}, {40, 262}});
    Vents(46, 104, 206, 5, kGold);
    Vents(156, 214, 206, 5, kGold);
    Neck(red);
    auto head = HeadShape(50, 60);
    Paint(head, red, {H.x - 16, H.y - 30}, 3);
    Lens({128, 48}, 9.5f, 26, 0);
    Silver(FaceMask(), {H.x - 10, H.y + 4}, 2);
    Lamp({128, 86}, 7.5f, 4.6f, {110, 255, 160, 255});
    Ears(H.y + 6, 50, kSilver);
    Eyes(22, H.y + 6, 18.5f, 11.5f, 0.3f, 0.22f);
    Mouth(H.y + 42, 11);
}

void DrawTiga() {
    const Color red{206, 38, 52, 255}, purple{120, 56, 186, 255};
    auto sh = Shoulders();
    Silver(sh, {96, 214});
    Paint({{6, 264}, {12, 226}, {46, 202}, {94, 192}, {100, 264}}, red, {40, 220}, 2);
    Paint(Mirror({{6, 264}, {12, 226}, {46, 202}, {94, 192}, {100, 264}}), purple, {200, 220}, 2);
    std::vector<Vector2> plate = {{36, 208}, {84, 192}, {128, 212}, {172, 192}, {220, 208}, {198, 244},
                                  {128, 260}, {58, 244}};
    Silver(plate, {90, 206}, 2.4f);
    Draw::PolyLine(plate, 3.5f, kGold, true);
    Draw::PolyLine(Draw::Transform(plate, {128, 226}, {128, 228}, 0.84f), 1.6f, WithAlpha(kGold, 0.8f), true);
    ColorTimer({128, 232}, 11);
    Neck(kSilver);
    auto head = HeadShape(49, 60);
    Silver(head, {H.x - 16, H.y - 30}, 3);
    for (float side : {-1.0f, 1.0f}) {
        Draw::Line({H.x + side * 22, 56}, {H.x + side * 40, 98}, 2.4f, WithAlpha(kSilverLo, 0.7f));
        Draw::Line({H.x + side * 20, 56}, {H.x + side * 38, 98}, 1.4f, WithAlpha(WHITE, 0.8f));
    }
    Crest(40, 84, 11, 6, kSilverHi, kSilver);
    // Tiga crystal
    Draw::Diamond({128, 88}, 8, 11, kOutline);
    Draw::Glow({128, 88}, 18, WithAlpha({255, 220, 120, 255}, 0.55f));
    Draw::Diamond({128, 88}, 6, 9, {214, 150, 40, 255});
    Draw::Diamond({126.5f, 86}, 3, 5, {255, 240, 180, 255});
    Ears(H.y + 4, 49, kSilver);
    CheekShade();
    Eyes(22, H.y + 4, 16.5f, 10.5f, 0.38f, 0.3f, {224, 186, 90, 255}, {255, 252, 232, 255});
    NoseRidge(H.y + 12, H.y + 30);
    Mouth(H.y + 40);
}

void DrawCosmos() {
    const Color blue{70, 158, 238, 255};
    auto sh = Shoulders();
    Silver(sh, {96, 214});
    PaintBoth({{6, 264}, {14, 226}, {44, 204}, {86, 194}, {94, 214}, {78, 264}}, blue);
    std::vector<Vector2> plate = {{80, 204}, {128, 196}, {176, 204}, {168, 246}, {128, 258}, {88, 246}};
    Silver(plate, {104, 206}, 2.4f);
    Draw::PolyLine(plate, 3, kGold, true);
    ColorTimer({128, 230}, 12);
    Neck(kSilver);
    auto head = HeadShape(50, 60);
    Silver(head, {H.x - 16, H.y - 30}, 3);
    Draw::Crescent(H, 47, 10, PI * 1.06f, PI * 1.42f, blue, 16);
    Draw::Crescent(H, 47, 10, PI * 1.58f, PI * 1.94f, blue, 16);
    Crest(42, 90, 7, 3.5f, kSilverHi, kSilver);
    Draw::Line({H.x, 46}, {H.x, 84}, 2.2f, blue);
    Lamp({128, 88}, 5, 4, {120, 200, 255, 255});
    Ears(H.y + 4, 50, kSilver);
    CheekShade();
    Eyes(23, H.y + 4, 18, 12, 0.3f, 0.28f, {230, 180, 60, 255}, {255, 253, 226, 255});
    NoseRidge(H.y + 12, H.y + 30);
    Mouth(H.y + 41);
}

void DrawTaro() {
    const Color red{214, 34, 38, 255};
    auto sh = Shoulders();
    Paint(sh, red, {96, 210});
    std::vector<Vector2> plate = {{22, 214}, {72, 194}, {128, 208}, {184, 194}, {234, 214}, {212, 262}, {44, 262}};
    Silver(plate, {80, 204}, 2.4f);
    for (float y = 214; y < 256; y += 12)
        for (float x = 40; x < 220; x += 14) {
            if (std::fabs(x - 128) < 24) continue;
            DrawCircleV({x, y + 1}, 3.2f, kSilverLo);
            DrawCircleV({x, y}, 2.6f, kGold);
        }
    ColorTimer({128, 232}, 12);
    Neck(red);
    HornFill(Horn({100, 66}, {44, 56}, {60, 6}, 22));
    HornFill(Mirror(Horn({100, 66}, {44, 56}, {60, 6}, 22)));
    auto head = HeadShape(50, 60);
    Paint(head, red, {H.x - 16, H.y - 30}, 3);
    Silver(FaceMask(), {H.x - 10, H.y + 4}, 2);
    Ears(H.y + 6, 50, kSilver);
    Eyes(22, H.y + 6, 18.5f, 12, 0.3f, 0.25f);
    Mouth(H.y + 42, 11);
}

void DrawZero() {
    const Color blue{36, 86, 204, 255}, red{210, 36, 52, 255};
    auto sh = Shoulders();
    Paint(sh, blue, {96, 210});
    PaintBoth({{6, 264}, {12, 228}, {34, 206}, {60, 198}, {58, 230}, {44, 264}}, red);
    SilverBoth({{62, 200}, {110, 198}, {114, 232}, {104, 262}, {58, 262}, {56, 226}});
    Vents(66, 106, 208, 5, {120, 255, 220, 255});
    Vents(150, 190, 208, 5, {120, 255, 220, 255});
    ColorTimer({128, 236}, 11);
    Neck(blue);
    auto head = HeadShape(50, 60);
    Paint(head, blue, {H.x - 16, H.y - 30}, 3);
    Draw::Crescent(H, 48, 13, PI * 1.02f, PI * 1.36f, red, 16);
    Draw::Crescent(H, 48, 13, PI * 1.64f, PI * 1.98f, red, 16);
    Silver(FaceMask(), {H.x - 10, H.y + 4}, 2);
    Lens({110, 50}, 7.5f, 28, -0.42f);
    Lens({146, 50}, 7.5f, 28, 0.42f);
    Lamp({128, 86}, 6.5f, 4.4f, {120, 255, 200, 255});
    Ears(H.y + 6, 50, kSilver);
    Eyes(22, H.y + 6, 20, 9.8f, 0.55f, 0.5f, {226, 196, 60, 255}, {255, 255, 236, 255});
    Mouth(H.y + 42, 10);
}

void DrawTrigger() {
    const Color red{206, 38, 52, 255}, purple{124, 52, 190, 255};
    auto sh = Shoulders();
    Silver(sh, {96, 214});
    Paint({{6, 264}, {12, 226}, {46, 202}, {94, 192}, {100, 264}}, purple, {40, 220}, 2);
    Paint(Mirror({{6, 264}, {12, 226}, {46, 202}, {94, 192}, {100, 264}}), red, {200, 220}, 2);
    std::vector<Vector2> plate = {{40, 206}, {90, 194}, {128, 220}, {166, 194}, {216, 206}, {196, 246}, {128, 262}, {60, 246}};
    Silver(plate, {90, 206}, 2.4f);
    Draw::PolyLine(plate, 3.5f, kGold, true);
    for (float side : {-1.0f, 1.0f}) Draw::Line({128 + side * 10, 222}, {128 + side * 52, 204}, 3, kGold);
    ColorTimer({128, 234}, 11);
    Neck(kSilver);
    auto head = HeadShape(49, 60);
    Silver(head, {H.x - 16, H.y - 30}, 3);
    for (float side : {-1.0f, 1.0f}) {
        Color stripe = side < 0 ? purple : red;
        Draw::Quad({H.x + side * 14, 50}, {H.x + side * 22, 52}, {H.x + side * 40, 96}, {H.x + side * 32, 98}, stripe);
        Draw::Line({H.x + side * 23, 52}, {H.x + side * 41, 96}, 1.4f, WithAlpha(WHITE, 0.7f));
    }
    Crest(36, 86, 13, 6, kSilverHi, kSilver);
    Draw::Diamond({128, 90}, 8, 11, kOutline);
    Draw::Glow({128, 90}, 18, WithAlpha({220, 150, 255, 255}, 0.55f));
    Draw::Diamond({128, 90}, 6, 9, {170, 90, 230, 255});
    Draw::Diamond({126.5f, 88}, 3, 5, {245, 225, 255, 255});
    Ears(H.y + 4, 49, kSilver);
    CheekShade();
    Eyes(22, H.y + 4, 17, 10.5f, 0.42f, 0.32f, {224, 186, 90, 255}, {255, 252, 232, 255});
    NoseRidge(H.y + 12, H.y + 30);
    Mouth(H.y + 40);
}

void DrawDecker() {
    const Color red{206, 36, 48, 255}, blue{40, 96, 210, 255}, navy{22, 30, 72, 255};
    auto sh = Shoulders();
    Silver(sh, {96, 214});
    Paint({{6, 264}, {12, 226}, {40, 204}, {78, 194}, {86, 264}}, red, {40, 220}, 2);
    Paint(Mirror({{6, 264}, {12, 226}, {40, 204}, {78, 194}, {86, 264}}), blue, {200, 220}, 2);
    std::vector<Vector2> plate = {{70, 200}, {128, 192}, {186, 200}, {178, 250}, {128, 262}, {78, 250}};
    Paint(plate, navy, {110, 206}, 2.4f);
    Draw::PolyLine(plate, 3, kGold, true);
    static const Vector2 stars[] = {{96, 210}, {86, 228}, {108, 240}, {90, 248}, {128, 214}, {120, 252}, {160, 250}};
    for (Vector2 s : stars) Draw::Star(s, 3.2f, 1.3f, 0, {235, 240, 255, 255});
    ColorTimer({154, 226}, 10); // on Decker's left chest, i.e. the viewer's right
    Neck(kSilver);
    auto head = HeadShape(50, 60);
    Silver(head, {H.x - 16, H.y - 30}, 3);
    Draw::Crescent(H, 47, 9, PI * 1.08f, PI * 1.40f, red, 16);
    Draw::Crescent(H, 47, 9, PI * 1.60f, PI * 1.92f, blue, 16);
    Crest(40, 90, 9, 4, kSilverHi, kSilver);
    Draw::Line({H.x - 3, 46}, {H.x - 3, 86}, 2, red);
    Draw::Line({H.x + 3, 46}, {H.x + 3, 86}, 2, blue);
    Lamp({128, 90}, 6, 4.2f, {90, 170, 255, 255});
    Ears(H.y + 4, 50, kSilver);
    CheekShade();
    Eyes(23, H.y + 4, 18, 11.5f, 0.34f, 0.3f, {230, 190, 70, 255}, {255, 253, 230, 255});
    NoseRidge(H.y + 12, H.y + 30);
    Mouth(H.y + 41);
}

void DrawBlazar() {
    const Color red{214, 40, 52, 255}, blue{50, 120, 230, 255}, dark{46, 48, 62, 255};
    auto sh = Shoulders();
    Paint(sh, dark, {96, 210});
    SilverBoth({{70, 198}, {112, 196}, {116, 232}, {106, 262}, {66, 262}, {62, 226}});
    // Asymmetric red/blue streaks down the right shoulder only.
    Paint({{150, 196}, {176, 192}, {236, 230}, {250, 264}, {226, 264}, {170, 214}}, red, {190, 210}, 2);
    Paint({{162, 214}, {178, 208}, {226, 240}, {236, 264}, {214, 264}, {176, 230}}, blue, {200, 230}, 2);
    DrawCircleV({128, 232}, 14, kOutline);
    Draw::GradientEllipse({128, 232}, 11, 11, {150, 30, 50, 255}, {255, 200, 160, 255}, {124, 228}, 10);
    Neck(dark);
    auto head = HeadShape(50, 60);
    Silver(head, {H.x - 16, H.y - 30}, 3);
    // Single swept fin and paint on one side of the head.
    std::vector<Vector2> fin = {{H.x + 2, 46}, {H.x + 16, 26}, {H.x + 38, 14}, {H.x + 50, 36},
                                {H.x + 46, 66}, {H.x + 30, 86}, {H.x + 8, 92}};
    Paint(fin, red, {H.x + 24, 34}, 2.4f);
    std::vector<Vector2> stripe = {{H.x + 14, 52}, {H.x + 30, 34}, {H.x + 42, 40}, {H.x + 38, 64}, {H.x + 22, 82}, {H.x + 12, 84}};
    Draw::Poly(stripe, blue);
    Draw::Line({H.x + 18, 30}, {H.x + 44, 40}, 1.4f, WithAlpha(WHITE, 0.7f));
    Ears(H.y + 4, 50, kSilver);
    CheekShade();
    Eyes(22, H.y + 6, 19, 10, 0.5f, 0.42f, {236, 200, 80, 255}, {255, 255, 238, 255});
    NoseRidge(H.y + 12, H.y + 30);
    Mouth(H.y + 41);
}

void Dilate(Image& image) {
    auto* px = (Color*)image.data;
    int w = image.width, h = image.height;
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<Color> copy(px, px + w * h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                Color& p = px[y * w + x];
                if (copy[y * w + x].a != 0) continue;
                int r = 0, g = 0, b = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                        const Color& q = copy[ny * w + nx];
                        if (q.a == 0 && q.r == 0 && q.g == 0 && q.b == 0) continue;
                        r += q.r, g += q.g, b += q.b, ++n;
                    }
                if (n) p = {(unsigned char)(r / n), (unsigned char)(g / n), (unsigned char)(b / n), 0};
            }
    }
}

void Finish(int i, Image& image) {
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Dilate(image);
    portraits[i] = ResourceManager::CreateTextureFromImage(std::string("portrait-") + HeroInfo((HeroId)i).key, image, true);
    if (portraits[i]) SetTextureWrap(*portraits[i], TEXTURE_WRAP_CLAMP);
}

bool LoadCustom(int i) {
    std::string file = std::string("avatars/") + HeroInfo((HeroId)i).key + ".png";
    std::string path = Skins::Find(file);
    if (path.empty()) path = AssetPath(file);
    if (!FileSystem::AssetExists(path)) return false;
    Image image = LoadImage(path.c_str());
    if (!IsImageValid(image)) return false;
    int side = std::min(image.width, image.height);
    ImageCrop(&image, {(image.width - side) / 2.0f, (image.height - side) / 2.0f, (float)side, (float)side});
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageResize(&image, 256, 256);
    Finish(i, image);
    UnloadImage(image);
    return true;
}
} // namespace

void BeginBakeBlend() {
    rlDrawRenderBatchActive();
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD,
                              RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
}

void DrawBust(HeroId id) {
    switch (id) {
    case HeroId::Ultraman: DrawUltraman(); break;
    case HeroId::Seven: DrawSeven(); break;
    case HeroId::Tiga: DrawTiga(); break;
    case HeroId::Cosmos: DrawCosmos(); break;
    case HeroId::Taro: DrawTaro(); break;
    case HeroId::Zero: DrawZero(); break;
    case HeroId::Trigger: DrawTrigger(); break;
    case HeroId::Decker: DrawDecker(); break;
    case HeroId::Blazar: DrawBlazar(); break;
    default: break;
    }
}

void BuildPortraits() {
    for (int i = 0; i < kHeroCount; ++i) {
        if (portraits[i]) continue;
        custom[i] = LoadCustom(i);
        if (custom[i]) continue;
        RenderTexture2D target = LoadRenderTexture(512, 512);
        BeginTextureMode(target);
        ClearBackground(BLANK);
        BeginBakeBlend();
        rlPushMatrix();
        rlScalef(2, 2, 1);
        DrawBust((HeroId)i);
        rlPopMatrix();
        EndBlendMode();
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        ImageResize(&image, 256, 256);
        FadeBottom(image);
        Finish(i, image);
        UnloadImage(image);
        UnloadRenderTexture(target);
    }
}

void ReloadPortraits() {
    for (int i = 0; i < kHeroCount; ++i) {
        if (!portraits[i]) continue;
        ResourceManager::UnloadTextureNamed(std::string("portrait-") + HeroInfo((HeroId)i).key);
        portraits[i] = TextureHandle{};
    }
    BuildPortraits();
}

Texture2D Portrait(HeroId id) {
    const auto& handle = portraits[(int)id];
    return handle ? *handle : Texture2D{};
}
bool HasCustomAvatar(HeroId id) { return custom[(int)id]; }

void DrawPortrait(HeroId id, Rectangle dest, Color tint) {
    Texture2D t = Portrait(id);
    if (!t.id) return;
    DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, dest, {0, 0}, 0, tint);
}

Texture2D CodexPortrait(const std::string& id) {
    HeroId hero{};
    if (HeroByKey(id, hero)) return Portrait(hero);
    static std::map<std::string, TextureHandle> loaded;
    auto it = loaded.find(id);
    if (it == loaded.end()) {
        TextureHandle handle;
        std::string path = AssetPath("avatars/" + id + ".png");
        Image image = FileSystem::AssetExists(path) ? LoadImage(path.c_str()) : Image{};
        if (IsImageValid(image)) {
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            handle = ResourceManager::CreateTextureFromImage("codex-" + id, image, true);
        }
        UnloadImage(image);
        it = loaded.emplace(id, handle).first;
    }
    return it->second ? *it->second : Texture2D{};
}

Vector2 EyePoint(HeroId id, int side) {
    float y = (id == HeroId::Seven || id == HeroId::Taro || id == HeroId::Zero) ? H.y + 6 : H.y + 3;
    return {H.x + (side < 0 ? -22.0f : 22.0f), y};
}

} // namespace UG::Art
