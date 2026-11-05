#include "Characters.h"
#include "Assets.h"
#include "Draw.h"
#include <unordered_map>

namespace UQ {

const Look& LookFor(const std::string& key) {
    static const std::unordered_map<std::string, Look> looks = [] {
        std::unordered_map<std::string, Look> m;
        Look haruki; // STORAGE field jacket
        m["haruki"] = haruki;
        Look captain;
        captain.hair = {150, 150, 158, 255};
        captain.hair_style = Look::Hair::Grey;
        captain.accent = {240, 196, 80, 255};
        captain.height = 160;
        m["captain"] = captain;
        Look owner;
        owner.top = {120, 150, 190, 255};
        owner.accent = {200, 40, 40, 255};
        owner.hair_style = Look::Hair::Band;
        owner.apron = true;
        owner.legs = {60, 50, 44, 255};
        owner.height = 148;
        m["owner"] = owner;
        Look kid;
        kid.top = {220, 60, 60, 255};
        kid.accent = {255, 220, 60, 255};
        kid.hair_style = Look::Hair::Cap;
        kid.hat = {255, 210, 40, 255};
        kid.backpack = true;
        kid.legs = {50, 70, 140, 255};
        kid.height = 100;
        m["kid"] = kid;
        Look worker;
        worker.top = {255, 130, 30, 255};
        worker.accent = {230, 240, 120, 255};
        worker.hair_style = Look::Hair::Helmet;
        worker.hat = {255, 214, 40, 255};
        worker.legs = {70, 74, 84, 255};
        worker.skin = {226, 182, 140, 255};
        worker.height = 156;
        m["worker"] = worker;
        Look granny;
        granny.top = {140, 100, 160, 255};
        granny.accent = {230, 220, 240, 255};
        granny.hair = {216, 216, 222, 255};
        granny.hair_style = Look::Hair::Grey;
        granny.legs = {90, 70, 110, 255};
        granny.height = 132;
        m["granny"] = granny;
        return m;
    }();
    auto it = looks.find(key);
    return it != looks.end() ? it->second : looks.at("haruki");
}

namespace {
void Limb(Vector2 a, Vector2 b, float w, Color c) { Draw::Capsule(a, b, w * 0.5f, c); }

void Hair(const Look& look, Vector2 head, float r, int facing) {
    switch (look.hair_style) {
    // Hair stays above the eye line (head.y + 0.15 r).
    case Look::Hair::Spiky: {
        Draw::Ellipse({head.x, head.y - r * 0.5f}, r * 1.04f, r * 0.55f, look.hair);
        for (int i = -2; i <= 2; ++i) {
            float x = head.x + i * r * 0.4f - facing * r * 0.15f;
            Draw::Tri({x - r * 0.3f, head.y - r * 0.7f}, {x + r * 0.3f, head.y - r * 0.7f},
                      {x - facing * r * 0.35f, head.y - r * (1.4f - std::abs(i) * 0.12f)}, look.hair);
        }
        // Fringe and sideburns.
        Draw::Tri({head.x - r * 0.6f, head.y - r * 0.4f}, {head.x + r * 0.5f, head.y - r * 0.45f},
                  {head.x + facing * r * 0.1f, head.y - r * 0.05f}, look.hair);
        DrawRectangleRec({head.x - r * 1.0f, head.y - r * 0.5f, r * 0.22f, r * 0.6f}, look.hair);
        DrawRectangleRec({head.x + r * 0.78f, head.y - r * 0.5f, r * 0.22f, r * 0.6f}, look.hair);
        break;
    }
    case Look::Hair::Short:
    case Look::Hair::Grey:
        Draw::Ellipse({head.x - facing * r * 0.05f, head.y - r * 0.5f}, r * 1.04f, r * 0.55f, look.hair);
        DrawRectangleRec({head.x - r * 1.0f, head.y - r * 0.5f, r * 0.2f, r * 0.5f}, look.hair);
        DrawRectangleRec({head.x + r * 0.8f, head.y - r * 0.5f, r * 0.2f, r * 0.5f}, look.hair);
        break;
    case Look::Hair::Cap:
        Draw::Ellipse({head.x, head.y - r * 0.45f}, r * 1.02f, r * 0.7f, look.hat);
        Draw::Ellipse({head.x + facing * r * 0.8f, head.y - r * 0.32f}, r * 0.7f, r * 0.18f, Darken(look.hat, 0.15f));
        break;
    case Look::Hair::Helmet:
        Draw::Ellipse({head.x, head.y - r * 0.4f}, r * 1.12f, r * 0.85f, look.hat);
        DrawRectangleRec({head.x - r * 1.2f, head.y - r * 0.2f, r * 2.4f, r * 0.2f}, Darken(look.hat, 0.2f));
        DrawRectangleRec({head.x - r * 0.12f, head.y - r * 1.2f, r * 0.24f, r * 0.8f}, Lighten(look.hat, 0.3f));
        break;
    case Look::Hair::Band:
        Draw::Ellipse({head.x, head.y - r * 0.5f}, r * 1.04f, r * 0.55f, look.hair);
        DrawRectangleRec({head.x - r * 1.04f, head.y - r * 0.5f, r * 2.08f, r * 0.28f}, WHITE);
        DrawRectangleRec({head.x - r * 1.04f, head.y - r * 0.42f, r * 2.08f, r * 0.08f}, look.accent);
        break;
    }
}
} // namespace

void DrawPerson(const Look& look, Vector2 feet, int facing, float walk, float react, float pose) {
    float H = look.height;
    float s = H / 150.0f;
    float bob = std::fabs(std::sin(walk)) * 3 * s;
    float shake = react > 0 ? std::sin(react * 60) * 2.5f * react : 0;
    Vector2 hip = {feet.x + shake, feet.y - H * 0.46f - bob};
    Vector2 neck = {hip.x + facing * 2 * s, feet.y - H * 0.8f - bob};
    float headR = H * 0.11f;
    Vector2 head = {neck.x + facing * 2 * s, neck.y - headR * 1.05f};
    float swing = std::sin(walk);

    Draw::Shadow({feet.x, feet.y + 2}, 26 * s, 6 * s);
    // Legs (back leg darker).
    Color back = Darken(look.legs, 0.25f);
    Limb(hip, {hip.x - swing * 14 * s, feet.y - 4}, 15 * s, back);
    Limb(hip, {hip.x + swing * 14 * s, feet.y - 4}, 15 * s, look.legs);
    DrawRectangleRec({hip.x - swing * 14 * s - 8 * s + facing * 3, feet.y - 7 * s, 16 * s, 7 * s}, {30, 30, 36, 255});
    DrawRectangleRec({hip.x + swing * 14 * s - 8 * s + facing * 3, feet.y - 7 * s, 16 * s, 7 * s}, {40, 40, 46, 255});
    // Back arm.
    Vector2 shoulderB = {neck.x - facing * 10 * s, neck.y + 8 * s};
    Limb(shoulderB, {shoulderB.x + swing * 12 * s, hip.y + 6 * s}, 12 * s, Darken(look.top, 0.3f));
    if (look.backpack)
        DrawRectangleRounded({neck.x - facing * 26 * s - 12 * s, neck.y + 4 * s, 24 * s, 34 * s}, 0.3f, 4, look.accent);
    // Torso.
    Vector2 a = {neck.x - 17 * s, neck.y + 2}, b = {neck.x + 17 * s, neck.y + 2};
    Vector2 c = {hip.x + 15 * s, hip.y + 4}, d = {hip.x - 15 * s, hip.y + 4};
    Draw::Quad(a, b, c, d, look.top);
    if (look.apron) Draw::Quad({neck.x - 11 * s, neck.y + 14 * s}, {neck.x + 11 * s, neck.y + 14 * s},
                               {hip.x + 15 * s, hip.y + 18 * s}, {hip.x - 15 * s, hip.y + 18 * s}, {250, 250, 246, 255});
    else Draw::Line({neck.x, neck.y + 4}, {hip.x, hip.y}, 4 * s, look.accent);
    DrawRectangleRec({hip.x - 15 * s, hip.y - 2 * s, 30 * s, 5 * s}, Darken(look.top, 0.4f));
    // Front arm (raised for the Z Riser).
    Vector2 shoulderF = {neck.x + facing * 10 * s, neck.y + 8 * s};
    Vector2 hand = pose > 0 ? Vector2{shoulderF.x + facing * 18 * s, shoulderF.y - 40 * s * pose}
                            : Vector2{shoulderF.x - swing * 12 * s + facing * 3 * s, hip.y + 6 * s};
    Limb(shoulderF, hand, 12 * s, look.top);
    DrawCircleV(hand, 5 * s, look.skin);
    if (pose > 0) {
        Draw::BlendAdd();
        Draw::Glow(hand, 26 * s * pose, WithAlpha({120, 200, 255, 255}, 0.7f * pose));
        Draw::BlendNormal();
        DrawRectanglePro({hand.x, hand.y - 4 * s, 9 * s, 26 * s}, {4.5f * s, 22 * s}, facing * 12.0f, {200, 210, 230, 255});
    }
    // Head.
    DrawRectangleRec({neck.x - 4 * s, neck.y - 6 * s, 8 * s, 10 * s}, look.skin);
    DrawCircleV(head, headR, look.skin);
    Hair(look, head, headR, facing);
    DrawCircleV({head.x + facing * headR * 0.45f, head.y + headR * 0.18f}, headR * 0.12f, {30, 30, 40, 255});
    DrawCircleV({head.x + facing * headR * 0.05f, head.y + headR * 0.18f}, headR * 0.1f, {30, 30, 40, 255});
}

void DrawPortrait(const std::string& key, Rectangle r) {
    DrawRectangleRounded(r, 0.18f, 8, {22, 34, 66, 255});
    if (key == "z" || key == "redking") {
        auto tex = key == "z" ? Avatar("z") : TextureHandle{};
        if (key == "redking") {
            if (const Pose* pose = Poses::Get("rk_idle")) tex = pose->texture;
        }
        if (auto* t = tex.Get()) {
            Rectangle src = key == "z" ? Rectangle{0, 0, (float)t->width, (float)t->height}
                                       : Rectangle{t->width * 0.2f, 0, t->width * 0.6f, t->width * 0.6f};
            Display::ClipScope clip(r);
            DrawTexturePro(*t, src, r, {0, 0}, 0, WHITE);
        }
        DrawRectangleRoundedLinesEx(r, 0.18f, 8, 2, WithAlpha(Palette::cyan, 0.7f));
        return;
    }
    const Look& look = LookFor(key);
    float s = r.width / 150.0f;
    Vector2 head = {r.x + r.width * 0.5f, r.y + r.height * 0.44f};
    float headR = 34 * s;
    // Shoulders and collar.
    Draw::Ellipse({head.x, r.y + r.height + 10 * s}, 66 * s, 50 * s, look.top);
    if (look.apron) Draw::Ellipse({head.x, r.y + r.height + 18 * s}, 30 * s, 40 * s, {250, 250, 246, 255});
    else Draw::Tri({head.x - 14 * s, r.y + r.height - 34 * s}, {head.x + 14 * s, r.y + r.height - 34 * s},
                   {head.x, r.y + r.height - 6 * s}, look.accent);
    DrawRectangleRec({head.x - 10 * s, head.y + headR * 0.7f, 20 * s, 22 * s}, Darken(look.skin, 0.08f));
    DrawCircleV(head, headR, look.skin);
    Hair(look, head, headR, 1);
    // Face: eyes, brows, mouth.
    for (int side : {-1, 1}) {
        Vector2 eye = {head.x + side * headR * 0.38f, head.y + headR * 0.2f};
        DrawEllipse((int)eye.x, (int)eye.y, (int)(headR * 0.11f), (int)(headR * 0.16f), {30, 30, 40, 255});
        DrawCircleV({eye.x + headR * 0.03f, eye.y - headR * 0.05f}, headR * 0.04f, WHITE);
        Draw::Line({eye.x - headR * 0.16f, eye.y - headR * 0.3f}, {eye.x + headR * 0.16f, eye.y - headR * 0.34f},
                   3 * s, Darken(look.hair, 0.2f));
    }
    Draw::Arc({head.x, head.y + headR * 0.5f}, headR * 0.22f, 3 * s, 0.25f, PI - 0.25f, {150, 70, 70, 255});
    DrawCircleV({head.x - headR * 0.58f, head.y + headR * 0.45f}, headR * 0.12f, WithAlpha({255, 120, 120, 255}, 0.35f));
    DrawCircleV({head.x + headR * 0.58f, head.y + headR * 0.45f}, headR * 0.12f, WithAlpha({255, 120, 120, 255}, 0.35f));
    DrawRectangleRoundedLinesEx(r, 0.18f, 8, 2, WithAlpha(Palette::line, 1));
}

} // namespace UQ
