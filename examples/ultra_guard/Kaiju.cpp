#include "Art.h"
#include "Draw.h"
#include "Skins.h"

namespace UG::Art {

namespace {
// Local frame: origin at the feet, +x forward (mirrored by facing), -y up.
struct Frame {
    Vector2 o;
    float s, f;
    float flash, alpha, chill, burn;
    Vector2 P(float x, float y) const { return {o.x + x * s * f, o.y + y * s}; }
    float R(float r) const { return r * s; }
    float A(float radians) const { return f > 0 ? radians : PI - radians; }
    float Rot(float radians) const { return radians * f; }
    Color C(Color c) const {
        if (chill > 0) c = Mix(c, {150, 210, 255, c.a}, chill * 0.45f);
        if (burn > 0) c = Mix(c, {255, 120, 40, c.a}, burn * 0.25f);
        if (flash > 0) c = Mix(c, {255, 255, 255, c.a}, flash);
        c.a = (unsigned char)(c.a * alpha);
        return c;
    }
};

constexpr Color kInk{18, 16, 24, 255};

void Body(const Frame& k, float x, float y, float rx, float ry, Color c, float rot = 0, float ink = 1.3f) {
    Vector2 p = k.P(x, y);
    float r = k.Rot(rot);
    Draw::Ellipse(p, k.R(rx) + ink, k.R(ry) + ink, k.C(kInk), r);
    Draw::Ellipse(p, k.R(rx), k.R(ry), k.C(c), r);
    Vector2 off = Rotate({-k.R(rx) * 0.2f * k.f, -k.R(ry) * 0.32f}, r);
    Draw::Ellipse(Add(p, off), k.R(rx) * 0.6f, k.R(ry) * 0.48f, k.C(WithAlpha(Lighten(c, 0.28f), 0.6f)), r);
}

void Limb(const Frame& k, float x0, float y0, float x1, float y1, float r, Color c) {
    Draw::Capsule(k.P(x0, y0), k.P(x1, y1), k.R(r) + 1.2f, k.C(kInk));
    Draw::Capsule(k.P(x0, y0), k.P(x1, y1), k.R(r), k.C(c));
}

void TriK(const Frame& k, Vector2 a, Vector2 b, Vector2 c, Color col) {
    Draw::Tri(k.P(a.x, a.y), k.P(b.x, b.y), k.P(c.x, c.y), k.C(col));
}

void PolyK(const Frame& k, std::initializer_list<Vector2> pts, Color col) {
    std::vector<Vector2> out;
    for (auto& p : pts) out.push_back(k.P(p.x, p.y));
    Draw::Poly(out, k.C(col));
}

void Eye(const Frame& k, float x, float y, float r, Color c) {
    DrawCircleV(k.P(x, y), k.R(r) + 1, k.C(kInk));
    DrawCircleV(k.P(x, y), k.R(r), k.C(c));
    DrawCircleV(k.P(x + 0.3f, y - 0.3f), k.R(r * 0.45f), k.C(WHITE));
}

void Gomora(const Frame& k, float t) {
    const Color body{142, 94, 58, 255}, belly{226, 196, 140, 255}, horn{244, 230, 196, 255};
    float step = std::sin(t * 9);
    PolyK(k, {{-8, -18}, {-26, -10}, {-42, -2}, {-40, 1}, {-22, -3}, {-6, -6}}, Darken(body, 0.15f));
    Limb(k, -5, -10, -6 - step * 3, -1, 5, Darken(body, 0.2f));
    Body(k, 0, -18, 16, 14.5f, body);
    for (int i = 0; i < 5; ++i) {
        float a = PI * (1.1f + 0.16f * i);
        Vector2 b = {std::cos(a) * 14, -18 + std::sin(a) * 13};
        TriK(k, {b.x - 3, b.y + 1}, {b.x + 3, b.y + 1}, {b.x - 2, b.y - 6}, Darken(body, 0.35f));
    }
    Body(k, 7, -16, 8, 10.5f, belly, 0, 1.0f);
    for (int i = 0; i < 4; ++i)
        Draw::Line(k.P(2, -22 + i * 4.5f), k.P(12, -22 + i * 4.5f), k.R(0.9f), k.C(Darken(belly, 0.25f)));
    Limb(k, 5, -9, 6 + step * 3, -1, 5.5f, body);
    DrawCircleV(k.P(7 + step * 3, 0), k.R(5.5f), k.C(Darken(body, 0.3f)));
    Limb(k, 11, -22, 18, -18 + step, 3, body);
    TriK(k, {18, -20 + step}, {22, -18 + step}, {19, -16 + step}, horn);
    Body(k, 15, -31, 10, 8, body);
    Body(k, 22, -28.5f, 5.5f, 3.8f, belly, 0, 1.0f);
    TriK(k, {20, -34}, {26, -31}, {32, -45}, horn);
    TriK(k, {8, -36}, {13, -38}, {6, -47}, horn);
    TriK(k, {13, -38}, {18, -37}, {19, -47}, horn);
    Eye(k, 16.5f, -33, 1.8f, {255, 210, 60, 255});
    Draw::Line(k.P(18, -27), k.P(25, -27.5f), k.R(0.8f), k.C(kInk));
}

void RedKing(const Frame& k, float t) {
    const Color body{214, 170, 92, 255}, stripe{168, 106, 52, 255};
    float step = std::sin(t * 7);
    PolyK(k, {{-10, -14}, {-28, -8}, {-40, -1}, {-38, 2}, {-20, -2}, {-8, -4}}, Darken(body, 0.12f));
    Limb(k, -6, -10, -7 - step * 3, -1, 6.5f, Darken(body, 0.18f));
    Body(k, 0, -20, 18, 18, body);
    static const Vector2 bumps[] = {{-10, -28}, {-4, -33}, {-13, -18}, {-8, -12}, {-2, -24}, {3, -34}, {-15, -25}};
    for (auto b : bumps) Body(k, b.x, b.y, 2.6f, 2.3f, Darken(body, 0.18f), 0, 0.6f);
    Body(k, 8, -18, 9, 12.5f, Lighten(body, 0.2f), 0, 1.0f);
    for (int i = 0; i < 6; ++i)
        Draw::Line(k.P(1, -27 + i * 3.6f), k.P(15, -27 + i * 3.6f), k.R(1.0f), k.C(stripe));
    Limb(k, 6, -10, 7 + step * 3, -1, 7, body);
    Limb(k, 12, -26, 19, -21 - step, 3.2f, body);
    Limb(k, 4, -34, 10, -44, 4.5f, body);
    Body(k, 11, -47, 6, 5, body);
    Eye(k, 13.5f, -48, 1.3f, {255, 220, 90, 255});
    Draw::Line(k.P(13, -44.5f), k.P(17, -45), k.R(0.7f), k.C(kInk));
}

void Eleking(const Frame& k, float t) {
    const Color body{238, 238, 228, 255}, spot{30, 30, 36, 255}, horn{255, 190, 50, 255};
    float step = std::sin(t * 11);
    // Long curling tail
    Vector2 prev = {-8, -12};
    for (int i = 1; i <= 10; ++i) {
        float u = i / 10.0f;
        Vector2 p = {-8 - u * 30, -12 + u * 10 + std::sin(t * 4 + u * 5) * 3};
        Limb(k, prev.x, prev.y, p.x, p.y, 4.5f * (1 - u * 0.7f), body);
        if (i % 3 == 0) DrawCircleV(k.P(p.x, p.y - 1), k.R(1.4f), k.C(spot));
        prev = p;
    }
    Limb(k, -4, -9, -5 - step * 3, -1, 4, Darken(body, 0.12f));
    Body(k, 0, -16, 14, 12, body);
    static const Vector2 spots[] = {{-7, -20}, {2, -24}, {-2, -12}, {6, -16}, {-10, -13}, {9, -22}};
    for (auto s : spots) Body(k, s.x, s.y, 2.4f, 1.9f, spot, 0.3f, 0.3f);
    Limb(k, 4, -8, 5 + step * 3, -1, 4.2f, body);
    Limb(k, 9, -22, 14, -32, 4.2f, body);
    Body(k, 16, -34, 7.5f, 6, body);
    DrawCircleV(k.P(23, -34), k.R(1.2f), k.C(spot));
    // Rotating antennae
    for (int i = 0; i < 2; ++i) {
        Vector2 c = k.P(13 + i * 5, -41);
        float spin = t * 7 + i * PI;
        Draw::Crescent(c, k.R(4.5f), k.R(2.2f), spin, spin + PI * 1.3f, k.C(Darken(horn, 0.5f)), 10);
        Draw::Crescent(c, k.R(4.0f), k.R(1.8f), spin, spin + PI * 1.3f, k.C(horn), 10);
    }
    // Head lamp
    float blink = 0.5f + 0.5f * std::sin(t * 13);
    DrawCircleV(k.P(19, -36), k.R(2.2f), k.C(Mix({255, 200, 60, 255}, {255, 255, 210, 255}, blink)));
    if (std::fmod(t, 1.7f) < 0.25f)
        Draw::Lightning(k.P(-20, -10), k.P(-36, -2), k.R(3), k.R(1.2f), k.C({140, 230, 255, 220}),
                        (std::uint32_t)(t * 30));
}

void Baltan(const Frame& k, float t, bool clone) {
    Color body = clone ? Color{96, 160, 230, 190} : Color{66, 72, 118, 255};
    Color claw = clone ? Color{150, 200, 255, 200} : Color{92, 100, 150, 255};
    float step = std::sin(t * 10);
    float bob = std::sin(t * 5) * 2;
    float open = 0.35f + 0.3f * (0.5f + 0.5f * std::sin(t * 6));
    Limb(k, -3, -14, -4 - step * 2, -1, 2.4f, Darken(body, 0.2f));
    Limb(k, 3, -14, 4 + step * 2, -1, 2.4f, body);
    Body(k, 0, -21, 8.5f, 10.5f, body);
    for (int i = 0; i < 3; ++i)
        Draw::Line(k.P(-5, -25 + i * 4), k.P(5, -25 + i * 4), k.R(0.8f), k.C(Lighten(body, 0.25f)));
    for (float side : {-1.0f, 1.0f}) {
        Limb(k, side * 6, -27, side * 13, -33 + bob, 2.4f, body);
        Vector2 c = {side * 15, -39 + bob};
        float base = -PI / 2;
        for (float jaw : {-1.0f, 1.0f}) {
            float a = base + jaw * open;
            Vector2 tip = {c.x + std::cos(a) * 9 * side, c.y + std::sin(a) * 9};
            TriK(k, {c.x - 2.8f, c.y + 2}, {c.x + 2.8f, c.y + 2}, tip, claw);
        }
        Body(k, c.x, c.y + 1.5f, 3.6f, 3.2f, claw, 0, 0.8f);
    }
    Body(k, 0, -37, 8, 6.5f, body);
    TriK(k, {-2, -42}, {2, -42}, {0, -50}, Lighten(body, 0.3f));
    TriK(k, {-6, -41}, {-3, -43}, {-9, -49}, body);
    TriK(k, {6, -41}, {3, -43}, {9, -49}, body);
    Eye(k, -4.8f, -37.5f, 2.6f, {255, 222, 70, 255});
    Eye(k, 4.8f, -37.5f, 2.6f, {255, 222, 70, 255});
}

void Zetton(const Frame& k, float t, bool shielded) {
    const Color body{36, 36, 44, 255}, line{86, 88, 104, 255}, glow{255, 214, 70, 255};
    float step = std::sin(t * 6);
    Limb(k, -5, -14, -6 - step * 2, -1, 5, Darken(body, 0.1f));
    Limb(k, 5, -14, 6 + step * 2, -1, 5, body);
    Body(k, 0, -26, 14, 16, body);
    for (int i = 0; i < 4; ++i)
        Draw::Arc(k.P(0, -26), k.R(6 + i * 3), k.R(0.8f), PI * 0.1f, PI * 0.9f, k.C(line), 14);
    for (float side : {-1.0f, 1.0f}) {
        Limb(k, side * 12, -34, side * 18, -22, 3.6f, body);
        Limb(k, side * 18, -22, side * 15, -13, 3.2f, body);
        Body(k, side * 4.5f, -29, 3.2f, 6.5f, glow, 0, 1.0f);
    }
    for (int i = 0; i < 4; ++i) DrawCircleV(k.P(-4.5f + i * 3, -19), k.R(1.2f), k.C({255, 140, 50, 255}));
    Body(k, 0, -45, 8, 7, body);
    for (float side : {-1.0f, 1.0f}) {
        PolyK(k, {{side * 4, -50}, {side * 7, -49}, {side * 12, -60}, {side * 10, -61}}, Lighten(body, 0.1f));
    }
    float pulse = 0.6f + 0.4f * std::sin(t * 8);
    Draw::Capsule(k.P(0, -49), k.P(0, -41), k.R(1.8f), k.C(Mix(glow, WHITE, pulse * 0.5f)));
    for (int i = 0; i < 3; ++i) DrawCircleV(k.P(-3 + i * 3, -45), k.R(0.8f), k.C({255, 255, 220, 255}));
    if (shielded) {
        Vector2 c = k.P(0, -28);
        float r = k.R(32);
        std::vector<Vector2> hex;
        for (int i = 0; i < 6; ++i) hex.push_back(Add(c, Mul(Dir(PI / 6 + i * PI / 3 + t * 0.6f), r)));
        Draw::Poly(hex, WithAlpha({255, 220, 90, 255}, 0.16f * k.alpha));
        Draw::PolyLine(hex, 2.2f, WithAlpha({255, 230, 120, 255}, 0.85f * k.alpha), true);
        for (int i = 0; i < 6; ++i)
            Draw::Line(c, hex[i], 1, WithAlpha({255, 230, 120, 255}, 0.35f * k.alpha));
    }
}

void Belial(const Frame& k, float t, bool enraged) {
    const Color body{34, 28, 36, 255}, red{220, 30, 40, 255}, gold{220, 170, 60, 255};
    float step = std::sin(t * 5);
    float hover = std::sin(t * 2) * 1.5f;
    Draw::Glow(k.P(0, -30), k.R(enraged ? 46 : 38), WithAlpha({140, 20, 60, 255}, 0.35f * k.alpha));
    Limb(k, -5, -16, -6 - step * 2, -1, 4.6f, body);
    Limb(k, 5, -16, 6 + step * 2, -1, 4.6f, body);
    Draw::Line(k.P(-5, -14), k.P(-6 - step * 2, -3), k.R(1), k.C(red));
    Draw::Line(k.P(5, -14), k.P(6 + step * 2, -3), k.R(1), k.C(red));
    Body(k, 0, -29 + hover, 13, 16, body);
    PolyK(k, {{-9, -40 + hover}, {-3, -33 + hover}, {-7, -20 + hover}, {-12, -28 + hover}}, red);
    PolyK(k, {{9, -40 + hover}, {3, -33 + hover}, {7, -20 + hover}, {12, -28 + hover}}, red);
    DrawCircleV(k.P(0, -33 + hover), k.R(3.2f) + 1, k.C(kInk));
    DrawCircleV(k.P(0, -33 + hover), k.R(3.2f), k.C(Mix(red, {255, 90, 90, 255}, 0.5f + 0.5f * std::sin(t * 6))));
    Limb(k, -11, -38 + hover, -17, -25 + hover, 3.4f, body);
    Limb(k, -17, -25 + hover, -19, -16 + hover, 3, body);
    for (int i = 0; i < 3; ++i) TriK(k, {-19, -16 + hover}, {-18.0f + i, -16 + hover}, {-22 + i * 2.5f, -10 + hover}, red);
    Limb(k, 11, -38 + hover, 16, -28 + hover, 3.4f, body);
    // Giga Battlenizer
    Draw::Line(k.P(17, -54 + hover), k.P(17, -2 + hover), k.R(1.8f) + 1, k.C(kInk));
    Draw::Line(k.P(17, -54 + hover), k.P(17, -2 + hover), k.R(1.8f), k.C(gold));
    PolyK(k, {{17, -60 + hover}, {23, -54 + hover}, {17, -48 + hover}, {11, -54 + hover}}, red);
    TriK(k, {12, -55 + hover}, {8, -64 + hover}, {15, -57 + hover}, gold);
    TriK(k, {22, -55 + hover}, {26, -64 + hover}, {19, -57 + hover}, gold);
    // Head with crown crest
    auto head = Draw::EggPoints(k.P(0, -50 + hover), k.R(7.5f), k.R(9), 0.3f, 28);
    Draw::Poly(Draw::Transform(head, k.P(0, -50 + hover), k.P(0, -50 + hover), 1.15f), k.C(kInk));
    Draw::Poly(head, k.C(body));
    for (int i = -2; i <= 2; ++i)
        TriK(k, {i * 3.0f - 1.6f, -57 + hover}, {i * 3.0f + 1.6f, -57 + hover},
             {i * 3.6f, -64 - (2 - std::abs(i)) * 2.5f + hover}, i == 0 ? red : body);
    Color eye = Mix({255, 40, 40, 255}, {255, 200, 200, 255}, enraged ? 0.6f : 0.2f);
    Draw::Glow(k.P(0, -50 + hover), k.R(12), WithAlpha({255, 40, 40, 255}, 0.4f * k.alpha));
    PolyK(k, {{-6, -53 + hover}, {-1.5f, -50 + hover}, {-2, -48 + hover}, {-6.5f, -50 + hover}}, eye);
    PolyK(k, {{6, -53 + hover}, {1.5f, -50 + hover}, {2, -48 + hover}, {6.5f, -50 + hover}}, eye);
    Draw::Line(k.P(-7, -56 + hover), k.P(-2, -45 + hover), k.R(0.7f), k.C(red));
}
void KingJoe(const Frame& k, float t) {
    const Color metal{206, 194, 160, 255}, dark{120, 110, 88, 255}, lens{120, 220, 255, 255};
    float step = std::sin(t * 4);
    Limb(k, -6, -16, -7 - step * 2, -1, 5.5f, dark);
    Limb(k, 6, -16, 7 + step * 2, -1, 5.5f, metal);
    PolyK(k, {{-13, -40}, {13, -40}, {11, -16}, {-11, -16}}, kInk);
    PolyK(k, {{-12, -39}, {12, -39}, {10, -17}, {-10, -17}}, metal);
    for (int i = 0; i < 3; ++i) Draw::Line(k.P(-9, -34 + i * 6.0f), k.P(9, -34 + i * 6.0f), k.R(0.9f), k.C(dark));
    for (float side : {-1.0f, 1.0f}) {
        Limb(k, side * 13, -37, side * 19, -26, 3.8f, metal);
        Limb(k, side * 19, -26, side * 18, -17, 3.4f, dark);
    }
    Body(k, 0, -47, 9, 7.5f, metal);
    Body(k, -3.5f, -47, 3, 4, lens, 0, 0.8f);
    Body(k, 3.5f, -47, 3, 4, lens, 0, 0.8f);
    Draw::Line(k.P(0, -54), k.P(0, -60), k.R(1), k.C(dark));
    DrawCircleV(k.P(0, -61), k.R(1.6f), k.C({255, 90, 70, 255}));
}

void Metron(const Frame& k, float t) {
    const Color head{226, 74, 40, 255}, spot{255, 210, 70, 255}, limb{70, 110, 200, 255};
    float step = std::sin(t * 7);
    Limb(k, -4, -14, -5 - step * 2, -1, 3.2f, limb);
    Limb(k, 4, -14, 5 + step * 2, -1, 3.2f, limb);
    for (float side : {-1.0f, 1.0f}) Limb(k, side * 7, -30, side * 15, -22 + step * side, 2.4f, limb);
    auto shape = Draw::EggPoints(k.P(0, -34), k.R(10), k.R(22), -0.25f, 32);
    Draw::Poly(Draw::Transform(shape, k.P(0, -34), k.P(0, -34), 1.08f), k.C(kInk));
    Draw::Poly(shape, k.C(head));
    for (int i = 0; i < 6; ++i) Body(k, (i % 2 ? 4.0f : -4.0f), -48 + i * 5.0f, 2, 1.4f, spot, 0, 0.5f);
    Eye(k, 7, -44, 1.6f, {255, 240, 120, 255});
}

void Tregear(const Frame& k, float t) {
    const Color blue{38, 56, 140, 255}, black{22, 22, 34, 255}, silver{196, 204, 222, 255}, orange{255, 150, 40, 255};
    float step = std::sin(t * 5);
    Limb(k, -5, -16, -6 - step * 2, -1, 4.2f, black);
    Limb(k, 5, -16, 6 + step * 2, -1, 4.2f, blue);
    Body(k, 0, -29, 11, 15, blue);
    PolyK(k, {{0, -40}, {7, -33}, {0, -24}, {-7, -33}}, orange);
    DrawCircleV(k.P(0, -32), k.R(2.2f), k.C({255, 230, 160, 255}));
    for (float side : {-1.0f, 1.0f}) {
        Limb(k, side * 10, -38, side * 16, -27, 3.2f, black);
        Limb(k, side * 16, -27, side * 15, -18, 2.8f, blue);
        TriK(k, {side * 8, -42}, {side * 15, -40}, {side * 20, -48}, silver);
    }
    Body(k, 0, -49, 7, 8, silver);
    TriK(k, {-3, -55}, {3, -55}, {0, -64}, blue);
    PolyK(k, {{-6, -51}, {-1.5f, -48}, {-2, -46}, {-6.5f, -48}}, {255, 230, 120, 255});
    PolyK(k, {{6, -51}, {1.5f, -48}, {2, -46}, {6.5f, -48}}, {255, 230, 120, 255});
}

void Gatanothor(const Frame& k, float t, bool cracked) {
    const Color shell{92, 84, 70, 255}, ridge{150, 140, 116, 255}, flesh{64, 54, 60, 255};
    float sway = std::sin(t * 2);
    for (int i = 0; i < 6; ++i) {
        float x = -16 + i * 6.5f;
        Draw::Capsule(k.P(x, -6), k.P(x + std::sin(t * 3 + i) * 4, 0), k.R(2.2f), k.C(flesh));
    }
    Body(k, 0, -24, 22, 18, shell);
    for (int i = 0; i < 4; ++i)
        Draw::Arc(k.P(-4, -27), k.R(5 + i * 4.5f), k.R(1.2f), PI * 0.2f + i * 0.4f, PI * 1.6f + i * 0.4f, k.C(ridge), 16);
    for (int i = 0; i < 7; ++i) TriK(k, {-18 + i * 6.0f, -38}, {-14 + i * 6.0f, -38}, {-16 + i * 6.0f, -48.0f - (i % 2) * 4}, ridge);
    for (float side : {-1.0f, 1.0f}) {
        Limb(k, side * 18, -22, side * 28, -16 + sway * side, 3.6f, flesh);
        TriK(k, {side * 26, -20}, {side * 34, -22}, {side * 30, -12}, ridge);
    }
    Body(k, 8, -14, 8, 6, flesh);
    Eye(k, 6, -11, 1.8f, {255, 220, 90, 255});
    Eye(k, 11, -11, 1.8f, {255, 220, 90, 255});
    if (cracked)
        for (int i = 0; i < 3; ++i)
            Draw::Line(k.P(-10 + i * 8.0f, -38), k.P(-6 + i * 7.0f, -20), k.R(0.9f), k.C({255, 120, 60, 255}));
}
} // namespace

float KaijuHeight(EnemyId id) {
    switch (id) {
    case EnemyId::Zetton: return 62;
    case EnemyId::Belial: return 66;
    case EnemyId::RedKing: return 54;
    case EnemyId::KingJoe: return 60;
    case EnemyId::Metron: return 56;
    case EnemyId::Tregear: return 64;
    case EnemyId::Gatanothor: return 50;
    default: return 48;
    }
}

void DrawKaiju(EnemyId id, const KaijuPose& pose) {
    const Skins::KaijuSprite* sprite = Skins::Kaiju(id);
    if (!sprite) return DrawBuiltinKaiju(id, pose);
    const Texture2D& tex = sprite->texture;
    int frame = sprite->frames > 1 ? (int)(std::max(0.0f, pose.time) * sprite->fps) % sprite->frames : 0;
    float fw = tex.width / (float)sprite->frames, fh = (float)tex.height;
    float hop = sprite->frames > 1 ? 0 : std::fabs(std::sin(pose.time * 7)) * 2 * pose.scale; // still images hop
    float h = sprite->size * pose.scale, w = h * fw / fh;
    Rectangle src{frame * fw, 0, pose.face >= 0 ? fw : -fw, fh};
    Rectangle dst{pose.feet.x - w / 2, pose.feet.y - h * sprite->feet - hop, w, h};
    Vector2 center{pose.feet.x, pose.feet.y - h * 0.45f};
    float alpha = Clamp(pose.alpha, 0.0f, 1.0f);
    if (pose.enraged || pose.shielded) {
        Draw::BlendAdd();
        if (pose.enraged) Draw::Glow(center, h * 0.7f, WithAlpha({255, 60, 40, 255}, 0.35f * alpha));
        if (pose.shielded) Draw::Glow(center, h * 0.65f, WithAlpha({120, 200, 255, 255}, 0.45f * alpha));
        Draw::BlendNormal();
    }
    Color tint = WHITE;
    if (pose.chill > 0) tint = Mix(tint, {150, 210, 255, 255}, pose.chill * 0.45f);
    if (pose.burn > 0) tint = Mix(tint, {255, 150, 90, 255}, pose.burn * 0.3f);
    DrawTexturePro(tex, src, dst, {0, 0}, 0, WithAlpha(tint, alpha));
    if (pose.flash > 0) {
        Draw::BlendAdd();
        DrawTexturePro(tex, src, dst, {0, 0}, 0, WithAlpha(WHITE, pose.flash * alpha));
        Draw::BlendNormal();
    }
}

void DrawBuiltinKaiju(EnemyId id, const KaijuPose& pose) {
    Frame k{pose.feet, pose.scale, pose.face >= 0 ? 1.0f : -1.0f, pose.flash, pose.alpha, pose.chill, pose.burn};
    switch (id) {
    case EnemyId::Gomora: Gomora(k, pose.time); break;
    case EnemyId::RedKing: RedKing(k, pose.time); break;
    case EnemyId::Eleking: Eleking(k, pose.time); break;
    case EnemyId::Baltan: Baltan(k, pose.time, false); break;
    case EnemyId::BaltanClone: Baltan(k, pose.time, true); break;
    case EnemyId::Zetton: Zetton(k, pose.time, pose.shielded); break;
    case EnemyId::Belial: Belial(k, pose.time, pose.enraged); break;
    case EnemyId::KingJoe: KingJoe(k, pose.time); break;
    case EnemyId::Metron: Metron(k, pose.time); break;
    case EnemyId::Tregear: Tregear(k, pose.time); break;
    case EnemyId::Gatanothor: Gatanothor(k, pose.time, pose.enraged); break;
    default: break;
    }
}

} // namespace UG::Art
