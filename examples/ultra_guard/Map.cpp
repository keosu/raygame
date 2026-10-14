#include "Map.h"
#include "Draw.h"
#include "Skins.h"
#include <rlgl.h>

namespace UG {

Vector2 PathLine::Sample(float distance, Vector2* direction) const {
    if (points.size() < 2) return points.empty() ? Vector2{0, 0} : points[0];
    distance = Clamp(distance, 0.0f, length);
    size_t i = 1;
    while (i + 1 < cumulative.size() && cumulative[i] < distance) ++i;
    float segment = cumulative[i] - cumulative[i - 1];
    float t = segment > 0 ? (distance - cumulative[i - 1]) / segment : 0;
    if (direction) *direction = Normalize(Sub(points[i], points[i - 1]));
    return Lerp(points[i - 1], points[i], t);
}

void MapData::Build(const LevelDef& def) {
    level = &def;
    for (auto& row : cells) row.fill(Cell::Open);
    paths.clear();
    decor.clear();
    for (const auto& waypoints : def.paths) {
        PathLine line;
        for (size_t i = 0; i < waypoints.size(); ++i) {
            Vector2 p = TileCenter(waypoints[i].col, waypoints[i].row);
            if (i == 0 && waypoints[i].col < 0) p.x = kMapX - kTile * 0.8f;
            line.points.push_back(p);
            if (i == 0) continue;
            GridPoint a = waypoints[i - 1], b = waypoints[i];
            int dc = (b.col > a.col) - (b.col < a.col), dr = (b.row > a.row) - (b.row < a.row);
            for (GridPoint g = a;; g.col += dc, g.row += dr) {
                if (InGrid(g.col, g.row)) cells[g.row][g.col] = Cell::Path;
                if (g.col == b.col && g.row == b.row) break;
            }
        }
        line.cumulative.push_back(0);
        for (size_t i = 1; i < line.points.size(); ++i)
            line.cumulative.push_back(line.cumulative.back() + Len(Sub(line.points[i], line.points[i - 1])));
        line.length = line.cumulative.back();
        paths.push_back(line);
    }
    basePos = Add(TileCenter(def.base.col, def.base.row), {kTile * 0.5f, 0});
    for (int r = def.base.row - 1; r <= def.base.row + 1; ++r)
        for (int c = def.base.col; c <= def.base.col + 1; ++c)
            if (InGrid(c, r)) cells[r][c] = Cell::Base;

    Rng rng(def.seed);
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (cells[r][c] != Cell::Open) continue;
            bool nearPath = false;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    if (InGrid(c + dc, r + dr) && cells[r + dr][c + dc] != Cell::Open &&
                        cells[r + dr][c + dc] != Cell::Blocked)
                        nearPath = true;
            float roll = rng.Float();
            if (nearPath || roll > 0.24f) continue;
            cells[r][c] = Cell::Blocked;
            Decor d;
            d.kind = rng.Int(0, 2);
            d.col = c;
            d.row = r;
            d.pos = TileCenter(c, r);
            d.size = rng.Range(0.8f, 1.1f);
            d.seed = rng.Next();
            decor.push_back(d);
        }
    }
}

namespace {

void Speckle(Rng& rng, int count, Color a, Color b, float minR, float maxR) {
    for (int i = 0; i < count; ++i) {
        Vector2 p{kMapX + rng.Float() * kMapW, kMapY + rng.Float() * kMapH};
        DrawCircleV(p, rng.Range(minR, maxR), Mix(a, b, rng.Float()));
    }
}

void Tufts(Rng& rng, const MapData& map, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        int c = rng.Int(0, kCols - 1), r = rng.Int(0, kRows - 1);
        if (map.cells[r][c] != Cell::Open) continue;
        Vector2 p = Add(TileCenter(c, r), {rng.Range(-24, 24), rng.Range(-22, 22)});
        for (int k = -1; k <= 1; ++k)
            Draw::Line(p, Add(p, {k * 2.5f, -4.5f - (k == 0) * 1.5f}), 1.1f, color);
    }
}

void PathStroke(const MapData& map, float width, Color color) {
    for (const auto& path : map.paths) {
        for (size_t i = 1; i < path.points.size(); ++i)
            DrawLineEx(path.points[i - 1], path.points[i], width, color);
        for (const auto& p : path.points) DrawCircleV(p, width / 2, color);
    }
}

void PathDashes(const MapData& map, float dash, float gap, float width, Color color) {
    for (const auto& path : map.paths) {
        for (float d = 10; d < path.length - 10; d += dash + gap) {
            Vector2 a = path.Sample(d), b = path.Sample(std::min(path.length, d + dash));
            if (std::fabs(a.x - b.x) > 0.1f && std::fabs(a.y - b.y) > 0.1f) continue; // skip corners
            DrawLineEx(a, b, width, color);
        }
    }
}

void PathPebbles(const MapData& map, Rng& rng, int perTile, Color a, Color b) {
    for (const auto& path : map.paths) {
        for (float d = 0; d < path.length; d += kTile / perTile) {
            Vector2 dir;
            Vector2 p = path.Sample(d, &dir);
            Vector2 n{-dir.y, dir.x};
            p = Add(p, Mul(n, rng.Range(-kTile * 0.32f, kTile * 0.32f)));
            p = Add(p, Mul(dir, rng.Range(-6, 6)));
            DrawCircleV(p, rng.Range(0.8f, 2.2f), Mix(a, b, rng.Float()));
        }
    }
}

void Building(const Decor& d) {
    Rng rng(d.seed);
    static const Color tones[] = {{126, 138, 160, 255}, {176, 160, 140, 255}, {150, 96, 86, 255}, {110, 124, 150, 255},
                                  {196, 190, 176, 255}};
    Color base = tones[rng.Int(0, 4)];
    float x = kMapX + d.col * kTile, y = kMapY + d.row * kTile;
    float h = rng.Range(14, 30);
    float inset = rng.Range(4, 8);
    Rectangle roof{x + inset, y + inset - h + 8, kTile - inset * 2, kTile - inset * 2 - 6};
    Rectangle front{roof.x, roof.y + roof.height, roof.width, h};
    Draw::GlowStretched({x + kTile * 0.62f, y + kTile * 0.78f}, kTile * 0.62f, kTile * 0.36f, 0,
                        WithAlpha({0, 0, 10, 255}, 0.5f));
    DrawRectangleRec({front.x - 1, roof.y - 1, front.width + 2, roof.height + front.height + 2}, {26, 26, 36, 255});
    DrawRectangleRec(front, Darken(base, 0.38f));
    DrawRectangleRec(roof, base);
    DrawRectangleLinesEx(roof, 1.5f, Lighten(base, 0.25f));
    float wx = front.x + 4;
    while (wx + 4 < front.x + front.width - 2) {
        for (float wy = front.y + 4; wy + 4 < front.y + front.height - 2; wy += 7) {
            bool lit = rng.Float() < 0.55f;
            DrawRectangleRec({wx, wy, 4, 4}, lit ? Color{255, 214, 120, 255} : Color{40, 48, 70, 255});
        }
        wx += 7;
    }
    int detail = rng.Int(0, 3);
    if (detail == 0) {
        Vector2 c{roof.x + roof.width / 2, roof.y + roof.height / 2};
        DrawCircleV(c, 9, {70, 74, 84, 255});
        DrawRing(c, 7, 8.5f, 0, 360, 20, {240, 240, 240, 255});
        Draw::Line({c.x - 3, c.y - 4}, {c.x - 3, c.y + 4}, 1.5f, WHITE);
        Draw::Line({c.x + 3, c.y - 4}, {c.x + 3, c.y + 4}, 1.5f, WHITE);
        Draw::Line({c.x - 3, c.y}, {c.x + 3, c.y}, 1.5f, WHITE);
    } else if (detail == 1) {
        DrawRectangleRec({roof.x + 5, roof.y + 5, 10, 8}, Darken(base, 0.2f));
        DrawRectangleRec({roof.x + roof.width - 14, roof.y + 7, 8, 6}, Darken(base, 0.25f));
    } else {
        DrawRectangleRec({roof.x + 4, roof.y + 4, roof.width - 8, 3}, Lighten(base, 0.15f));
    }
}

void Tree(Vector2 p, float s, Color leaf, std::uint32_t seed) {
    Rng rng(seed);
    for (int i = 0; i < 3; ++i) {
        Vector2 c = Add(p, {rng.Range(-12, 12) * s, rng.Range(-10, 10) * s});
        float r = rng.Range(9, 13) * s;
        Draw::GlowStretched(Add(c, {5, 8}), r * 1.4f, r * 0.9f, 0, WithAlpha(BLACK, 0.4f));
        DrawCircleV(c, r + 1.2f, Darken(leaf, 0.55f));
        DrawCircleV(c, r, leaf);
        DrawCircleV(Add(c, {-r * 0.3f, -r * 0.3f}), r * 0.55f, Lighten(leaf, 0.18f));
    }
}

void Pine(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {8, 16}), 18 * s, 9 * s, 0, WithAlpha(BLACK, 0.45f));
    DrawRectangleRec({p.x - 2.5f * s, p.y + 6 * s, 5 * s, 10 * s}, {92, 62, 40, 255});
    for (int i = 0; i < 3; ++i) {
        float w = (16 - i * 4) * s, top = p.y - (8 + i * 9) * s, bottom = p.y + (10 - i * 8) * s;
        Color c = Mix({36, 92, 62, 255}, {70, 140, 84, 255}, i / 3.0f);
        Draw::Tri({p.x, top - 2}, {p.x - w - 1.5f, bottom + 1}, {p.x + w + 1.5f, bottom + 1}, {20, 46, 34, 255});
        Draw::Tri({p.x, top}, {p.x - w, bottom}, {p.x + w, bottom}, c);
        Draw::Tri({p.x, top}, {p.x - w, bottom}, {p.x - w * 0.2f, bottom}, Lighten(c, 0.12f));
    }
}

void Rock(Vector2 p, float s, Color base) {
    Draw::GlowStretched(Add(p, {6, 10}), 22 * s, 10 * s, 0, WithAlpha(BLACK, 0.45f));
    Draw::Shaded(Add(p, {-6 * s, 2 * s}), 12 * s, 9 * s, base, 0.2f, 1.5f);
    Draw::Shaded(Add(p, {7 * s, 4 * s}), 9 * s, 7 * s, Darken(base, 0.08f), -0.3f, 1.5f);
    Draw::Shaded(Add(p, {2 * s, -5 * s}), 8 * s, 7 * s, Lighten(base, 0.08f), 0, 1.5f);
}

void Crystal(Vector2 p, float s, Color tone, std::uint32_t seed) {
    Rng rng(seed);
    Draw::Glow(Add(p, {0, 6}), 30 * s, WithAlpha(tone, 0.22f));
    for (int i = 0; i < 3; ++i) {
        float dx = (i - 1) * 10 * s + rng.Range(-2, 2), h = rng.Range(22, 36) * s * (i == 1 ? 1.2f : 0.85f);
        float w = rng.Range(5, 7) * s;
        Vector2 b{p.x + dx, p.y + 12 * s};
        Vector2 top{b.x + rng.Range(-3, 3), b.y - h};
        Draw::Tri(top, {b.x - w, b.y - h * 0.25f}, {b.x, b.y}, Lighten(tone, 0.3f));
        Draw::Tri(top, {b.x, b.y}, {b.x + w, b.y - h * 0.25f}, Darken(tone, 0.2f));
        Draw::Tri({b.x - w, b.y - h * 0.25f}, {b.x, b.y}, {b.x, b.y + 3 * s}, Darken(tone, 0.35f));
        Draw::Line(top, {b.x, b.y}, 1, WithAlpha(WHITE, 0.6f));
    }
}

void LightTower(Vector2 p, float s) {
    Draw::Glow(p, 30 * s, WithAlpha({120, 180, 255, 255}, 0.25f));
    DrawRectangleRec({p.x - 10 * s, p.y - 4 * s, 20 * s, 18 * s}, {70, 90, 150, 255});
    DrawRectangleRec({p.x - 6 * s, p.y - 24 * s, 12 * s, 22 * s}, {120, 150, 210, 255});
    DrawRectangleRec({p.x - 6 * s, p.y - 24 * s, 4 * s, 22 * s}, {170, 200, 250, 255});
    DrawCircleV({p.x, p.y - 28 * s}, 7 * s, {200, 230, 255, 255});
    DrawCircleV({p.x, p.y - 28 * s}, 4 * s, WHITE);
}

void Ground(const MapData& map) {
    Rng rng(map.level->seed * 7 + 3);
    Rectangle area{kMapX, kMapY, kMapW, kMapH};
    switch (map.level->theme) {
    case Theme::City: {
        DrawRectangleRec(area, {84, 132, 78, 255});
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c)
                if ((r + c) % 2 == 0)
                    DrawRectangleRec({kMapX + c * kTile, kMapY + r * kTile, kTile, kTile}, {90, 140, 82, 255});
        Speckle(rng, 3800, {66, 112, 62, 255}, {120, 170, 96, 255}, 0.6f, 1.8f);
        Tufts(rng, map, 260, {56, 98, 52, 255});
        for (int i = 0; i < 90; ++i) {
            int c = rng.Int(0, kCols - 1), r = rng.Int(0, kRows - 1);
            if (map.cells[r][c] != Cell::Open) continue;
            Vector2 p = Add(TileCenter(c, r), {rng.Range(-22, 22), rng.Range(-22, 22)});
            static const Color flowers[] = {{255, 240, 240, 255}, {255, 210, 90, 255}, {255, 150, 180, 255}};
            DrawCircleV(p, 1.6f, flowers[rng.Int(0, 2)]);
        }
        break;
    }
    case Theme::Mountain: {
        DrawRectangleRec(area, {118, 152, 82, 255});
        Speckle(rng, 4200, {96, 132, 70, 255}, {150, 178, 98, 255}, 0.6f, 2.0f);
        Speckle(rng, 400, {130, 124, 100, 255}, {160, 150, 120, 255}, 0.6f, 1.4f);
        Tufts(rng, map, 320, {80, 116, 58, 255});
        for (int i = 0; i < 120; ++i) {
            int c = rng.Int(0, kCols - 1), r = rng.Int(0, kRows - 1);
            if (map.cells[r][c] != Cell::Open) continue;
            Vector2 p = Add(TileCenter(c, r), {rng.Range(-22, 22), rng.Range(-22, 22)});
            static const Color flowers[] = {{250, 250, 255, 255}, {255, 220, 90, 255}, {200, 150, 255, 255}};
            DrawCircleV(p, 1.5f, flowers[rng.Int(0, 2)]);
        }
        break;
    }
    case Theme::Nebula: {
        Draw::VerticalGradientRect(area, {28, 26, 78, 255}, {14, 22, 56, 255});
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c) {
                Rectangle t{kMapX + c * kTile + 3, kMapY + r * kTile + 3, kTile - 6, kTile - 6};
                Draw::RoundedPanel(t, 6, {36, 44, 100, 160}, {90, 130, 230, 60}, 1.0f);
            }
        Draw::BlendAdd();
        Draw::Glow({kMapX + 160, kMapY + 120}, 360, WithAlpha({170, 80, 255, 255}, 0.22f));
        Draw::Glow({kMapX + kMapW - 200, kMapY + kMapH - 100}, 420, WithAlpha({60, 140, 255, 255}, 0.2f));
        for (int i = 0; i < 260; ++i) {
            Vector2 p{kMapX + rng.Float() * kMapW, kMapY + rng.Float() * kMapH};
            Draw::Glow(p, rng.Range(1.5f, 4.5f), WithAlpha({200, 220, 255, 255}, rng.Range(0.3f, 0.9f)));
        }
        Draw::BlendNormal();
        break;
    }
    }
}

void Paths(const MapData& map) {
    Rng rng(map.level->seed * 13 + 1);
    switch (map.level->theme) {
    case Theme::City:
        PathStroke(map, kTile * 1.02f, {52, 58, 52, 255});
        PathStroke(map, kTile * 0.96f, {178, 176, 166, 255});
        PathStroke(map, kTile * 0.8f, {62, 66, 80, 255});
        PathPebbles(map, rng, 9, {54, 58, 70, 255}, {80, 84, 98, 255});
        PathDashes(map, 12, 12, 2.4f, {250, 210, 90, 255});
        break;
    case Theme::Mountain:
        PathStroke(map, kTile * 0.98f, {104, 80, 52, 255});
        PathStroke(map, kTile * 0.86f, {162, 124, 82, 255});
        PathStroke(map, kTile * 0.6f, {176, 138, 94, 255});
        PathPebbles(map, rng, 14, {120, 96, 70, 255}, {200, 180, 150, 255});
        break;
    case Theme::Nebula:
        Draw::BlendAdd();
        for (const auto& path : map.paths)
            for (float d = 0; d < path.length; d += 14) Draw::Glow(path.Sample(d), kTile * 0.95f, WithAlpha({60, 150, 255, 255}, 0.08f));
        Draw::BlendNormal();
        PathStroke(map, kTile * 0.94f, {120, 200, 255, 255});
        PathStroke(map, kTile * 0.86f, {40, 56, 126, 255});
        PathStroke(map, kTile * 0.62f, {56, 80, 160, 255});
        for (const auto& path : map.paths) {
            for (float d = kTile; d < path.length - kTile * 0.5f; d += kTile * 0.75f) {
                Vector2 dir;
                Vector2 p = path.Sample(d, &dir);
                Vector2 n{-dir.y, dir.x};
                Vector2 tip = Add(p, Mul(dir, 5));
                Color c{170, 220, 255, 150};
                Draw::Line(tip, Add(Sub(p, Mul(dir, 4)), Mul(n, 7)), 2.4f, c);
                Draw::Line(tip, Sub(Sub(p, Mul(dir, 4)), Mul(n, 7)), 2.4f, c);
            }
        }
        break;
    }
}

void DecorLayer(const MapData& map) {
    std::vector<Decor> sorted = map.decor;
    std::sort(sorted.begin(), sorted.end(), [](const Decor& a, const Decor& b) { return a.row < b.row; });
    for (const auto& d : sorted) {
        switch (map.level->theme) {
        case Theme::City:
            if (d.kind == 2) Tree(d.pos, d.size, {56, 120, 64, 255}, d.seed);
            else Building(d);
            break;
        case Theme::Mountain:
            if (d.kind == 0) Rock(d.pos, d.size, {140, 136, 128, 255});
            else {
                Pine(Add(d.pos, {-9, 2}), d.size * 0.8f);
                Pine(Add(d.pos, {10, -4}), d.size);
            }
            break;
        case Theme::Nebula:
            if (d.kind == 0) LightTower(d.pos, d.size);
            else Crystal(d.pos, d.size, d.kind == 1 ? Color{110, 190, 255, 255} : Color{190, 120, 255, 255}, d.seed);
            break;
        }
    }
}

void Lighting(const MapData& map) {
    Rectangle area{kMapX, kMapY, kMapW, kMapH};
    switch (map.level->theme) {
    case Theme::City:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 150, 70, 60},
                               {120, 60, 140, 40});
        break;
    case Theme::Mountain:
        DrawRectangleGradientH((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 230, 170, 40},
                               {80, 120, 200, 30});
        break;
    case Theme::Nebula: break;
    }
    // Vignette
    for (int i = 0; i < 18; ++i) {
        float a = 0.16f * (1 - i / 18.0f);
        Color c = WithAlpha({4, 6, 18, 255}, a);
        DrawRectangleRec({area.x, area.y + i * 2.0f, area.width, 2}, c);
        DrawRectangleRec({area.x, area.y + area.height - (i + 1) * 2.0f, area.width, 2}, c);
        DrawRectangleRec({area.x + i * 2.0f, area.y, 2, area.height}, c);
        DrawRectangleRec({area.x + area.width - (i + 1) * 2.0f, area.y, 2, area.height}, c);
    }
}

void Portal(Vector2 p, float time, Color tone) {
    Draw::BlendAdd();
    Draw::Glow(p, 58, WithAlpha(tone, 0.45f));
    for (int i = 0; i < 4; ++i) {
        float a = time * (2.2f + i * 0.4f) + i * 1.6f;
        Draw::Arc(p, 16 + i * 7.0f, 3.2f, a, a + PI * 0.9f, WithAlpha(Lighten(tone, 0.3f), 0.75f - i * 0.12f), 18);
    }
    Draw::BlendNormal();
    DrawCircleV(p, 13, {14, 4, 22, 255});
    Draw::BlendAdd();
    Draw::Glow(p, 16, WithAlpha({255, 120, 200, 255}, 0.5f + 0.2f * std::sin(time * 6)));
    Draw::BlendNormal();
}

void Base(const MapData& map, float time, float flash) {
    Vector2 p = map.basePos;
    bool nebula = map.level->theme == Theme::Nebula;
    Draw::GlowStretched(Add(p, {6, 52}), 72, 26, 0, WithAlpha(BLACK, 0.5f));
    Color hurt = WithAlpha({255, 60, 60, 255}, flash * 0.8f);
    if (nebula) {
        // Plasma Spark Tower
        Draw::RoundedPanel({p.x - 46, p.y + 20, 92, 40}, 10, {60, 80, 150, 255}, {150, 200, 255, 255}, 2);
        Draw::Quad({p.x - 22, p.y + 24}, {p.x + 22, p.y + 24}, {p.x + 10, p.y - 58}, {p.x - 10, p.y - 58},
                   {150, 180, 230, 255});
        Draw::Quad({p.x - 22, p.y + 24}, {p.x - 6, p.y + 24}, {p.x - 3, p.y - 58}, {p.x - 10, p.y - 58},
                   {200, 220, 250, 255});
        for (int i = 0; i < 3; ++i)
            DrawRing({p.x, p.y - 20.0f - i * 14}, 16.0f - i * 3, 19.0f - i * 3, 200, 340, 16, {220, 236, 255, 255});
        Draw::BlendAdd();
        float pulse = 0.8f + 0.2f * std::sin(time * 3);
        Draw::Glow({p.x, p.y - 70}, 64 * pulse, WithAlpha({120, 200, 255, 255}, 0.7f));
        Draw::Glow({p.x, p.y - 70}, 26, WithAlpha(WHITE, 0.9f));
        for (int i = 0; i < 6; ++i) {
            float a = time * 1.5f + i * PI / 3;
            Draw::Glow(Add({p.x, p.y - 70}, Mul(Dir(a), 30)), 6, WithAlpha({170, 230, 255, 255}, 0.8f));
        }
        if (flash > 0) Draw::Glow(p, 90, hurt);
        Draw::BlendNormal();
        return;
    }
    // Science Patrol headquarters
    Draw::RoundedPanel({p.x - 52, p.y - 6, 104, 70}, 8, {70, 80, 100, 255}, {30, 30, 40, 255}, 2);
    Draw::RoundedPanel({p.x - 48, p.y - 2, 96, 26}, 6, {160, 170, 186, 255}, BLANK, 0);
    for (int i = 0; i < 6; ++i)
        DrawRectangleRec({p.x - 42 + i * 15.0f, p.y + 30, 9, 8}, (int)(time * 2 + i) % 5 == 0 ? Color{255, 240, 180, 255}
                                                                                      : Color{255, 200, 100, 255});
    DrawRectangleRec({p.x - 52, p.y + 46, 104, 6}, {236, 120, 40, 255});
    DrawCircleSector({p.x, p.y - 4}, 40, 180, 360, 32, {30, 34, 46, 255});
    DrawCircleSector({p.x, p.y - 4}, 38, 180, 360, 32, {206, 214, 228, 255});
    DrawCircleSector({p.x - 8, p.y - 10}, 24, 180, 360, 24, {236, 242, 250, 255});
    DrawRing({p.x, p.y - 4}, 26, 31, 180, 360, 32, {236, 120, 40, 255});
    Draw::Line({p.x + 14, p.y - 40}, {p.x + 24, p.y - 74}, 3, {80, 84, 100, 255});
    DrawCircleV({p.x, p.y - 22}, 11, {30, 40, 70, 255});
    Draw::Star({p.x, p.y - 22}, 8, 3.5f, 0, {255, 210, 80, 255});
    Draw::BlendAdd();
    float blink = std::fmod(time, 1.2f) < 0.6f ? 1.0f : 0.25f;
    Draw::Glow({p.x + 24, p.y - 76}, 14, WithAlpha({255, 60, 60, 255}, 0.9f * blink));
    Draw::Glow({p.x, p.y - 22}, 22, WithAlpha({255, 200, 100, 255}, 0.4f));
    if (flash > 0) Draw::Glow(p, 90, hurt);
    Draw::BlendNormal();
}

} // namespace

RenderTexture2D RenderMap(const MapData& map, float scale, bool builtinGroundOnly) {
    RenderTexture2D target = LoadRenderTexture((int)(kMapW * scale), (int)(kMapH * scale));
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(target);
    ClearBackground(BLACK);
    rlPushMatrix();
    rlScalef(scale, scale, 1);
    rlTranslatef(-kMapX, -kMapY, 0);
    Texture2D ground = builtinGroundOnly ? Texture2D{} : Skins::MapGround(*map.level);
    if (ground.id) {
        DrawTexturePro(ground, {0, 0, (float)ground.width, (float)ground.height}, {kMapX, kMapY, kMapW, kMapH},
                       {0, 0}, 0, WHITE);
    } else {
        Ground(map);
    }
    if (!builtinGroundOnly) {
        if (!ground.id || Skins::MapPaths()) Paths(map);
        if (!ground.id || Skins::MapDecor()) DecorLayer(map);
        Lighting(map);
    }
    rlPopMatrix();
    EndTextureMode();
    return target;
}

void DrawMapTexture(const RenderTexture2D& texture, Rectangle dest) { Display::PresentOpaque(texture.texture, dest); }

void DrawMapLive(const MapData& map, float time, float baseDamageFlash) {
    Color tone = map.level->theme == Theme::Nebula ? Color{170, 60, 255, 255} : Color{200, 40, 120, 255};
    std::vector<Vector2> drawn;
    for (const auto& path : map.paths) {
        Vector2 p = path.Sample(kTile * 0.9f);
        p.x = std::max(p.x, kMapX + 18);
        bool dup = false;
        for (auto& q : drawn) dup |= Len(Sub(q, p)) < 4;
        if (dup) continue;
        drawn.push_back(p);
        Portal(p, time, tone);
    }
    Base(map, time, baseDamageFlash);
}

} // namespace UG
