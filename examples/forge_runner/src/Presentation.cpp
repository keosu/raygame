#include "Presentation.h"

namespace Forge {
namespace {
struct SectorStyle {
    Color sky, horizon, farMetal, nearMetal, edge, glow;
};
SectorStyle Style(int theme) {
    if (theme == 1)
        return {{7, 24, 43, 255},  {18, 67, 82, 255},   {17, 44, 65, 255},
                {31, 80, 95, 255}, {98, 220, 238, 255}, {67, 196, 234, 255}};
    if (theme == 2)
        return {{21, 10, 40, 255}, {53, 24, 66, 255},    {39, 25, 62, 255},
                {69, 45, 88, 255}, {235, 193, 108, 255}, {186, 112, 255, 255}};
    return {{27, 16, 22, 255}, {77, 42, 32, 255},   {46, 31, 34, 255},
            {79, 49, 42, 255}, {242, 167, 91, 255}, {255, 124, 57, 255}};
}
bool HasUnicode(const std::string& text) {
    return std::any_of(text.begin(), text.end(), [](unsigned char c) { return c >= 128; });
}
// Noto's ascender/descender box is taller than its CJK em square. Compensate so
// a requested 24px label has a readable 24px character body, with a 16px floor.
float TextSize(const std::string& text, float size) {
    return HasUnicode(text) ? std::max(16.0f, size) * 1.4f : size;
}
} // namespace
float Presentation::Random(float min, float max) {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return min + (max - min) * (random % 10000) / 10000.0f;
}
std::string Presentation::Localize(const std::string& value) const {
    std::string key = value;
    if (touchControls) {
        if (value == "A / D TO MOVE")
            key = "TOUCH ARROWS TO MOVE";
        if (value == "SPACE TO JUMP / HOLD FOR HEIGHT")
            key = "TOUCH JUMP / HOLD FOR HEIGHT";
        if (value == "SHIFT TO DASH")
            key = "TOUCH DASH";
    }
    return catalog.Translate(key, settings.language);
}
void Presentation::Init(const std::string& assets) {
    catalog.Load(assets + "/locales/zh-CN.tsv");
    chineseGlyphs = catalog.ChineseCodepoints();
    fontDirectory = assets + "/fonts/";
    ChineseFont(15);
    ChineseFont(24, true);
    body = RayGame::ResourceManager::LoadFontResource("forge-body",
                                                      assets + "/fonts/Barlow-Medium.ttf", 72);
    title = RayGame::ResourceManager::LoadFontResource(
        "forge-title", assets + "/fonts/Rajdhani-SemiBold.ttf", 128);
    if (!body || !title)
        throw std::runtime_error("FORGE fonts are missing from the assets directory");
    SetTextureFilter(body->texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(title->texture, TEXTURE_FILTER_BILINEAR);
    glow = RayGame::ResourceManager::LoadTexture("forge-glow", assets + "/glow.png");
    if (!glow)
        throw std::runtime_error("FORGE lighting texture is missing");
    particles.reserve(512);
}
float Presentation::Measure(const std::string& value, float size, bool heading) const {
    auto text = Localize(value);
    size = TextSize(text, size);
    auto font = FontFor(text, size, heading);
    return font ? MeasureTextEx(*font, text.c_str(), size, HasUnicode(text) ? 0.4f : 1.0f).x
                : (float)MeasureText(text.c_str(), (int)size);
}
void Presentation::Text(const std::string& value, Vector2 position, float size, Color color,
                        bool heading) {
    auto text = Localize(value);
    if (HasUnicode(text))
        position.y -= std::max(16.0f, size) * 0.18f;
    size = TextSize(text, size);
    auto font = FontFor(text, size, heading);
    if (font)
        DrawTextEx(*font, text.c_str(), position, size, HasUnicode(text) ? 0.4f : 1.0f, color);
    else
        DrawText(text.c_str(), (int)position.x, (int)position.y, (int)size, color);
}
RayGame::FontHandle Presentation::ChineseFont(float size, bool heading) const {
    int rasterSize = (int)std::ceil(size * 2);
    auto key = std::make_pair(rasterSize, heading);
    auto& font = chineseFonts[key];
    if (!font) {
        font = RayGame::ResourceManager::LoadFontResource(
            "forge-sc-" + std::to_string(rasterSize) + (heading ? "-title" : "-body"),
            fontDirectory + (heading ? "ForgeSC-SemiBold.ttf" : "ForgeSC-Medium.ttf"), rasterSize,
            chineseGlyphs);
        if (!font)
            throw std::runtime_error("FORGE SC fonts are missing or invalid");
        SetTextureFilter(font->texture, TEXTURE_FILTER_BILINEAR);
    }
    return font;
}
RayGame::FontHandle Presentation::FontFor(const std::string& text, float size, bool heading) const {
    return HasUnicode(text) ? ChineseFont(size, heading) : (heading ? title : body);
}
void Presentation::Glow(Vector2 p, float radius, Color color, float alpha) {
    if (!glow)
        return;
    DrawTexturePro(*glow, {0, 0, (float)glow->width, (float)glow->height},
                   {p.x - radius, p.y - radius, radius * 2, radius * 2}, {0, 0}, 0,
                   Fade(color, alpha));
}
void Presentation::Burst(Vector2 p, Color c, int count, float speed) {
    for (int i = 0; i < count && particles.size() < 512; ++i) {
        float angle = Random(0, 2 * PI), velocity = Random(speed * 0.3f, speed);
        particles.push_back({p,
                             {std::cos(angle) * velocity, std::sin(angle) * velocity},
                             0,
                             Random(0.3f, 0.85f),
                             Random(1, 3),
                             c});
    }
}
void Presentation::React(unsigned event, Vector2 p) {
    if (event & Jump)
        Burst(p, Muted, 12, 100);
    if (event & Dash) {
        Burst({p.x, p.y - 20}, Mint, 24, 150);
        shake = std::max(shake, 2.0f);
    }
    if (event & Land) {
        Burst(p, Ember, 14, 130);
        landing = 0.18f;
        shake = std::max(shake, 1.6f);
    }
    if (event & Cell) {
        Burst({p.x, p.y - 24}, Mint, 32, 170);
        flash = 0.04f;
    }
    if (event & Checkpoint) {
        Burst(p, Mint, 45, 230);
        flash = 0.08f;
    }
    if (event & Death) {
        Burst({p.x, p.y - 22}, Red, 60, 300);
        shake = 9;
        flash = 0.13f;
    }
    if (event & Finish) {
        Burst({p.x, p.y - 36}, Mint, 90, 330);
        flash = 0.15f;
    }
}
void Presentation::Update(float dt) {
    clock += dt;
    shake = std::max(0.0f, shake - dt * 22);
    flash = std::max(0.0f, flash - dt);
    landing = std::max(0.0f, landing - dt);
    for (auto& p : particles) {
        p.age += dt;
        p.velocity.y += 200 * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Spark& p) { return p.age >= p.life; }),
                    particles.end());
}
void Presentation::Follow(const World& w, Vector2 p, float dt, bool snap) {
    float width = RayGame::Display::GetWidth(), height = RayGame::Display::GetHeight();
    camera.offset = {width / 2, height * 0.52f};
    Vector2 destination{w.level.size.x <= width ? w.level.size.x / 2
                                                : Clamp(p.x + w.player.velocity.x * 0.25f,
                                                        width / 2, w.level.size.x - width / 2),
                        w.level.size.y <= height
                            ? w.level.size.y / 2
                            : Clamp(p.y - 120, height * 0.52f, w.level.size.y - height * 0.48f)};
    camera.target =
        snap ? destination : Vector2Lerp(camera.target, destination, 1 - std::exp(-dt * 5));
}
void Presentation::Background(int theme) {
    float width = RayGame::Display::GetWidth(), height = RayGame::Display::GetHeight();
    auto style = Style(theme);
    DrawRectangleGradientV(0, 0, (int)width, (int)height, style.sky, style.horizon);
    if (theme == 2) {
        // A monumental reactor, orbital rings and suspended radial machinery.
        Vector2 core{width * 0.62f - std::fmod(camera.target.x * 0.06f, 180.0f), height * 0.44f};
        Glow(core, 430, style.glow, 0.3f);
        for (int ring = 0; ring < 4; ++ring) {
            float radius = 105 + ring * 66.0f;
            DrawRing(core, radius, radius + 4, 0, 360, 96, Fade(style.edge, 0.12f + ring * 0.02f));
        }
        for (int arm = 0; arm < 16; ++arm) {
            float angle = arm * 2 * PI / 16 + clock * 0.025f;
            Vector2 direction{std::cos(angle), std::sin(angle)};
            DrawLineEx(Vector2Add(core, Vector2Scale(direction, 120)),
                       Vector2Add(core, Vector2Scale(direction, 300)), 9, style.farMetal);
            DrawCircleV(Vector2Add(core, Vector2Scale(direction, 244)), 5, Fade(style.edge, 0.6f));
        }
        DrawPoly(core, 6, 92, clock * 2, style.farMetal);
        DrawPolyLinesEx(core, 6, 82, clock * 2, 3, style.edge);
        Glow(core, 115, style.glow, 0.45f);
        for (int i = -1; i < (int)(width / 390) + 2; ++i) {
            float x = i * 390.0f - std::fmod(camera.target.x * 0.22f, 390.0f);
            DrawRectangleRec({x, 0, 30, height}, style.nearMetal);
            DrawRectangleRec({x + 10, 0, 3, height}, Fade(style.glow, 0.28f));
            DrawLineEx({x + 30, 90}, {x + 135, 0}, 18, style.nearMetal);
            DrawLineEx({x + 30, height - 70}, {x + 125, height}, 18, style.nearMetal);
        }
    } else if (theme == 1) {
        // Cooling towers and falling coolant distinguish the conduit silhouette.
        for (int layer = 0; layer < 2; ++layer) {
            float spacing = layer == 0 ? 430.0f : 610.0f;
            float offset = std::fmod(camera.target.x * (0.08f + layer * 0.15f), spacing);
            for (int i = -1; i < (int)(width / spacing) + 2; ++i) {
                float x = i * spacing - offset, top = height * (0.18f + layer * 0.12f);
                Color metal = layer == 0 ? style.farMetal : style.nearMetal;
                DrawRectangleRounded({x + 30, top, 160, height}, 0.15f, 12, metal);
                DrawRectangleRec({x + 52, top + 35, 116, height}, Fade(Ink, 0.5f));
                for (int pipe = 0; pipe < 4; ++pipe) {
                    float px = x + 63 + pipe * 27.0f;
                    DrawRectangleRec({px, top + 45, 9, height}, Fade(style.glow, 0.12f));
                    float y = top + 45 + std::fmod(clock * (38 + pipe * 9) + pipe * 64, 290.0f);
                    DrawRectangleRec({px, y, 9, 44}, Fade(style.edge, 0.38f));
                }
                for (int band = 0; band < 5; ++band)
                    DrawRectangleRec({x + 25, top + band * 115, 170, 12}, metal);
                DrawLineEx({x + 110, top}, {x + 110, top - 42}, 18, metal);
                DrawLineEx({x + 110, top - 42}, {x + spacing + 110, top - 42}, 18, metal);
                DrawCircleV({x + 242, top - 42}, 19, metal);
                DrawCircleLines((int)(x + 242), (int)(top - 42), 12, style.edge);
            }
        }
        for (int i = 0; i < 7; ++i) {
            float y = height * 0.78f + i * 18;
            DrawLineEx({0, y}, {width, y}, 1, Fade(style.edge, 0.05f));
        }
    } else {
        // Warm foundry: roof trusses, suspended crucibles, furnace mouths.
        for (int layer = 0; layer < 2; ++layer) {
            float spacing = 460 + layer * 100.0f;
            float offset = std::fmod(camera.target.x * (0.1f + layer * 0.13f), spacing);
            for (int i = -1; i < (int)(width / spacing) + 2; ++i) {
                float x = i * spacing - offset, top = height * (0.19f + layer * 0.18f);
                Color metal = layer == 0 ? style.farMetal : style.nearMetal;
                DrawRectangleRec({x + 40, top, 225, height}, metal);
                DrawRectangleRec({x + 72, top + 68, 162, 210}, Fade(Ink, 0.8f));
                DrawRectangleGradientV((int)x + 85, (int)top + 90, 136, 174,
                                       Fade(style.glow, 0.12f), Fade(style.glow, 0.5f));
                for (int rib = 0; rib < 5; ++rib)
                    DrawRectangleRec({x + 85 + rib * 31.0f, top + 82, 7, 196}, metal);
                DrawRectangleRec({x + 54, top - 10, 197, 16}, metal);
                DrawLineEx({x + 305, 0}, {x + 305, top + 80}, 4, metal);
                DrawRectangleRounded({x + 280, top + 80, 72, 94}, 0.3f, 8, metal);
                DrawRectangleRec({x + 285, top + 93, 62, 7}, style.edge);
                Glow({x + 165, top + 218}, 160, style.glow, 0.2f);
                DrawLineEx({x, top - 45}, {x + spacing, top - 45}, 8, metal);
                DrawLineEx({x, top - 45}, {x + 200, top - 160}, 6, metal);
            }
        }
    }
    // Each sector has its own atmospheric motion.
    for (int i = 0; i < 32; ++i) {
        float x = std::fmod(i * 97.7f + clock * (7 + i % 3) + width, width);
        float direction = theme == 1 ? 1.0f : -1.0f;
        float y = std::fmod(i * 73.1f + clock * (12 + i % 7) * direction + height * 100, height);
        if (theme == 1)
            DrawLineEx({x, y}, {x, y + 7}, 1, Fade(style.edge, 0.17f));
        else
            DrawCircleV({x, y}, i % 4 == 0 ? 2.0f : 1.0f, Fade(style.edge, 0.25f));
    }
    DrawRectangleGradientV(0, 0, (int)width, 120, Fade(Ink, 0.65f), Fade(Ink, 0));
}
void Presentation::DrawPlatform(const Platform& p, float time, int theme) {
    auto r = p.At(time);
    auto style = Style(theme);
    Color edge = p.oneWay ? Mint : style.edge;
    float halfWidth = RayGame::Display::GetWidth() / 2 + 80;
    if (r.x + r.width < camera.target.x - halfWidth || r.x > camera.target.x + halfWidth)
        return;
    if (p.oneWay) {
        DrawRectangleRec({r.x, r.y, r.width, r.height}, {45, 63, 65, 255});
        DrawRectangleRec({r.x, r.y, r.width, 3}, Fade(Mint, 0.8f));
        DrawRectangleRec({r.x + 6, r.y + 5, r.width - 12, 4}, {19, 33, 39, 255});
        for (float x = r.x + 12; x < r.x + r.width - 8; x += 24)
            DrawLineEx({x, r.y + 12}, {x + 12, r.y + 5}, 2, Muted);
        if (p.travel.x || p.travel.y) {
            DrawLineEx({p.rect.x + p.rect.width / 2, p.rect.y - 230},
                       {p.rect.x + p.rect.width / 2, p.rect.y + 60}, 3, Fade(Muted, 0.3f));
            Glow({r.x + r.width / 2, r.y + 15}, 65, Mint, 0.12f);
        }
        return;
    }
    DrawRectangleRec(r, style.nearMetal);
    DrawRectangleRec({r.x, r.y + 12, r.width, std::max(0.0f, r.height - 12)}, style.farMetal);
    if (theme == 1) {
        for (float x = r.x + 16; x < r.x + r.width - 12; x += 56)
            DrawRectangleRec({x, r.y + 20, 5, std::min(65.0f, std::max(0.0f, r.height - 28))},
                             Fade(style.glow, 0.5f));
    } else if (theme == 2) {
        for (float x = r.x + 42; x < r.x + r.width - 30; x += 112)
            DrawPolyLinesEx({x, r.y + 51}, 6, 22, 30, 2, Fade(style.edge, 0.45f));
    }
    for (float x = r.x + 8; x < r.x + r.width - 20; x += 80) {
        float w = std::min(72.0f, r.x + r.width - x - 4);
        DrawRectangleLinesEx({x, r.y + 30, w, 104}, 1, {44, 59, 62, 255});
        DrawCircleV({x + 5, r.y + 36}, 2, Muted);
        DrawCircleV({x + w - 5, r.y + 124}, 2, Fade(Muted, 0.5f));
        for (int k = 0; k < 4; ++k)
            DrawLineEx({x + 16, r.y + 54 + k * 12.0f}, {x + w - 14, r.y + 54 + k * 12.0f}, 2,
                       {17, 27, 33, 255});
    }
    DrawRectangleRec({r.x, r.y, r.width, 4}, edge);
    DrawRectangleRec({r.x, r.y + 4, r.width, 8}, {53, 65, 57, 255});
    for (float x = r.x; x + 20 < r.x + r.width; x += 40)
        DrawLineEx({x + 6, r.y + 9}, {x + 18, r.y + 4}, 3, {23, 32, 32, 255});
    DrawRectangleRec({r.x, r.y + 12, r.width, 2}, Ink);
    if (r.height > 150) {
        for (float x = r.x + 150; x < r.x + r.width - 60; x += 250) {
            Glow({x, r.y + 55}, 80, style.glow, 0.13f);
            DrawRectangleRec({x - 30, r.y + 26, 60, 4}, Fade(style.glow, 0.7f));
        }
    }
}
void Presentation::DrawRunner(Vector2 p, const Runner& s, float scale, float alpha) {
    float speed = std::min(1.0f, std::fabs(s.velocity.x) / 300);
    float stride = s.grounded ? std::sin(clock * 17) * 9 * speed : 5;
    float bob = s.grounded ? std::fabs(std::cos(clock * 17)) * 1.5f * speed : -1;
    float crouch = landing > 0 ? 3.0f : 0.0f;
    auto q = [&](float x, float y) {
        return Vector2{p.x + x * scale * s.facing, p.y + (y + bob + crouch) * scale};
    };
    auto line = [&](Vector2 a, Vector2 b, float width, Color color) {
        DrawLineEx(q(a.x, a.y), q(b.x, b.y), width * scale, Fade(color, alpha));
    };
    auto circle = [&](float x, float y, float radius, Color color) {
        DrawCircleV(q(x, y), radius * scale, Fade(color, alpha));
    };
    auto box = [&](float x, float y, float w, float h, Color color) {
        auto center = q(x, y);
        DrawRectangleRounded(
            {center.x - w * scale / 2, center.y - h * scale / 2, w * scale, h * scale}, 0.25f, 8,
            Fade(color, alpha));
    };
    auto triangle = [&](Vector2 a, Vector2 b, Vector2 c, Color color) {
        a = q(a.x, a.y);
        b = q(b.x, b.y);
        c = q(c.x, c.y);
        if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
            std::swap(b, c);
        DrawTriangle(a, b, c, Fade(color, alpha));
    };
    Color outline{10, 22, 30, 255}, armor{226, 218, 190, 255}, shadow{93, 122, 128, 255};
    Color suit{39, 64, 77, 255}, visor{57, 173, 180, 255};
    if (s.grounded)
        DrawEllipse((int)p.x, (int)(p.y + 2 * scale), 19 * scale, 3 * scale,
                    Fade(Ink, alpha * 0.45f));
    if (s.dashTime > 0) {
        Glow(q(-28, -24), 65, Mint, alpha * 0.22f);
        for (int i = 0; i < 4; ++i)
            line({-22.0f - i * 14, -22.0f + i * 3}, {-50.0f - i * 14, -22.0f + i * 3}, 3, Mint);
    }
    // Long scarf and the courier's illuminated delivery pack.
    float flutter = std::sin(clock * 8) * 3;
    triangle({-4, -35}, {-39, -38 + flutter}, {-24, -25 + flutter}, outline);
    triangle({-6, -34}, {-36, -36 + flutter}, {-24, -28 + flutter}, Red);
    triangle({-24, -28 + flutter}, {-36, -36 + flutter}, {-42, -29 + flutter}, Ember);
    box(-13, -26, 14, 24, outline);
    box(-14, -27, 10, 19, shadow);
    box(-15, -27, 5, 12, Mint);
    line({-18, -33}, {-18, -22}, 2, Light);
    // Separate knee joints and oversized boots make motion readable at game scale.
    for (int leg = 0; leg < 2; ++leg) {
        float side = leg == 0 ? -1.0f : 1.0f, step = side * stride;
        Color plate = leg == 0 ? shadow : armor;
        line({side * 5, -18}, {side * 6 + step * 0.4f, -10}, 8, outline);
        line({side * 6 + step * 0.4f, -10}, {side * 7 - step, -3}, 8, outline);
        line({side * 5, -18}, {side * 6 + step * 0.4f, -10}, 5, suit);
        circle(side * 6 + step * 0.4f, -10, 3, plate);
        line({side * 6 + step * 0.4f, -8}, {side * 7 - step, -3}, 5, plate);
        box(side * 7 - step + 2, -2, 12, 6, outline);
        box(side * 7 - step + 2, -3, 10, 3, plate);
    }
    box(0, -26, 27, 25, outline);
    box(0, -27, 22, 20, armor);
    box(0, -26, 15, 13, suit);
    box(0, -27, 9, 7, shadow);
    box(1, -27, 4, 5, Mint);
    line({-9, -18}, {9, -18}, 3, Ember);
    line({-10, -31}, {-15, -21}, 8, outline);
    line({-10, -31}, {-15, -21}, 5, shadow);
    line({10, -30}, {15, -22 + stride * 0.25f}, 8, outline);
    line({10, -30}, {15, -22 + stride * 0.25f}, 5, armor);
    circle(15, -22 + stride * 0.25f, 3, suit);
    // Helmet shell, inset glass, earpiece and a small forward headlamp.
    circle(0, -42, 14, outline);
    circle(0, -43, 12, armor);
    circle(-3, -46, 7, {247, 237, 207, 255});
    box(4, -42, 19, 12, outline);
    box(5, -42, 16, 9, visor);
    line({0, -45}, {10, -45}, 2, Mint);
    line({7, -42}, {10, -42}, 2, Light);
    circle(-10, -42, 4, outline);
    circle(-10, -42, 2, shadow);
    box(6, -53, 8, 5, outline);
    box(7, -53, 5, 3, Ember);
    Glow(q(7, -42), 20, Mint, 0.12f * alpha);
}
void Presentation::DrawWorld(const World& w, Vector2 runner, bool menu) {
    Background(w.level.index);
    Camera2D view = camera;
    if (settings.shake && !menu) {
        view.offset.x += std::sin(clock * 111) * shake;
        view.offset.y += std::cos(clock * 97) * shake * 0.6f;
    }
    BeginMode2D(view);
    for (const auto& a : w.level.airflows) {
        auto r = a.rect;
        Color air{134, 224, 255, 255};
        DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade(air, 0.02f),
                               Fade(air, 0.16f));
        DrawRectangleRec({r.x, r.y + r.height - 12, r.width, 12}, Steel);
        for (float x = r.x + 18; x < r.x + r.width - 10; x += 34) {
            DrawCircleLines((int)x, (int)(r.y + r.height - 5), 10, air);
            float blade = w.time * 15;
            DrawLineEx({x - std::cos(blade) * 9, r.y + r.height - 5 - std::sin(blade) * 5},
                       {x + std::cos(blade) * 9, r.y + r.height - 5 + std::sin(blade) * 5}, 2, air);
            for (int n = 0; n < 5; ++n) {
                float y = r.y + r.height - std::fmod(w.time * 150 + n * 83 + x, r.height);
                DrawLineEx({x, y + 10}, {x, y - 10}, 1, Fade(air, 0.45f));
                DrawLineEx({x - 4, y - 4}, {x, y - 10}, 1, air);
                DrawLineEx({x + 4, y - 4}, {x, y - 10}, 1, air);
            }
        }
    }
    for (size_t i = 0; i < w.level.platforms.size(); ++i) {
        const auto& p = w.level.platforms[i];
        auto r = p.At(w.time);
        if (!w.PlatformActive(i)) {
            DrawRectangleLinesEx(r, 1, Fade(Ember, 0.28f));
            float recovery =
                Clamp((w.platformTimers[i] - p.breakDelay) / p.rebuildDelay, 0.0f, 1.0f);
            DrawRectangleRec({r.x, r.y + r.height / 2, r.width * recovery, 2}, Fade(Mint, 0.65f));
            continue;
        }
        DrawPlatform(p, w.time, w.level.index);
        if (p.surface == Surface::Ice) {
            Color ice{163, 237, 255, 255};
            DrawRectangleRec({r.x, r.y, r.width, 8}, ice);
            DrawRectangleGradientV((int)r.x, (int)r.y + 8, (int)r.width, 22, Fade(ice, 0.65f),
                                   Fade(ice, 0));
            for (float x = r.x + 16; x < r.x + r.width - 12; x += 43) {
                DrawLineEx({x, r.y + 2}, {x + 8, r.y + 15}, 2, WHITE);
                DrawLineEx({x + 8, r.y + 15}, {x + 5, r.y + 23}, 1, ice);
            }
        } else if (p.surface == Surface::Conveyor) {
            DrawRectangleRec({r.x, r.y, r.width, 20}, Ink);
            DrawLineEx({r.x, r.y}, {r.x + r.width, r.y}, 3, Ember);
            float offset = std::fmod(w.time * p.beltSpeed, 36.0f);
            if (offset < 0)
                offset += 36;
            float direction = p.beltSpeed > 0 ? 1.0f : -1.0f;
            for (float x = r.x + offset + 12; x < r.x + r.width - 12; x += 36) {
                DrawLineEx({x - direction * 6, r.y + 5}, {x + direction * 6, r.y + 10}, 2, Ember);
                DrawLineEx({x - direction * 6, r.y + 15}, {x + direction * 6, r.y + 10}, 2, Ember);
            }
            DrawCircleV({r.x + 8, r.y + 11}, 5, Muted);
            DrawCircleV({r.x + r.width - 8, r.y + 11}, 5, Muted);
        } else if (p.surface == Surface::Fragile) {
            float load = std::max(0.0f, w.platformTimers[i]) / p.breakDelay;
            DrawRectangleRec({r.x, r.y, r.width, 4}, load > 0 ? Red : Ember);
            for (float x = r.x + 20; x < r.x + r.width - 15; x += 32) {
                DrawLineEx({x, r.y + 2}, {x + 7, r.y + 9}, 2, Ink);
                DrawLineEx({x + 7, r.y + 9}, {x + 2, r.y + 19}, 2, Ink);
            }
            if (load > 0) {
                DrawRectangleRec({r.x, r.y - 7, r.width * (1 - load), 3}, Red);
                Glow({r.x + r.width / 2, r.y}, 80, Red, load * 0.2f);
            }
        }
    }
    for (auto h : w.level.hazards) {
        DrawRectangleRec(h, {71, 34, 28, 255});
        for (float x = h.x; x < h.x + h.width; x += 16)
            DrawRectangleRec({x, h.y, 8, h.height},
                             Fade(Ember, 0.3f + std::sin(clock * 3 + x) * 0.1f));
        Glow({h.x + h.width / 2, h.y}, h.width, Ember, 0.16f);
    }
    for (const auto& s : w.level.signs) {
        Text(s.text, s.position, 13, Fade(Muted, 0.75f));
        DrawLineEx({s.position.x, s.position.y + 23}, {s.position.x + 120, s.position.y + 23}, 1,
                   Fade(Muted, 0.25f));
    }
    for (size_t i = 0; i < w.level.checkpoints.size(); ++i) {
        auto p = w.level.checkpoints[i];
        bool active = (int)i <= w.checkpoint;
        DrawRectangleRec({p.x - 8, p.y - 76, 16, 76}, Steel);
        DrawRectangleRec({p.x - 19, p.y - 7, 38, 7}, Muted);
        DrawRectangleRec({p.x - 6, p.y - 70, 12, 32}, active ? Mint : Ember);
        Glow({p.x, p.y - 52}, 70, active ? Mint : Ember, active ? 0.22f : 0.1f);
        if (active) {
            DrawCircleLines((int)p.x, (int)(p.y - 52), 25, Fade(Mint, 0.3f));
        }
    }
    for (size_t i = 0; i < w.level.cells.size(); ++i)
        if (!(w.collected & (std::uint64_t{1} << i))) {
            auto p = w.level.cells[i];
            p.y += std::sin(clock * 3 + i) * 5;
            Glow(p, 65, Mint, 0.22f);
            DrawCircleLines((int)p.x, (int)p.y, 21, Fade(Mint, 0.3f));
            DrawPoly(p, 4, 13, 45, {42, 83, 72, 255});
            DrawPoly(p, 4, 8, 45, Mint);
            DrawCircleV(p, 3, Light);
        }
    for (auto l : w.level.lasers) {
        bool active = l.Active(w.time);
        auto r = l.rect;
        DrawRectangleRec({r.x - 10, r.y - 12, r.width + 20, 14}, Steel);
        DrawRectangleRec({r.x - 10, r.y + r.height - 2, r.width + 20, 12}, Steel);
        if (active) {
            DrawRectangleRec({r.x - 9, r.y, r.width + 18, r.height}, Fade(Red, 0.12f));
            DrawRectangleRec({r.x + 5, r.y, r.width - 10, r.height}, Red);
            DrawLineEx({r.x + r.width / 2, r.y}, {r.x + r.width / 2, r.y + r.height}, 2, Light);
            Glow({r.x + r.width / 2, r.y + r.height / 2}, 110, Red, 0.12f);
        } else {
            for (float y = r.y; y < r.y + r.height; y += 18)
                DrawRectangleRec({r.x + 7, y, 2, 7}, Fade(Red, 0.25f));
        }
        DrawCircleV({r.x + r.width / 2, r.y - 7}, 3, active ? Red : Mint);
    }
    for (auto d : w.level.drones) {
        auto p = d.At(w.time);
        Glow(p, 65, Red, 0.17f);
        DrawCircleV(p, 22, Steel);
        DrawCircleLines((int)p.x, (int)p.y, 23, Muted);
        DrawLineEx({p.x - 35, p.y - 2}, {p.x + 35, p.y - 2}, 6, {47, 67, 72, 255});
        DrawCircleV(p, 8, Red);
        DrawCircleV({p.x + 2, p.y - 2}, 3, Light);
        DrawLineEx({p.x - 32, p.y - 10}, {p.x - 18, p.y - 10}, 2, Mint);
        DrawLineEx({p.x + 18, p.y - 10}, {p.x + 32, p.y - 10}, 2, Mint);
    }
    for (const auto& c : w.level.crawlers) {
        auto p = c.At(w.time);
        DrawRectangleRounded({p.x - 24, p.y - 12, 48, 25}, 0.3f, 6, Steel);
        DrawRectangleRec({p.x - 21, p.y - 10, 42, 6}, Ember);
        for (int k = -1; k <= 1; ++k) {
            float x = p.x + k * 16;
            DrawCircleV({x, p.y + 10}, 6, Ink);
            DrawCircleLines((int)x, (int)(p.y + 10), 5, Muted);
            DrawLineEx({x, p.y + 10},
                       {x + std::cos(w.time * 12) * 4, p.y + 10 + std::sin(w.time * 12) * 4}, 2,
                       Ember);
            DrawTriangle({x - 5, p.y - 10}, {x + 5, p.y - 10}, {x, p.y - 18}, Ember);
        }
        DrawCircleV({p.x - 14, p.y - 2}, 3, Red);
        DrawCircleV({p.x + 14, p.y - 2}, 3, Red);
    }
    for (const auto& s : w.level.sentries) {
        auto p = s.origin;
        float cycle = s.Cycle(w.time), range = w.SentryRange(s);
        bool charging = cycle < Sentry::chargeTime;
        DrawRectangleRec({p.x - 11, p.y + 9, 22, 34}, Steel);
        DrawPoly(p, 6, 21, 30, Steel);
        DrawPolyLinesEx(p, 6, 22, 30, 2, Ember);
        DrawLineEx(p, {p.x + s.direction * 25, p.y}, 12, Muted);
        DrawCircleV(p, 9, charging ? Ember : Red);
        if (charging) {
            for (float x = 30; x < range; x += 24)
                DrawLineEx({p.x + s.direction * x, p.y}, {p.x + s.direction * (x + 8), p.y}, 1,
                           Fade(Ember, 0.6f));
            Glow(p, 40 + cycle * 45, Ember, 0.2f + cycle * 0.2f);
            DrawCircleLines((int)p.x, (int)p.y, 28 - cycle * 12, Ember);
        }
        Vector2 bolt;
        if (w.SentryBolt(s, bolt)) {
            Glow(bolt, 45, Red, 0.35f);
            DrawRectangleRec({bolt.x - 9, bolt.y - 5, 18, 10}, Red);
            DrawLineEx({bolt.x - 7, bolt.y}, {bolt.x + 7, bolt.y}, 3, Light);
        }
    }
    auto g = w.level.goal;
    bool unlocked = w.CellCount() == (int)w.level.cells.size();
    DrawRectangleRec({g.x - 42, g.y - 126, 84, 126}, {29, 49, 51, 255});
    DrawRectangleLinesEx({g.x - 45, g.y - 130, 90, 130}, 4, Muted);
    DrawRectangleRec({g.x - 26, g.y - 112, 52, 112}, unlocked ? Fade(Mint, 0.25f) : Ink);
    for (int i = 0; i < 6; ++i)
        DrawRectangleRec({g.x - 22, g.y - 103 + i * 16.0f, 44, 2},
                         unlocked ? Mint : Fade(Muted, 0.4f));
    Glow({g.x, g.y - 70}, 130, unlocked ? Mint : Ember, 0.2f);
    Text(unlocked ? "CORE ONLINE" : "CELLS REQUIRED", {g.x - 55, g.y - 158}, 14,
         unlocked ? Mint : Ember);
    if (w.deathTimer <= 0)
        DrawRunner(runner, w.player);
    for (const auto& p : particles) {
        float a = 1 - p.age / p.life;
        DrawLineEx(p.position,
                   {p.position.x - p.velocity.x * 0.025f, p.position.y - p.velocity.y * 0.025f},
                   p.size, Fade(p.color, a));
    }
    EndMode2D();
    // Display's quantized target draws its HUD in logical canvas units.
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, RayGame::Display::GetWidth(), RayGame::Display::GetHeight(), 0,
            RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    float width = RayGame::Display::GetWidth(), height = RayGame::Display::GetHeight();
    DrawRectangleGradientV(0, (int)(height - 90), (int)width, 90, Fade(Ink, 0), Fade(Ink, 0.5f));
    if (flash > 0)
        DrawRectangle(0, 0, (int)width, (int)height, Fade(Mint, flash * 0.35f));
}
} // namespace Forge
