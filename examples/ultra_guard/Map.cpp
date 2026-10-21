#include "Map.h"
#include "Art.h"
#include "Assets.h"
#include "Draw.h"
#include "Gui.h"
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

void MapData::PlaceLandmarks() {
    struct Want {
        Landmark kind;
        int w, h;
    };
    std::vector<Want> wants;
    switch (level->theme) {
    case Theme::Park: wants = {{Landmark::FerrisWheel, 3, 3}, {Landmark::Safari, 3, 2}, {Landmark::WhaleShark, 2, 2}}; break;
    case Theme::Plaza: wants = {{Landmark::BetaCapsule, 2, 3}, {Landmark::JetBeetle, 3, 2}}; break;
    case Theme::Studio: wants = {{Landmark::Castle, 3, 3}, {Landmark::Dinosaur, 3, 2}, {Landmark::Soundstage, 3, 2}}; break;
    default: return;
    }
    // Distance (in tiles) from the nearest road or base cell: landmarks take the spots towers want least.
    std::array<std::array<int, kCols>, kRows> far{};
    std::vector<GridPoint> frontier;
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c) {
            bool road = cells[r][c] == Cell::Path || cells[r][c] == Cell::Base;
            far[r][c] = road ? 0 : 99;
            if (road) frontier.push_back({c, r});
        }
    for (size_t i = 0; i < frontier.size(); ++i) {
        GridPoint g = frontier[i];
        for (GridPoint d : {GridPoint{1, 0}, GridPoint{-1, 0}, GridPoint{0, 1}, GridPoint{0, -1}}) {
            int c = g.col + d.col, r = g.row + d.row;
            if (InGrid(c, r) && far[r][c] > far[g.row][g.col] + 1) {
                far[r][c] = far[g.row][g.col] + 1;
                frontier.push_back({c, r});
            }
        }
    }
    for (const Want& want : wants) {
        int bestScore = -1, bestC = 0, bestR = 0;
        for (int r = 0; r + want.h <= kRows; ++r)
            for (int c = 0; c + want.w <= kCols; ++c) {
                int score = 0, nearest = 99;
                for (int dr = 0; dr < want.h; ++dr)
                    for (int dc = 0; dc < want.w; ++dc) {
                        if (cells[r + dr][c + dc] != Cell::Open) nearest = -1;
                        nearest = std::min(nearest, far[r + dr][c + dc]);
                        score += std::min(far[r + dr][c + dc], 4);
                    }
                if (nearest < 1) continue; // never on or inside the road
                if (nearest >= 2) score += 100;
                if (score > bestScore) {
                    bestScore = score;
                    bestC = c;
                    bestR = r;
                }
            }
        if (bestScore < 0) continue;
        landmarks.push_back({want.kind, bestC, bestR, want.w, want.h});
        for (int dr = 0; dr < want.h; ++dr)
            for (int dc = 0; dc < want.w; ++dc) cells[bestR + dr][bestC + dc] = Cell::Blocked;
    }
}

void MapData::Build(const LevelDef& def) {
    level = &def;
    for (auto& row : cells) row.fill(Cell::Open);
    paths.clear();
    decor.clear();
    landmarks.clear();
    for (const auto& waypoints : def.paths) {
        PathLine line;
        for (size_t i = 0; i < waypoints.size(); ++i) {
            Vector2 p = TileCenter(waypoints[i].col, waypoints[i].row);
            if (i == 0) {
                if (waypoints[i].col < 0) p.x = kMapX - kTile * 0.8f;
                else if (waypoints[i].col >= kCols) p.x = kMapX + kMapW + kTile * 0.8f;
                else if (waypoints[i].row < 0) p.y = kMapY - kTile * 0.8f;
                else if (waypoints[i].row >= kRows) p.y = kMapY + kMapH + kTile * 0.8f;
            }
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
    if (def.HasCastle())
        for (GridPoint corner : {def.castleMin, def.castleMax, GridPoint{def.castleMin.col, def.castleMax.row},
                                 GridPoint{def.castleMax.col, def.castleMin.row}})
            if (cells[corner.row][corner.col] == Cell::Open) cells[corner.row][corner.col] = Cell::Blocked; // corner towers
    PlaceLandmarks();

    Rng rng(def.seed);
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (cells[r][c] != Cell::Open) continue;
            if (InCastle(c, r)) continue; // walls and courtyard stay clear for heroes
            bool nearPath = false;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    if (InGrid(c + dc, r + dr) && cells[r + dr][c + dc] != Cell::Open && cells[r + dr][c + dc] != Cell::Blocked)
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
        for (int k = -1; k <= 1; ++k) Draw::Line(p, Add(p, {k * 2.5f, -4.5f - (k == 0) * 1.5f}), 1.1f, color);
    }
}

void PathStroke(const MapData& map, float width, Color color) {
    for (const auto& path : map.paths) {
        for (size_t i = 1; i < path.points.size(); ++i) DrawLineEx(path.points[i - 1], path.points[i], width, color);
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

void PathArrows(const MapData& map, float spacing, Color color) {
    for (const auto& path : map.paths) {
        for (float d = kTile; d < path.length - kTile * 0.5f; d += spacing) {
            Vector2 dir;
            Vector2 p = path.Sample(d, &dir);
            Vector2 n{-dir.y, dir.x};
            Vector2 tip = Add(p, Mul(dir, 5));
            Draw::Line(tip, Add(Sub(p, Mul(dir, 4)), Mul(n, 7)), 2.4f, color);
            Draw::Line(tip, Sub(Sub(p, Mul(dir, 4)), Mul(n, 7)), 2.4f, color);
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
    static const Color tones[] = {
        {126, 138, 160, 255}, {176, 160, 140, 255}, {150, 96, 86, 255}, {110, 124, 150, 255}, {196, 190, 176, 255}};
    Color base = tones[rng.Int(0, 4)];
    float x = kMapX + d.col * kTile, y = kMapY + d.row * kTile;
    float h = rng.Range(14, 30);
    float inset = rng.Range(4, 8);
    Rectangle roof{x + inset, y + inset - h + 8, kTile - inset * 2, kTile - inset * 2 - 6};
    Rectangle front{roof.x, roof.y + roof.height, roof.width, h};
    Draw::GlowStretched({x + kTile * 0.62f, y + kTile * 0.78f}, kTile * 0.62f, kTile * 0.36f, 0, WithAlpha({0, 0, 10, 255}, 0.5f));
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

void Palm(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {4, 16}), 22 * s, 8 * s, 0, WithAlpha(BLACK, 0.4f));
    DrawRectangleRec({p.x - 3 * s, p.y - 4 * s, 6 * s, 22 * s}, {122, 78, 42, 255});
    DrawRectangleRec({p.x - 3 * s, p.y - 4 * s, 2 * s, 22 * s}, {156, 104, 58, 255});
    Vector2 top{p.x, p.y - 8 * s};
    for (int i = 0; i < 6; ++i) {
        float a = i * PI / 3.0f - 0.5f;
        Vector2 tip = Add(top, Mul(Dir(a), 24 * s));
        Vector2 side = Add(top, Mul(Dir(a + 0.45f), 12 * s));
        Draw::Tri(top, tip, side, i % 2 ? Color{28, 140, 68, 255} : Color{78, 186, 84, 255});
    }
    DrawCircleV(top, 3.5f * s, {36, 110, 52, 255});
}

void BalloonCart(Vector2 p, float s, std::uint32_t seed) {
    Rng rng(seed);
    Draw::GlowStretched(Add(p, {3, 12}), 16 * s, 6 * s, 0, WithAlpha(BLACK, 0.4f));
    DrawRectangleRec({p.x - 10 * s, p.y + 2 * s, 20 * s, 9 * s}, {214, 60, 70, 255});
    DrawRectangleRec({p.x - 10 * s, p.y + 2 * s, 20 * s, 3 * s}, {255, 230, 120, 255});
    DrawCircleV({p.x - 6 * s, p.y + 12 * s}, 2.4f * s, {40, 40, 48, 255});
    DrawCircleV({p.x + 6 * s, p.y + 12 * s}, 2.4f * s, {40, 40, 48, 255});
    static const Color hues[] = {{255, 80, 110, 255}, {255, 200, 60, 255}, {90, 200, 255, 255}, {150, 230, 110, 255}, {200, 120, 255, 255}};
    for (int i = 0; i < 5; ++i) {
        Vector2 b{p.x + (i - 2) * 5.5f * s + rng.Range(-1, 1), p.y - (16 + rng.Range(0, 10)) * s};
        Draw::Line({p.x, p.y + 2 * s}, b, 0.8f, {240, 240, 240, 200});
        Draw::Ellipse(b, 4 * s, 5 * s, hues[(i + seed) % 5], 0);
        DrawCircleV(Add(b, {-1.2f * s, -1.8f * s}), 1.2f * s, WithAlpha(WHITE, 0.7f));
    }
}

void TimerLamp(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {2, 14}), 10 * s, 4 * s, 0, WithAlpha(BLACK, 0.4f));
    DrawRectangleRec({p.x - 1.6f * s, p.y - 12 * s, 3.2f * s, 26 * s}, {150, 156, 170, 255});
    DrawRectangleRec({p.x - 6 * s, p.y + 12 * s, 12 * s, 3 * s}, {120, 126, 140, 255});
    Vector2 c{p.x, p.y - 16 * s};
    DrawCircleV(c, 6.5f * s, {200, 206, 218, 255});
    DrawCircleV(c, 4.6f * s, {40, 120, 220, 255});
    DrawCircleV(Add(c, {-1.4f * s, -1.4f * s}), 1.6f * s, {220, 246, 255, 255});
    Draw::BlendAdd();
    Draw::Glow(c, 14 * s, WithAlpha({90, 190, 255, 255}, 0.5f));
    Draw::BlendNormal();
}

void Vent(Vector2 p, float s, std::uint32_t seed) {
    Rng rng(seed);
    Draw::GlowStretched(Add(p, {4, 10}), 20 * s, 8 * s, 0, WithAlpha(BLACK, 0.45f));
    Draw::Shaded(Add(p, {0, 4 * s}), 16 * s, 9 * s, {54, 40, 38, 255}, 0, 1.5f);
    Draw::Ellipse(Add(p, {0, 3 * s}), 9 * s, 4 * s, {255, 120, 30, 255}, 0);
    Draw::Ellipse(Add(p, {0, 3 * s}), 5 * s, 2 * s, {255, 220, 120, 255}, 0);
    Draw::BlendAdd();
    Draw::Glow(Add(p, {0, 2 * s}), 22 * s, WithAlpha({255, 110, 30, 255}, 0.45f));
    Draw::BlendNormal();
    for (int i = 0; i < 3; ++i)
        Draw::Glow(Add(p, {rng.Range(-4, 4) * s, -(8 + i * 8) * s}), (6 + i * 3) * s, WithAlpha({90, 80, 86, 255}, 0.35f - i * 0.08f));
}

void Dish(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {4, 14}), 16 * s, 6 * s, 0, WithAlpha(BLACK, 0.45f));
    DrawRectangleRec({p.x - 2 * s, p.y - 4 * s, 4 * s, 16 * s}, {150, 156, 170, 255});
    DrawRectangleRec({p.x - 8 * s, p.y + 10 * s, 16 * s, 4 * s}, {110, 116, 130, 255});
    Draw::Ellipse(Add(p, {0, -8 * s}), 14 * s, 6 * s, {214, 220, 232, 255}, -0.5f);
    Draw::Ellipse(Add(p, {-1 * s, -9 * s}), 10 * s, 3.6f * s, {170, 178, 194, 255}, -0.5f);
    Draw::Line(Add(p, {0, -8 * s}), Add(p, {6 * s, -18 * s}), 1.4f, {200, 206, 220, 255});
    Draw::BlendAdd();
    Draw::Glow(Add(p, {6 * s, -18 * s}), 7 * s, WithAlpha({255, 80, 80, 255}, 0.8f));
    Draw::BlendNormal();
}

void SolarPanel(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {4, 12}), 18 * s, 6 * s, 0, WithAlpha(BLACK, 0.45f));
    DrawRectangleRec({p.x - 1.5f * s, p.y - 2 * s, 3 * s, 12 * s}, {150, 156, 170, 255});
    Draw::Quad(Add(p, {-16 * s, -10 * s}), Add(p, {12 * s, -14 * s}), Add(p, {16 * s, -2 * s}), Add(p, {-12 * s, 2 * s}), {40, 70, 150, 255});
    for (int i = 1; i < 4; ++i) {
        float u = i / 4.0f;
        Draw::Line(Lerp(Add(p, {-16 * s, -10 * s}), Add(p, {12 * s, -14 * s}), u), Lerp(Add(p, {-12 * s, 2 * s}), Add(p, {16 * s, -2 * s}), u), 1,
                   {120, 160, 230, 255});
    }
}

void Crater(Vector2 p, float r, Color ground) {
    Draw::Ellipse(Add(p, {0, r * 0.08f}), r, r * 0.62f, Lighten(ground, 0.12f), 0);
    Draw::Ellipse(p, r * 0.86f, r * 0.52f, Darken(ground, 0.22f), 0);
    Draw::Ellipse(Add(p, {r * 0.1f, r * 0.08f}), r * 0.7f, r * 0.4f, Darken(ground, 0.1f), 0);
}

void MoonBase(Vector2 p, float time, float flash) {
    Draw::GlowStretched(Add(p, {6, 44}), 74, 24, 0, WithAlpha(BLACK, 0.5f));
    DrawRectangleRec({p.x - 56, p.y + 18, 112, 22}, {96, 104, 122, 255});
    DrawRectangleRec({p.x - 56, p.y + 18, 112, 5}, {150, 158, 176, 255});
    DrawCircleSector({p.x, p.y + 20}, 46, 180, 360, 40, {190, 200, 218, 255});
    DrawCircleSector({p.x - 10, p.y + 12}, 30, 180, 360, 32, {222, 230, 244, 255});
    for (int i = 0; i < 4; ++i) DrawRing({p.x, p.y + 20}, 18.0f + i * 8, 19.5f + i * 8, 180, 360, 32, {150, 160, 182, 255});
    Draw::Line({p.x + 24, p.y - 18}, {p.x + 34, p.y - 52}, 2.5f, {170, 178, 196, 255});
    Draw::BlendAdd();
    float blink = std::fmod(time, 1.4f) < 0.7f ? 1.0f : 0.3f;
    Draw::Glow({p.x + 34, p.y - 54}, 14, WithAlpha({110, 200, 255, 255}, 0.9f * blink));
    for (int i = 0; i < 5; ++i) Draw::Glow({p.x - 44 + i * 22.0f, p.y + 32}, 5, WithAlpha({120, 220, 255, 255}, 0.7f));
    Draw::Glow({p.x, p.y - 4}, 50, WithAlpha({120, 190, 255, 255}, 0.18f));
    if (flash > 0) Draw::Glow(p, 90, WithAlpha({255, 60, 60, 255}, flash * 0.8f));
    Draw::BlendNormal();
}

void Pylon(Vector2 p, float s, Color glow) {
    DrawRectangleRec({p.x - 2 * s, p.y - 18 * s, 4 * s, 30 * s}, {168, 174, 186, 255});
    DrawCircleV({p.x, p.y - 22 * s}, 5 * s, glow);
    Draw::BlendAdd();
    Draw::Glow({p.x, p.y - 22 * s}, 14 * s, WithAlpha(glow, 0.55f));
    Draw::BlendNormal();
}

void FilmCamera(Vector2 p, float s) {
    Draw::GlowStretched(Add(p, {3, 14}), 16 * s, 6 * s, 0, WithAlpha(BLACK, 0.4f));
    Color dark{40, 42, 52, 255};
    for (float dx : {-9.0f, 0.0f, 9.0f}) Draw::Line({p.x, p.y - 4 * s}, {p.x + dx * s, p.y + 14 * s}, 2 * s, {90, 92, 104, 255});
    DrawRectangleRec({p.x - 9 * s, p.y - 14 * s, 18 * s, 11 * s}, dark);
    Draw::Quad({p.x + 9 * s, p.y - 12 * s}, {p.x + 17 * s, p.y - 15 * s}, {p.x + 17 * s, p.y - 2 * s}, {p.x + 9 * s, p.y - 5 * s}, {70, 72, 86, 255});
    for (float dx : {-4.5f, 4.5f}) {
        DrawCircleV({p.x + dx * s, p.y - 20 * s}, 5 * s, dark);
        DrawCircleV({p.x + dx * s, p.y - 20 * s}, 2 * s, {150, 152, 164, 255});
    }
    DrawCircleV({p.x - 6 * s, p.y - 10 * s}, 1.4f * s, {255, 60, 60, 255});
}

void PathPlanks(const MapData& map, float width, float spacing, Color color) {
    for (const auto& path : map.paths)
        for (float d = 0; d < path.length; d += spacing) {
            Vector2 dir;
            Vector2 p = path.Sample(d, &dir);
            Vector2 n = Mul(Vector2{-dir.y, dir.x}, width / 2);
            DrawLineEx(Sub(p, n), Add(p, n), 1.6f, color);
        }
}

void PathStars(const MapData& map, float spacing, Color star, Color rim) {
    for (const auto& path : map.paths)
        for (float d = kTile * 0.8f; d < path.length - kTile * 0.5f; d += spacing) {
            Vector2 p = path.Sample(d);
            Draw::Star(p, 8, 3.4f, 0, rim);
            Draw::Star(p, 6, 2.5f, 0, star);
        }
}

void PathEmblems(const MapData& map, float spacing) {
    for (const auto& path : map.paths)
        for (float d = kTile; d < path.length - kTile * 0.5f; d += spacing) {
            Vector2 p = path.Sample(d);
            Draw::Diamond(p, 7, 9, {230, 232, 240, 255});
            Draw::Diamond(p, 4, 5.5f, {214, 36, 52, 255});
            DrawCircleV(p, 2, {130, 220, 255, 255});
        }
}

// ---------------------------------------------------------------- landmarks

void FerrisWheel(Rectangle a, float time) {
    Vector2 hub{a.x + a.width / 2, a.y + a.height * 0.42f};
    float R = a.width * 0.38f;
    Color steel{214, 220, 232, 255};
    Draw::GlowStretched({hub.x + 6, a.y + a.height - 6}, a.width * 0.42f, 12, 0, WithAlpha(BLACK, 0.4f));
    for (float s : {-1.0f, 1.0f}) Draw::Line(hub, {hub.x + s * R * 0.75f, a.y + a.height - 8}, 5, {150, 158, 176, 255});
    DrawRectangleRec({hub.x - R * 0.9f, a.y + a.height - 11, R * 1.8f, 6}, {110, 116, 132, 255});
    DrawRing(hub, R - 2, R + 2, 0, 360, 56, steel);
    DrawRing(hub, R * 0.55f - 1, R * 0.55f + 1, 0, 360, 40, WithAlpha(steel, 0.7f));
    static const Color cabins[] = {{255, 90, 90, 255},  {255, 190, 60, 255},  {90, 200, 120, 255},
                                   {80, 160, 255, 255}, {190, 110, 255, 255}, {255, 120, 200, 255}};
    float spin = time * 0.3f;
    for (int i = 0; i < 12; ++i) {
        Vector2 rim = Add(hub, Mul(Dir(spin + i * 2 * PI / 12), R));
        Draw::Line(hub, rim, 1.4f, WithAlpha(steel, 0.8f));
        Vector2 cab = Add(rim, {0, 6});
        Draw::Line(rim, cab, 1.2f, steel);
        Color c = cabins[i % 6];
        Draw::RoundedPanel({cab.x - 6, cab.y - 1, 12, 10}, 3, c, Darken(c, 0.45f), 1);
    }
    DrawCircleV(hub, 7, {232, 238, 248, 255});
    DrawCircleV(hub, 3.5f, {255, 90, 90, 255});
    Draw::BlendAdd();
    for (int i = 0; i < 24; ++i) {
        bool on = ((int)(time * 4) + i) % 3 != 0;
        Draw::Glow(Add(hub, Mul(Dir(spin + i * PI / 12 + 0.13f), R)), on ? 6.0f : 3.0f, WithAlpha({255, 236, 160, 255}, on ? 0.7f : 0.3f));
    }
    Draw::BlendNormal();
}

void Giraffe(Vector2 feet, float s, float t) {
    Color body{234, 184, 72, 255}, spot{150, 92, 40, 255}, dark{120, 80, 40, 255};
    float bob = std::sin(t * 1.3f) * 2 * s;
    Vector2 b{feet.x, feet.y - 20 * s};
    for (float dx : {-8.0f, -4.0f, 5.0f, 9.0f}) Draw::Line({b.x + dx * s, b.y + 2 * s}, {feet.x + dx * s, feet.y}, 2.2f * s, dark);
    Draw::Ellipse(b, 12 * s, 7 * s, body, 0);
    Vector2 head{b.x + 13 * s, b.y - 30 * s + bob};
    Draw::Quad({b.x + 6 * s, b.y - 4 * s}, {b.x + 11 * s, b.y - 3 * s}, {head.x + 2 * s, head.y + 3 * s}, {head.x - 3 * s, head.y + 2 * s}, body);
    Draw::Ellipse(Add(head, {3 * s, 0}), 6 * s, 3.4f * s, body, 0.2f);
    Draw::Line({head.x - 1 * s, head.y - 2 * s}, {head.x - 2 * s, head.y - 7 * s}, 1.4f * s, dark);
    Draw::Line({head.x + 2 * s, head.y - 2 * s}, {head.x + 2 * s, head.y - 7 * s}, 1.4f * s, dark);
    Rng rng(91);
    for (int i = 0; i < 7; ++i) DrawCircleV({b.x + rng.Range(-9, 9) * s, b.y + rng.Range(-4, 4) * s}, 1.8f * s, spot);
    for (int i = 0; i < 4; ++i) DrawCircleV(Lerp({b.x + 9 * s, b.y - 6 * s}, head, 0.2f + i * 0.2f), 1.5f * s, spot);
}

void Panda(Vector2 feet, float t) {
    Color white{240, 240, 236, 255}, black{30, 30, 36, 255};
    Vector2 b{feet.x, feet.y - 8};
    Draw::Ellipse(b, 10, 8, white, 0);
    DrawCircleV({b.x - 7, b.y + 6}, 3.5f, black);
    DrawCircleV({b.x + 7, b.y + 6}, 3.5f, black);
    Draw::Ellipse({b.x, b.y - 1}, 10, 3.5f, black, 0);
    Vector2 h{b.x + 2, b.y - 12};
    DrawCircleV({h.x - 5, h.y - 5}, 3, black);
    DrawCircleV({h.x + 5, h.y - 5}, 3, black);
    DrawCircleV(h, 7, white);
    Draw::Ellipse({h.x - 3, h.y}, 2, 2.6f, black, -0.4f);
    Draw::Ellipse({h.x + 3, h.y}, 2, 2.6f, black, 0.4f);
    float chew = std::sin(t * 3) * 1.5f;
    Draw::Line({b.x + 8, b.y - 4 + chew}, {b.x + 16, b.y - 18 + chew}, 2, {90, 170, 70, 255});
}

void Safari(Rectangle a, float time) {
    Rectangle pen{a.x + 6, a.y + 8, a.width - 12, a.height - 14};
    Draw::GlowStretched({pen.x + pen.width / 2 + 4, pen.y + pen.height}, pen.width * 0.55f, 10, 0, WithAlpha(BLACK, 0.35f));
    Draw::RoundedPanel(pen, 14, {218, 190, 120, 255}, BLANK, 0);
    Draw::Ellipse({pen.x + pen.width * 0.7f, pen.y + pen.height * 0.68f}, 26, 10, {150, 170, 84, 220}, 0);
    Vector2 tree{pen.x + pen.width * 0.84f, pen.y + 18};
    Draw::Line({tree.x, tree.y + 30}, tree, 4, {112, 78, 42, 255});
    Draw::Ellipse(tree, 24, 8, {78, 138, 60, 255}, 0);
    Draw::Ellipse(Add(tree, {-4, -4}), 16, 5, {112, 172, 80, 255}, 0);
    Giraffe({pen.x + 26, pen.y + pen.height - 8}, 1.0f, time);
    Giraffe({pen.x + 62, pen.y + pen.height - 4}, 0.82f, time + 1.3f);
    Panda({pen.x + pen.width - 32, pen.y + pen.height - 10}, time);
    Color wood{132, 92, 52, 255};
    for (float y : {pen.y + 2, pen.y + pen.height - 2}) {
        Draw::Line({pen.x + 4, y}, {pen.x + pen.width - 4, y}, 2, wood);
        for (float x = pen.x + 6; x < pen.x + pen.width - 4; x += 13) DrawRectangleRec({x - 1.5f, y - 6, 3, 9}, Darken(wood, 0.2f));
    }
}

void WhaleShark(Rectangle a, float time) {
    Vector2 c{a.x + a.width / 2, a.y + a.height / 2 + 4};
    float rx = a.width * 0.46f, ry = a.height * 0.4f;
    Draw::GlowStretched(Add(c, {4, ry}), rx, 10, 0, WithAlpha(BLACK, 0.35f));
    Draw::Ellipse(c, rx + 4, ry + 4, {200, 230, 240, 255}, 0);
    Draw::GradientEllipse(c, rx, ry, {10, 50, 120, 255}, {40, 150, 210, 255}, {c.x - rx * 0.3f, c.y - ry * 0.3f}, 10);
    float ang = time * 0.6f;
    Vector2 pos = Add(c, {std::cos(ang) * rx * 0.5f, std::sin(ang) * ry * 0.45f});
    float heading = ang + PI / 2;
    Vector2 d = Dir(heading);
    Draw::Ellipse(pos, 18, 6.5f, {60, 78, 110, 255}, heading);
    Vector2 tail = Sub(pos, Mul(d, 18));
    Draw::Tri(tail, Add(Sub(tail, Mul(d, 8)), Mul(Vector2{-d.y, d.x}, 7)), Sub(Sub(tail, Mul(d, 8)), Mul(Vector2{-d.y, d.x}, 7)), {60, 78, 110, 255});
    Rng rng(5);
    for (int i = 0; i < 8; ++i) DrawCircleV(Add(pos, Rotate({rng.Range(-12, 12), rng.Range(-3, 3)}, heading)), 1.2f, {230, 240, 255, 255});
    Draw::BlendAdd();
    for (int i = 0; i < 4; ++i)
        Draw::Ellipse(Add(c, {std::sin(time * 0.9f + i * 1.7f) * rx * 0.6f, std::cos(time * 0.7f + i) * ry * 0.5f}), 10, 3, WithAlpha({140, 220, 255, 255}, 0.25f),
                      0.3f);
    Draw::BlendNormal();
}

// Giant replica of Hayata's Beta Capsule, flash head lit, on a museum pedestal.
void BetaCapsule(Rectangle a, float time) {
    float cx = a.x + a.width / 2;
    Rectangle step{a.x + 8, a.y + a.height - 16, a.width - 16, 10};
    Rectangle ped{a.x + 20, a.y + a.height - 32, a.width - 40, 18};
    Draw::GlowStretched({cx + 4, step.y + 10}, a.width * 0.5f, 10, 0, WithAlpha(BLACK, 0.45f));
    Draw::RoundedPanel(step, 3, {150, 150, 160, 255}, {100, 100, 112, 255}, 1);
    Draw::RoundedPanel(ped, 3, {188, 188, 198, 255}, {120, 120, 132, 255}, 1);
    DrawRectangleRec({cx - 12, ped.y + 6, 24, 6}, {214, 172, 70, 255});
    float top = a.y + 26, bottom = ped.y;
    Rectangle body{cx - 11, top, 22, bottom - top};
    DrawRectangleRounded({body.x - 1.5f, body.y - 1.5f, body.width + 3, body.height + 3}, 0.5f, 8, {40, 42, 52, 255});
    DrawRectangleGradientH((int)body.x, (int)body.y, (int)body.width, (int)body.height, {150, 156, 170, 255}, {236, 240, 248, 255});
    DrawRectangle((int)(body.x + body.width * 0.62f), (int)body.y, (int)(body.width * 0.38f), (int)body.height, {170, 176, 190, 255});
    for (float y : {bottom - 30, bottom - 22}) DrawRectangleRec({body.x, y, body.width, 4}, {36, 38, 46, 255});
    DrawRectangleRec({body.x + body.width - 4, top + 24, 6, 10}, {214, 40, 50, 255});
    Vector2 head{cx, top};
    Draw::Ellipse(head, 13, 6, {206, 212, 224, 255}, 0);
    float pulse = 0.6f + 0.4f * std::sin(time * 3);
    Draw::BlendAdd();
    Draw::Glow(head, 34 * pulse + 14, WithAlpha({255, 244, 210, 255}, 0.55f));
    for (int i = 0; i < 8; ++i) {
        float ang = time * 0.6f + i * PI / 4;
        Draw::Tri(head, Add(head, Mul(Dir(ang - 0.06f), 36 + 8 * pulse)), Add(head, Mul(Dir(ang + 0.06f), 36 + 8 * pulse)),
                  WithAlpha({255, 236, 180, 255}, 0.22f));
    }
    Draw::BlendNormal();
    DrawCircleV(head, 5, {255, 252, 236, 255});
}

// The Science Patrol's Jet Beetle on a display pylon, same plane as the air-support skill.
void JetBeetle(Rectangle a, float time) {
    Vector2 c{a.x + a.width / 2, a.y + a.height * 0.42f + std::sin(time * 1.2f) * 1.5f};
    Draw::GlowStretched({c.x + 6, a.y + a.height - 8}, a.width * 0.42f, 9, 0, WithAlpha(BLACK, 0.4f));
    DrawRectangleRec({c.x - 3, c.y + 6, 6, a.y + a.height - c.y - 14}, {120, 126, 140, 255});
    Draw::RoundedPanel({c.x - 30, a.y + a.height - 14, 60, 8}, 3, {150, 154, 166, 255}, {100, 104, 116, 255}, 1);
    Color silver{214, 220, 232, 255}, shade{160, 166, 180, 255}, red{214, 46, 46, 255};
    Draw::Quad({c.x - 6, c.y}, {c.x + 6, c.y}, {c.x + 26, c.y + 14}, {c.x - 26, c.y + 14}, shade);
    Draw::Quad({c.x - 4, c.y - 2}, {c.x + 4, c.y - 2}, {c.x + 38, c.y + 8}, {c.x - 38, c.y + 8}, silver);
    DrawRectangleRounded({c.x - 52, c.y - 9, 104, 16}, 1, 12, silver);
    DrawRectangleRec({c.x - 50, c.y - 3, 100, 3}, red);
    Draw::Tri({c.x + 52, c.y - 1}, {c.x + 44, c.y - 9}, {c.x + 44, c.y + 7}, shade);
    Draw::Ellipse({c.x + 26, c.y - 6}, 11, 5, {70, 140, 200, 255}, 0);
    for (float side : {-1.0f, 1.0f}) Draw::Tri({c.x - 40, c.y - 6}, {c.x - 52, c.y - 22 * side + (side < 0 ? 0 : 2)}, {c.x - 30, c.y - 6}, shade);
    DrawCircleV({c.x - 6, c.y - 1}, 4, {255, 140, 40, 255});
    Draw::BlendAdd();
    Draw::Glow({c.x - 56, c.y - 1}, 10 + 3 * std::sin(time * 9), WithAlpha({255, 160, 60, 255}, 0.6f));
    Draw::BlendNormal();
}

void Castle(Rectangle a, float time) {
    float base = a.y + a.height - 10;
    Draw::Ellipse({a.x + a.width / 2, base}, a.width * 0.5f, 14, {96, 120, 88, 255}, 0);
    Color stone{176, 172, 190, 255}, shade{140, 136, 156, 255}, roof{58, 70, 150, 255};
    DrawRectangleRec({a.x + 18, base - 48, a.width - 36, 46}, stone);
    for (float x = a.x + 18; x < a.x + a.width - 22; x += 10) DrawRectangleRec({x, base - 54, 6, 7}, stone);
    Draw::RoundedPanel({a.x + a.width / 2 - 10, base - 26, 20, 24}, 9, {50, 40, 60, 255}, BLANK, 0);
    struct Tower {
        float x, w, h;
    };
    Tower towers[] = {{a.x + 14, 18, 76}, {a.x + a.width / 2 - 13, 26, 118}, {a.x + a.width - 32, 18, 84}};
    for (const auto& t : towers) {
        DrawRectangleRec({t.x, base - t.h, t.w, t.h}, stone);
        DrawRectangleRec({t.x + t.w * 0.65f, base - t.h, t.w * 0.35f, t.h}, shade);
        Draw::Tri({t.x + t.w / 2, base - t.h - t.w * 1.3f}, {t.x - 4, base - t.h + 2}, {t.x + t.w + 4, base - t.h + 2}, roof);
        Vector2 pole{t.x + t.w / 2, base - t.h - t.w * 1.3f};
        Draw::Line(pole, Add(pole, {0, -10}), 1.2f, {60, 60, 70, 255});
        float wave = std::sin(time * 4 + t.x) * 2;
        Draw::Tri(Add(pole, {0, -10}), Add(pole, {9, -7 + wave}), Add(pole, {0, -4}), {230, 50, 60, 255});
        for (int i = 0; i < 2; ++i) {
            Rectangle win{t.x + t.w / 2 - 2.5f, base - t.h + 14 + i * 22.0f, 5, 8};
            DrawRectangleRec(win, {255, 214, 120, 255});
        }
    }
    Draw::BlendAdd();
    Draw::Glow({a.x + a.width / 2, base - 70}, 40, WithAlpha({255, 220, 150, 255}, 0.18f));
    Draw::BlendNormal();
}

void Dinosaur(Rectangle a, float time) {
    float base = a.y + a.height - 8;
    Draw::Ellipse({a.x + a.width / 2, base - 6}, a.width * 0.48f, 16, {52, 96, 52, 255}, 0);
    for (int i = 0; i < 6; ++i) {
        Vector2 f{a.x + 10 + i * (a.width - 20) / 5, base - 4};
        for (int k = -2; k <= 2; ++k) Draw::Tri(f, Add(f, Mul(Dir(-PI / 2 + k * 0.4f), 16)), Add(f, Mul(Dir(-PI / 2 + k * 0.4f + 0.18f), 14)),
                                                 k % 2 ? Color{60, 130, 60, 255} : Color{90, 160, 70, 255});
    }
    Color skin{96, 116, 74, 255}, belly{150, 160, 110, 255};
    Vector2 hip{a.x + a.width * 0.45f, base - 30};
    Draw::Tri({hip.x - 8, hip.y - 6}, {hip.x - 8, hip.y + 8}, {a.x + 6, hip.y + 10}, skin);
    Draw::Line({hip.x - 4, hip.y + 6}, {hip.x - 8, base - 6}, 6, Darken(skin, 0.2f));
    Draw::Line({hip.x + 6, hip.y + 6}, {hip.x + 8, base - 4}, 6, skin);
    Draw::Ellipse(hip, 20, 13, skin, -0.35f);
    Draw::Ellipse({hip.x + 4, hip.y + 4}, 12, 6, belly, -0.35f);
    Vector2 neck{hip.x + 18, hip.y - 14};
    float jaw = std::max(0.0f, std::sin(time * 1.4f)) * 0.5f;
    DrawRectangleRec({neck.x - 2, neck.y - 8, 24, 12}, skin);
    Draw::Quad({neck.x, neck.y + 4}, {neck.x + 22, neck.y + 4}, {neck.x + 20 - jaw * 6, neck.y + 9 + jaw * 10}, {neck.x + 2, neck.y + 8}, Darken(skin, 0.15f));
    DrawCircleV({neck.x + 15, neck.y - 4}, 1.8f, {255, 210, 60, 255});
    Draw::Line({hip.x + 14, hip.y - 2}, {hip.x + 20, hip.y + 4}, 2, skin);
}

void Soundstage(Rectangle a, float time, bool live) {
    float base = a.y + a.height - 6;
    Rectangle hall{a.x + 8, base - 54, a.width - 16, 50};
    Draw::GlowStretched({hall.x + hall.width / 2 + 4, base}, hall.width * 0.55f, 9, 0, WithAlpha(BLACK, 0.4f));
    Draw::Ellipse({hall.x + hall.width / 2, hall.y + 2}, hall.width / 2, 14, {200, 192, 170, 255}, 0);
    DrawRectangleRec(hall, {218, 210, 188, 255});
    DrawRectangleRec({hall.x, hall.y + hall.height - 6, hall.width, 6}, {170, 162, 140, 255});
    DrawRectangleRec({hall.x + 12, hall.y + 18, 30, 28}, {120, 112, 96, 255});
    DrawText("27", (int)(hall.x + hall.width - 52), (int)(hall.y + 10), 30, {60, 60, 70, 255});
    Rectangle board{hall.x + hall.width - 50, hall.y + 42, 22, 12};
    DrawRectangleRec(board, {30, 30, 36, 255});
    for (int i = 0; i < 4; ++i) DrawRectangleRec({board.x + i * 6.0f, board.y - 5, 3, 5}, i % 2 ? WHITE : Color{30, 30, 36, 255});
    if (!live) return;
    Draw::BlendAdd();
    for (int i = 0; i < 2; ++i) {
        Vector2 lamp{hall.x + 10 + i * (hall.width - 20), hall.y};
        float ang = -PI / 2 + std::sin(time * 0.7f + i * 2.1f) * 0.55f;
        Vector2 d = Dir(ang), n{-d.y, d.x};
        Vector2 far = Add(lamp, Mul(d, 300));
        Draw::Tri(lamp, Add(far, Mul(n, 34)), Sub(far, Mul(n, 34)), WithAlpha({255, 244, 200, 255}, 0.1f));
        Draw::Glow(lamp, 10, WithAlpha({255, 244, 200, 255}, 0.7f));
    }
    Draw::BlendNormal();
}

void DrawLandmark(const LandmarkSpot& spot, float time, bool live) {
    Rectangle a = spot.Area();
    switch (spot.kind) {
    case Landmark::FerrisWheel: FerrisWheel(a, time); break;
    case Landmark::Safari: Safari(a, time); break;
    case Landmark::WhaleShark: WhaleShark(a, time); break;
    case Landmark::BetaCapsule: BetaCapsule(a, time); break;
    case Landmark::JetBeetle: JetBeetle(a, time); break;
    case Landmark::Castle: Castle(a, time); break;
    case Landmark::Dinosaur: Dinosaur(a, time); break;
    case Landmark::Soundstage: Soundstage(a, time, live); break;
    }
}

void SpotLight(Vector2 p, float s) {
    DrawRectangleRec({p.x - 1.6f * s, p.y - 6 * s, 3.2f * s, 20 * s}, {36, 40, 52, 255});
    DrawCircleV({p.x, p.y - 12 * s}, 6 * s, {240, 236, 220, 255});
    Draw::BlendAdd();
    Draw::Glow({p.x, p.y - 12 * s}, 16 * s, WithAlpha({255, 236, 170, 255}, 0.55f));
    Draw::BlendNormal();
}

void SetWall(Vector2 p, float s, std::uint32_t seed) {
    Rng rng(seed);
    static const Color tones[] = {{198, 112, 146, 255}, {112, 146, 196, 255}, {186, 156, 96, 255}};
    Color c = tones[rng.Int(0, 2)];
    Draw::GlowStretched(Add(p, {4, 14}), 28 * s, 8 * s, 0, WithAlpha(BLACK, 0.35f));
    DrawRectangleRec({p.x - 16 * s, p.y - 16 * s, 32 * s, 30 * s}, Darken(c, 0.2f));
    DrawRectangleRec({p.x - 16 * s, p.y - 16 * s, 32 * s, 8 * s}, c);
    DrawRectangleRec({p.x - 6 * s, p.y - 2 * s, 12 * s, 10 * s}, {232, 240, 255, 255});
    Draw::Star({p.x, p.y - 12 * s}, 4 * s, 1.8f * s, 0, {255, 210, 80, 255});
}

void ParkGate(Vector2 p, float time, float flash) {
    Draw::GlowStretched(Add(p, {4, 36}), 86, 18, 0, WithAlpha(BLACK, 0.45f));
    Color teal{18, 138, 146, 255}, orange{255, 138, 36, 255};
    DrawRectangleRec({p.x - 48, p.y - 8, 16, 54}, teal);
    DrawRectangleRec({p.x + 32, p.y - 8, 16, 54}, teal);
    DrawRectangleRec({p.x - 52, p.y - 20, 104, 16}, orange);
    DrawRectangleRec({p.x - 36, p.y - 34, 72, 16}, teal);
    Draw::Star({p.x, p.y - 26}, 7, 3, time * 0.4f, {255, 228, 120, 255});
    Draw::BlendAdd();
    Draw::Glow({p.x, p.y - 26}, 22, WithAlpha(orange, 0.5f));
    if (flash > 0) Draw::Glow(p, 90, WithAlpha({255, 60, 60, 255}, flash * 0.8f));
    Draw::BlendNormal();
}

void TimerMonument(Vector2 p, float time, float flash) {
    Draw::GlowStretched(Add(p, {2, 40}), 70, 16, 0, WithAlpha(BLACK, 0.45f));
    Draw::RoundedPanel({p.x - 30, p.y + 18, 60, 26}, 6, {150, 156, 170, 255}, {80, 86, 102, 255}, 2);
    DrawCircleV({p.x, p.y - 6}, 28, {198, 206, 216, 255});
    DrawCircleV({p.x, p.y - 6}, 20, {18, 20, 30, 255});
    float blink = 0.65f + 0.35f * std::sin(time * 4);
    DrawCircleV({p.x, p.y - 6}, 11, {220, 32, 44, 255});
    Draw::BlendAdd();
    Draw::Glow({p.x, p.y - 6}, 40 * blink, WithAlpha({255, 40, 56, 255}, 0.75f));
    if (flash > 0) Draw::Glow(p, 90, WithAlpha({255, 60, 60, 255}, flash * 0.8f));
    Draw::BlendNormal();
}

// The Universal globe: a spinning blue Earth in a fountain, ringed by the studio's name.
void StudioGlobe(Vector2 p, float time, float flash) {
    Draw::GlowStretched(Add(p, {2, 44}), 70, 16, 0, WithAlpha(BLACK, 0.45f));
    Vector2 pool{p.x, p.y + 30};
    Draw::Ellipse(pool, 50, 14, {150, 156, 170, 255}, 0);
    Draw::Ellipse(pool, 44, 11, {50, 130, 200, 255}, 0);
    Draw::BlendAdd();
    for (int i = 0; i < 6; ++i) {
        float a = i * PI / 3 + time * 0.4f;
        Vector2 jet{pool.x + std::cos(a) * 34, pool.y + std::sin(a) * 8};
        Draw::Line(jet, Add(jet, {0, -10 - 3 * std::sin(time * 6 + i)}), 2, WithAlpha({200, 236, 255, 255}, 0.5f));
    }
    Draw::BlendNormal();
    DrawRectangleRec({p.x - 5, p.y - 2, 10, 32}, {170, 176, 190, 255});
    Vector2 g{p.x, p.y - 28};
    const float R = 30, ringX = 48, ringY = 13, tilt = -0.18f;
    auto ringPoint = [&](float a) { return Add(g, Rotate({std::cos(a) * ringX, std::sin(a) * ringY}, tilt)); };
    std::vector<Vector2> back, front;
    for (int i = 0; i <= 24; ++i) {
        back.push_back(ringPoint(PI + PI * i / 24));
        front.push_back(ringPoint(PI * i / 24));
    }
    Draw::PolyLine(back, 6, {150, 158, 176, 255}, false);
    DrawCircleV(g, R + 1.5f, {20, 40, 80, 255});
    Draw::GradientEllipse(g, R, R, {22, 70, 150, 255}, {90, 170, 240, 255}, {g.x - 10, g.y - 12}, 12);
    // Continents slide across the face; squash toward the limb to read as a sphere.
    static const float lon[] = {0.0f, 1.3f, 2.4f, 3.6f, 4.7f, 5.6f};
    static const float lat[] = {-10, 6, -4, 12, -14, 2};
    static const float size[] = {9, 7, 11, 6, 8, 10};
    for (int i = 0; i < 6; ++i) {
        float a = std::fmod(lon[i] + time * 0.35f, 2 * PI);
        float x = std::sin(a);
        if (std::cos(a) < 0.15f) continue; // far side
        float squash = std::cos(a);
        Draw::Ellipse({g.x + x * (R - size[i] * 0.6f), g.y + lat[i]}, size[i] * squash, size[i] * 0.6f, {70, 160, 90, 230}, 0);
    }
    Draw::Ellipse({g.x - 10, g.y - 13}, 9, 5, WithAlpha(WHITE, 0.35f), -0.5f);
    Draw::PolyLine(front, 6, {220, 226, 238, 255}, false);
    Draw::PolyLine(front, 2, {250, 252, 255, 255}, false);
    static const char* kLetters = "UNIVERSAL";
    for (int i = 0; i < 9; ++i) {
        float a = -time * 0.5f - i * 0.3f; // letters run left to right across the front
        if (std::sin(a) < 0.25f) continue;   // only the front arc
        Vector2 at = ringPoint(a);
        char letter[2] = {kLetters[i], 0};
        Gui::Text(letter, at.x, at.y - 7, 11, {24, 34, 70, 255}, Gui::Align::Center, true);
    }
    Draw::BlendAdd();
    Draw::Glow(g, 46, WithAlpha({120, 190, 255, 255}, 0.3f));
    if (flash > 0) Draw::Glow(p, 90, WithAlpha({255, 60, 60, 255}, flash * 0.8f));
    Draw::BlendNormal();
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
                if ((r + c) % 2 == 0) DrawRectangleRec({kMapX + c * kTile, kMapY + r * kTile, kTile, kTile}, {90, 140, 82, 255});
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
    case Theme::Park: {
        DrawRectangleRec(area, {58, 168, 86, 255});
        Speckle(rng, 3600, {48, 140, 72, 255}, {120, 200, 90, 255}, 0.6f, 1.8f);
        Tufts(rng, map, 220, {36, 120, 58, 255});
        for (int i = 0; i < 6; ++i) {
            Vector2 p{kMapX + rng.Range(40, kMapW - 40), kMapY + rng.Range(30, kMapH - 30)};
            float rx = rng.Range(26, 52), ry = rng.Range(14, 26);
            Draw::Ellipse(p, rx, ry, {36, 150, 186, 255}, rng.Range(-0.4f, 0.4f));
            Draw::Ellipse(Add(p, {-rx * 0.2f, -ry * 0.25f}), rx * 0.45f, ry * 0.35f, {150, 220, 235, 180}, 0.2f);
        }
        for (int i = 0; i < 140; ++i) {
            int c = rng.Int(0, kCols - 1), r = rng.Int(0, kRows - 1);
            if (map.cells[r][c] != Cell::Open) continue;
            Vector2 p = Add(TileCenter(c, r), {rng.Range(-20, 20), rng.Range(-20, 20)});
            static const Color flowers[] = {{255, 96, 140, 255}, {255, 196, 48, 255}, {255, 150, 40, 255}, {80, 210, 255, 255}};
            DrawCircleV(p, 1.8f, flowers[rng.Int(0, 3)]);
        }
        break;
    }
    case Theme::Plaza: {
        DrawRectangleRec(area, {176, 180, 190, 255});
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c) {
                Rectangle t{kMapX + c * kTile + 2, kMapY + r * kTile + 2, kTile - 4, kTile - 4};
                Color tile = (r + c) % 2 == 0 ? Color{196, 200, 208, 255} : Color{168, 172, 182, 255};
                DrawRectangleRec(t, tile);
            }
        for (int c = 0; c <= kCols; c += 4) DrawRectangleRec({kMapX + c * kTile - 1, kMapY, 2, kMapH}, {196, 48, 60, 90});
        Draw::BlendAdd();
        Draw::Glow({kMapX + kMapW * 0.5f, kMapY + kMapH * 0.45f}, 280, WithAlpha({255, 60, 70, 255}, 0.08f));
        Draw::BlendNormal();
        break;
    }
    case Theme::Studio: {
        DrawRectangleRec(area, {132, 140, 156, 255});
        Speckle(rng, 2400, {110, 118, 132, 255}, {168, 174, 186, 255}, 0.5f, 1.6f);
        for (int i = 0; i < 4; ++i) {
            int c = rng.Int(1, kCols - 4), r = rng.Int(1, kRows - 3);
            static const Color sets[] = {{186, 96, 132, 255}, {96, 128, 186, 255}, {176, 150, 90, 255}, {70, 150, 160, 255}};
            DrawRectangleRec({kMapX + c * kTile, kMapY + r * kTile, kTile * 2.2f, kTile * 1.6f}, WithAlpha(sets[i], 0.55f));
        }
        break;
    }
    case Theme::Volcano: {
        DrawRectangleRec(area, {62, 46, 44, 255});
        Speckle(rng, 4200, {44, 32, 32, 255}, {96, 72, 64, 255}, 0.6f, 2.0f);
        // Glowing fissures between cooled lava plates.
        Draw::BlendAdd();
        for (int i = 0; i < 16; ++i) {
            Vector2 a{kMapX + rng.Float() * kMapW, kMapY + rng.Float() * kMapH};
            float heading = rng.Range(0, 2 * PI);
            for (int k = 0; k < 5; ++k) {
                heading += rng.Range(-0.7f, 0.7f);
                Vector2 b = Add(a, Mul(Dir(heading), rng.Range(14, 30)));
                Draw::Line(a, b, 3.2f, WithAlpha({255, 90, 20, 255}, 0.35f));
                Draw::Line(a, b, 1.2f, WithAlpha({255, 200, 90, 255}, 0.6f));
                a = b;
            }
        }
        for (int i = 0; i < 5; ++i) {
            int c = rng.Int(0, kCols - 1), r = rng.Int(0, kRows - 1);
            if (map.cells[r][c] != Cell::Open) continue;
            Draw::Glow(TileCenter(c, r), 70, WithAlpha({255, 90, 30, 255}, 0.18f));
        }
        Draw::BlendNormal();
        break;
    }
    case Theme::Moon: {
        Color dust{126, 128, 140, 255};
        DrawRectangleRec(area, dust);
        Speckle(rng, 4600, {100, 102, 114, 255}, {160, 162, 174, 255}, 0.5f, 1.8f);
        for (int i = 0; i < 26; ++i) {
            Vector2 p{kMapX + rng.Float() * kMapW, kMapY + rng.Float() * kMapH};
            Crater(p, rng.Range(10, i < 4 ? 46.0f : 22.0f), dust);
        }
        Draw::BlendAdd();
        Draw::Glow({kMapX + kMapW - 60, kMapY + 50}, 140, WithAlpha({80, 150, 255, 255}, 0.18f));
        Draw::BlendNormal();
        break;
    }
    case Theme::Citadel: {
        // Land of Light: crystal paving radiating from the Plasma Spark Tower.
        Draw::VerticalGradientRect(area, {40, 62, 118, 255}, {22, 34, 76, 255});
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c) {
                Rectangle t{kMapX + c * kTile + 2, kMapY + r * kTile + 2, kTile - 4, kTile - 4};
                Color fill = (r + c) % 2 == 0 ? Color{56, 84, 150, 200} : Color{46, 70, 132, 200};
                Draw::RoundedPanel(t, 4, fill, {150, 200, 255, 50}, 1.0f);
            }
        Vector2 core = map.basePos;
        if (map.level->HasCastle()) {
            const LevelDef& def = *map.level;
            Rectangle outer{kMapX + def.castleMin.col * kTile, kMapY + def.castleMin.row * kTile,
                            (def.castleMax.col - def.castleMin.col + 1) * kTile, (def.castleMax.row - def.castleMin.row + 1) * kTile};
            // Moat of light hugging the walls; roads cross it as bridges.
            Rectangle moat{outer.x - 10, outer.y - 10, outer.width + 20, outer.height + 20};
            Draw::RoundedPanel(moat, 14, {18, 40, 96, 255}, {150, 210, 255, 160}, 2);
            Draw::RoundedPanel({moat.x + 6, moat.y + 6, moat.width - 12, moat.height - 12}, 10, {26, 58, 128, 255}, BLANK, 0);
            Rectangle yard{outer.x + kTile * 0.5f, outer.y + kTile * 0.5f, outer.width - kTile, outer.height - kTile};
            Draw::VerticalGradientRect(outer, {150, 168, 210, 255}, {118, 136, 186, 255});
            Draw::VerticalGradientRect(yard, {158, 174, 214, 255}, {132, 150, 198, 255});
            for (int r = def.castleMin.row + 1; r < def.castleMax.row; ++r)
                for (int c = def.castleMin.col + 1; c < def.castleMax.col; ++c) {
                    Rectangle t{kMapX + c * kTile + 3, kMapY + r * kTile + 3, kTile - 6, kTile - 6};
                    Draw::RoundedPanel(t, 3, (r + c) % 2 == 0 ? Color{172, 188, 226, 255} : Color{152, 170, 214, 255}, {255, 255, 255, 60}, 1);
                }
            for (int i = 1; i <= 2; ++i) DrawRing(core, i * 1.3f * kTile - 1.5f, i * 1.3f * kTile + 1.5f, 0, 360, 96, {214, 172, 70, 150});
            Draw::Star(core, kTile * 1.2f, kTile * 0.45f, -PI / 2, {214, 172, 70, 90}, 8);
        } else {
            for (int i = 1; i <= 4; ++i) DrawRing(core, i * 2.6f * kTile - 1, i * 2.6f * kTile + 1, 0, 360, 96, {190, 225, 255, 40});
            for (int i = 0; i < 12; ++i) {
                Vector2 d = Dir(i * PI / 6);
                Draw::Line(Add(core, Mul(d, kTile * 1.8f)), Add(core, Mul(d, kTile * 12)), 1.4f, {190, 225, 255, 26});
            }
        }
        Draw::BlendAdd();
        Draw::Glow(core, 420, WithAlpha({255, 220, 150, 255}, 0.16f));
        Draw::Glow({kMapX + 80, kMapY + 60}, 260, WithAlpha({90, 160, 255, 255}, 0.18f));
        Draw::Glow({kMapX + kMapW - 80, kMapY + kMapH - 60}, 260, WithAlpha({90, 160, 255, 255}, 0.18f));
        for (int i = 0; i < 160; ++i) {
            Vector2 p{kMapX + rng.Float() * kMapW, kMapY + rng.Float() * kMapH};
            Draw::Glow(p, rng.Range(1.5f, 3.5f), WithAlpha({220, 236, 255, 255}, rng.Range(0.25f, 0.7f)));
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
        PathArrows(map, kTile * 0.75f, {170, 220, 255, 150});
        break;
    case Theme::Park:
        PathStroke(map, kTile * 1.02f, {92, 64, 36, 255});
        PathStroke(map, kTile * 0.9f, {196, 150, 88, 255});
        PathStroke(map, kTile * 0.72f, {222, 184, 120, 255});
        PathPlanks(map, kTile * 0.72f, 7, {170, 124, 70, 255});
        PathArrows(map, kTile * 1.5f, {110, 70, 36, 190});
        break;
    case Theme::Plaza:
        PathStroke(map, kTile * 1.0f, {214, 172, 70, 255});
        PathStroke(map, kTile * 0.9f, {150, 24, 40, 255});
        PathStroke(map, kTile * 0.78f, {200, 40, 56, 255});
        PathEmblems(map, kTile * 2.5f);
        PathArrows(map, kTile * 1.25f, {255, 214, 120, 190});
        break;
    case Theme::Studio:
        PathStroke(map, kTile * 0.92f, {48, 52, 64, 255});
        PathStroke(map, kTile * 0.78f, {28, 30, 40, 255});
        PathStroke(map, kTile * 0.62f, {54, 50, 62, 255});
        PathStars(map, kTile * 1.4f, {255, 150, 190, 255}, {214, 172, 70, 255});
        PathArrows(map, kTile * 1.4f, {255, 214, 60, 160});
        break;
    case Theme::Volcano:
        Draw::BlendAdd();
        for (const auto& path : map.paths)
            for (float d = 0; d < path.length; d += 16) Draw::Glow(path.Sample(d), kTile * 0.9f, WithAlpha({255, 90, 20, 255}, 0.07f));
        Draw::BlendNormal();
        PathStroke(map, kTile * 1.0f, {214, 84, 28, 255});
        PathStroke(map, kTile * 0.88f, {44, 32, 32, 255});
        PathStroke(map, kTile * 0.72f, {78, 60, 56, 255});
        PathPebbles(map, rng, 12, {52, 40, 38, 255}, {120, 96, 84, 255});
        PathArrows(map, kTile * 1.3f, {255, 150, 70, 170});
        break;
    case Theme::Moon:
        PathStroke(map, kTile * 1.0f, {84, 88, 102, 255});
        PathStroke(map, kTile * 0.9f, {176, 180, 192, 255});
        PathStroke(map, kTile * 0.74f, {150, 154, 168, 255});
        PathDashes(map, 6, 10, 2.0f, {110, 104, 98, 255});
        PathArrows(map, kTile * 1.2f, {110, 200, 255, 190});
        break;
    case Theme::Citadel:
        Draw::BlendAdd();
        for (const auto& path : map.paths)
            for (float d = 0; d < path.length; d += 14) Draw::Glow(path.Sample(d), kTile * 0.9f, WithAlpha({255, 200, 120, 255}, 0.06f));
        Draw::BlendNormal();
        PathStroke(map, kTile * 0.98f, {255, 222, 150, 255});
        PathStroke(map, kTile * 0.88f, {150, 70, 60, 255});
        PathStroke(map, kTile * 0.7f, {190, 92, 76, 255});
        PathArrows(map, kTile * 1.2f, {255, 226, 160, 190});
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
        case Theme::Park:
            if (d.kind == 0) Palm(d.pos, d.size);
            else if (d.kind == 1) BalloonCart(d.pos, d.size, d.seed);
            else Tree(d.pos, d.size * 0.9f, {36, 150, 72, 255}, d.seed);
            break;
        case Theme::Plaza:
            if (d.kind == 0) TimerLamp(d.pos, d.size);
            else if (d.kind == 1) Pylon(d.pos, d.size, (d.seed & 1) ? Color{255, 64, 72, 255} : Color{80, 170, 255, 255});
            else Crystal(d.pos, d.size * 0.85f, {255, 80, 90, 255}, d.seed);
            break;
        case Theme::Studio:
            if (d.kind == 0) FilmCamera(d.pos, d.size);
            else if (d.kind == 1) SpotLight(d.pos, d.size);
            else SetWall(d.pos, d.size, d.seed);
            break;
        case Theme::Volcano:
            if (d.kind == 0) Rock(d.pos, d.size, {74, 60, 58, 255});
            else if (d.kind == 1) Vent(d.pos, d.size, d.seed);
            else Crystal(d.pos, d.size * 0.85f, {255, 110, 40, 255}, d.seed);
            break;
        case Theme::Moon:
            if (d.kind == 0) Rock(d.pos, d.size, {150, 150, 160, 255});
            else if (d.kind == 1) Dish(d.pos, d.size);
            else SolarPanel(d.pos, d.size);
            break;
        case Theme::Citadel:
            if (d.kind == 0) LightTower(d.pos, d.size);
            else Crystal(d.pos, d.size * 0.9f, d.kind == 1 ? Color{120, 200, 255, 255} : Color{255, 214, 130, 255}, d.seed);
            break;
        }
    }
}

void Lighting(const MapData& map) {
    Rectangle area{kMapX, kMapY, kMapW, kMapH};
    switch (map.level->theme) {
    case Theme::City:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 150, 70, 60}, {120, 60, 140, 40});
        break;
    case Theme::Mountain:
        DrawRectangleGradientH((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 230, 170, 40}, {80, 120, 200, 30});
        break;
    case Theme::Nebula:
        break;
    case Theme::Park:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 210, 90, 50}, {40, 120, 160, 28});
        break;
    case Theme::Plaza:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {255, 80, 90, 28}, {255, 255, 255, 0});
        break;
    case Theme::Studio:
        DrawRectangleGradientH((int)area.x, (int)area.y, (int)area.width, (int)area.height, {40, 80, 140, 36}, {255, 180, 60, 24});
        break;
    case Theme::Volcano:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {60, 20, 40, 50}, {255, 100, 30, 40});
        break;
    case Theme::Moon:
        DrawRectangleGradientH((int)area.x, (int)area.y, (int)area.width, (int)area.height, {10, 16, 40, 60}, {140, 180, 255, 24});
        break;
    case Theme::Citadel:
        DrawRectangleGradientV((int)area.x, (int)area.y, (int)area.width, (int)area.height, {120, 180, 255, 26}, {255, 210, 140, 20});
        break;
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

constexpr Color kWallTop{226, 234, 252, 255}, kWallFace{122, 140, 196, 255}, kWallEdge{40, 52, 104, 255};

void CastleTower(Vector2 c, float radius, Color roof, Color roofLit) {
    float body = radius * 0.7f;
    Draw::GlowStretched(Add(c, {5, radius * 0.75f}), radius * 1.5f, radius * 0.55f, 0, WithAlpha(BLACK, 0.45f));
    Draw::Ellipse({c.x, c.y + body * 0.4f}, radius + 1.5f, radius * 0.42f + 1.5f, kWallEdge);
    DrawRectangleRec({c.x - radius - 1.5f, c.y - body * 0.6f, radius * 2 + 3, body}, kWallEdge);
    Draw::Ellipse({c.x, c.y + body * 0.4f}, radius, radius * 0.42f, kWallFace);
    DrawRectangleRec({c.x - radius, c.y - body * 0.6f, radius * 2, body}, kWallFace);
    DrawRectangleRec({c.x - radius, c.y - body * 0.6f, radius * 0.6f, body}, Lighten(kWallFace, 0.2f));
    Vector2 top{c.x, c.y - body * 0.6f};
    Draw::Ellipse(top, radius + 1.5f, radius * 0.42f + 1.5f, kWallEdge);
    Draw::Ellipse(top, radius, radius * 0.42f, kWallTop);
    for (int i = 0; i < 8; ++i) {
        float a = i * PI / 4 + PI / 8;
        Vector2 m = Add(top, {std::cos(a) * radius * 0.86f, std::sin(a) * radius * 0.36f});
        DrawRectangleRec({m.x - 3, m.y - 6, 6, 6}, kWallEdge);
        DrawRectangleRec({m.x - 2.2f, m.y - 5.2f, 4.4f, 4.4f}, kWallTop);
    }
    float h = radius * 1.7f;
    Draw::Tri({top.x - radius * 0.72f, top.y}, {top.x + radius * 0.72f, top.y}, {top.x, top.y - h}, roof);
    Draw::Tri({top.x - radius * 0.72f, top.y}, {top.x, top.y}, {top.x, top.y - h}, roofLit);
    Draw::Ellipse(top, radius * 0.74f, radius * 0.22f, Darken(roof, 0.25f));
    Draw::BlendAdd();
    Draw::Glow({top.x, top.y - h}, radius * 0.55f, WithAlpha({255, 220, 140, 255}, 0.7f));
    Draw::BlendNormal();
}

// Land of Light castle: walls along the border cells, a gatehouse wherever a road crosses, towers at the corners.
void CastleWalls(const MapData& map) {
    const LevelDef& def = *map.level;
    const float w = 24, h = 12;
    auto gate = [&](int c, int r) { return map.cells[r][c] == Cell::Path; };
    float left = TileCenter(def.castleMin.col, 0).x, right = TileCenter(def.castleMax.col, 0).x;
    float top = TileCenter(0, def.castleMin.row).y, bottom = TileCenter(0, def.castleMax.row).y;
    auto merlons = [](Vector2 a, Vector2 b, Vector2 out) {
        float length = Len(Sub(b, a));
        Vector2 d = Normalize(Sub(b, a));
        for (float s = 7; s < length - 4; s += 13) {
            Vector2 m = Add(Add(a, Mul(d, s)), out);
            DrawRectangleRec({m.x - 4, m.y - 7, 8, 8}, kWallEdge);
            DrawRectangleRec({m.x - 3, m.y - 6, 6, 4}, kWallTop);
            DrawRectangleRec({m.x - 3, m.y - 2, 6, 2}, kWallFace);
        }
    };
    auto horizontal = [&](int row, float y, float outSide) {
        int c = def.castleMin.col;
        while (c <= def.castleMax.col) {
            if (gate(c, row)) {
                ++c;
                continue;
            }
            int end = c;
            while (end + 1 <= def.castleMax.col && !gate(end + 1, row)) ++end;
            float x0 = c == def.castleMin.col ? left : kMapX + c * kTile, x1 = end == def.castleMax.col ? right : kMapX + (end + 1) * kTile;
            Rectangle face{x0, y - w / 2 + w - h / 2, x1 - x0, h}, topFace{x0, y - w / 2 - h / 2, x1 - x0, w};
            DrawRectangleRec({x0 - 1.5f, topFace.y - 1.5f, x1 - x0 + 3, w + h + 3}, kWallEdge);
            Draw::VerticalGradientRect(face, kWallFace, Darken(kWallFace, 0.3f));
            DrawRectangleRec(topFace, kWallTop);
            DrawRectangleRec({x0, topFace.y + w / 2 - 1, x1 - x0, 2}, {180, 196, 236, 255});
            for (float x = x0 + 10; x < x1 - 6; x += 18) DrawRectangleRec({x, face.y + 3, 1.5f, h - 6}, Darken(kWallFace, 0.4f));
            float edge = outSide < 0 ? topFace.y + 2 : topFace.y + w - 2;
            merlons({x0, edge}, {x1, edge}, {0, 0});
            c = end + 1;
        }
    };
    auto vertical = [&](int col, float x, float outSide) {
        int r = def.castleMin.row;
        while (r <= def.castleMax.row) {
            if (gate(col, r)) {
                ++r;
                continue;
            }
            int end = r;
            while (end + 1 <= def.castleMax.row && !gate(col, end + 1)) ++end;
            float y0 = r == def.castleMin.row ? top : kMapY + r * kTile, y1 = end == def.castleMax.row ? bottom : kMapY + (end + 1) * kTile;
            bool open = end != def.castleMax.row; // a gate below shows the wall's front face
            DrawRectangleRec({x - w / 2 - 1.5f, y0 - h / 2 - 1.5f, w + 3, y1 - y0 + (open ? h : 0) + 3}, kWallEdge);
            DrawRectangleRec({x - w / 2, y0 - h / 2, w, y1 - y0}, kWallTop);
            DrawRectangleRec({x - w / 2, y0 - h / 2, 3, y1 - y0}, WHITE);
            DrawRectangleRec({x + w / 2 - 3, y0 - h / 2, 3, y1 - y0}, {180, 196, 236, 255});
            if (open) Draw::VerticalGradientRect({x - w / 2, y1 - h / 2, w, h}, kWallFace, Darken(kWallFace, 0.3f));
            float edge = x + outSide * (w / 2 - 3);
            merlons({edge, y0 - h / 2 + 4}, {edge, y1 - h / 2 + 4}, {0, 4});
            r = end + 1;
        }
    };
    horizontal(def.castleMin.row, top, -1);
    vertical(def.castleMin.col, left, -1);
    vertical(def.castleMax.col, right, 1);
    horizontal(def.castleMax.row, bottom, 1);
    Color red{206, 44, 54, 255}, redLit{240, 96, 96, 255};
    auto gateTowers = [&](Vector2 a, Vector2 b) {
        CastleTower(a, 13, red, redLit);
        CastleTower(b, 13, red, redLit);
    };
    for (int c = def.castleMin.col + 1; c < def.castleMax.col; ++c) {
        if (gate(c, def.castleMin.row)) gateTowers({kMapX + c * kTile - 3, top}, {kMapX + (c + 1) * kTile + 3, top});
        if (gate(c, def.castleMax.row)) gateTowers({kMapX + c * kTile - 3, bottom}, {kMapX + (c + 1) * kTile + 3, bottom});
    }
    for (int r = def.castleMin.row + 1; r < def.castleMax.row; ++r) {
        if (gate(def.castleMin.col, r)) gateTowers({left, kMapY + r * kTile - 6}, {left, kMapY + (r + 1) * kTile + 2});
        if (gate(def.castleMax.col, r)) gateTowers({right, kMapY + r * kTile - 6}, {right, kMapY + (r + 1) * kTile + 2});
    }
    Color blue{70, 130, 226, 255}, blueLit{140, 196, 255, 255};
    for (Vector2 corner : {Vector2{left, top}, Vector2{right, top}, Vector2{left, bottom}, Vector2{right, bottom}})
        CastleTower(corner, 21, blue, blueLit);
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

// The castle's guardian lighthouse: a white tower banded in Ultra red, its lantern sweeping two beams.
void Lighthouse(Vector2 p, float time, float flash) {
    Draw::GlowStretched(Add(p, {8, 50}), 84, 24, 0, WithAlpha(BLACK, 0.5f));
    Draw::Ellipse({p.x, p.y + 42}, 52, 18, kWallEdge);
    Draw::Ellipse({p.x, p.y + 40}, 50, 16, kWallFace);
    Draw::Ellipse({p.x, p.y + 35}, 48, 15, kWallTop);
    Draw::Ellipse({p.x, p.y + 31}, 36, 11, kWallFace);
    Draw::Ellipse({p.x, p.y + 27}, 35, 10, {236, 242, 255, 255});
    const float base = p.y + 28, crown = p.y - 72;
    auto half = [&](float y) { return 21 - (base - y) / (base - crown) * 8; };
    auto band = [&](float y0, float y1, Color c, Color shade) {
        Draw::Quad({p.x - half(y0), y0}, {p.x + half(y0), y0}, {p.x + half(y1), y1}, {p.x - half(y1), y1}, c);
        Draw::Quad({p.x + half(y0) * 0.4f, y0}, {p.x + half(y0), y0}, {p.x + half(y1), y1}, {p.x + half(y1) * 0.4f, y1}, shade);
        Draw::Quad({p.x - half(y0), y0}, {p.x - half(y0) * 0.6f, y0}, {p.x - half(y1) * 0.6f, y1}, {p.x - half(y1), y1}, Lighten(c, 0.25f));
    };
    band(base, crown, {236, 240, 250, 255}, {176, 188, 220, 255});
    for (int i = 0; i < 3; ++i) {
        float y0 = base - 16 - i * 30.0f;
        band(y0, y0 - 11, {214, 46, 52, 255}, {156, 28, 40, 255});
    }
    for (float y : {base - 32.0f, base - 62.0f}) DrawRectangleRec({p.x - 2.5f, y - 7, 5, 8}, {40, 52, 104, 255});
    Draw::BlendAdd();
    Draw::Glow({p.x, base - 90}, 7, WithAlpha({90, 200, 255, 255}, 0.9f)); // colour timer
    Draw::BlendNormal();
    DrawCircleV({p.x, base - 90}, 3, {170, 230, 255, 255});
    Draw::Ellipse({p.x, crown}, 22, 6, kWallEdge);
    Draw::Ellipse({p.x, crown - 1}, 20, 5, {200, 210, 236, 255});
    Vector2 lamp = Add(p, kLanternOffset);
    DrawRectangleRec({p.x - 11, crown - 22, 22, 20}, kWallEdge);
    DrawRectangleRec({p.x - 9.5f, crown - 21, 19, 18}, {255, 232, 160, 255});
    for (float x : {-4.0f, 4.0f}) DrawRectangleRec({p.x + x - 0.75f, crown - 21, 1.5f, 18}, {120, 96, 60, 255});
    for (int i = 0; i < 10; ++i) DrawRectangleRec({p.x - 20 + i * 4.2f, crown - 6, 1.2f, 5}, kWallEdge); // gallery rail
    DrawCircleSector({p.x, crown - 22}, 13, 180, 360, 20, {196, 40, 50, 255});
    DrawCircleSector({p.x - 3, crown - 23}, 7, 180, 360, 14, {240, 100, 100, 255});
    Draw::Line({p.x, crown - 34}, {p.x, crown - 42}, 2, kWallEdge);
    DrawCircleV({p.x, crown - 43}, 2.6f, {255, 214, 120, 255});
    Draw::BlendAdd();
    float pulse = 0.85f + 0.15f * std::sin(time * 2.6f);
    for (int k = 0; k < 2; ++k) {
        float a = time * 0.9f + k * PI;
        Vector2 far = Add(lamp, {std::cos(a) * 300, std::sin(a) * 300 * 0.4f});
        Vector2 side = Normalize({lamp.y - far.y, far.x - lamp.x});
        for (int j = 0; j < 3; ++j) {
            float spread = 16.0f + j * 16;
            Draw::Tri(lamp, Add(far, Mul(side, spread)), Sub(far, Mul(side, spread)), WithAlpha({255, 236, 180, 255}, 0.09f - j * 0.025f));
        }
    }
    Draw::Glow(lamp, 80 * pulse, WithAlpha({255, 214, 140, 255}, 0.5f));
    Draw::Glow(lamp, 30, WithAlpha({255, 246, 210, 255}, 0.9f));
    Draw::Glow(lamp, 12, WithAlpha(WHITE, 1.0f));
    if (flash > 0) Draw::Glow(p, 110, WithAlpha({255, 60, 60, 255}, flash * 0.8f));
    Draw::BlendNormal();
}

void Base(const MapData& map, float time, float flash) {
    Vector2 p = map.basePos;
    Theme theme = map.level->theme;
    if (theme == Theme::Park) {
        ParkGate(p, time, flash);
        return;
    }
    if (theme == Theme::Plaza) {
        TimerMonument(p, time, flash);
        return;
    }
    if (theme == Theme::Studio) {
        StudioGlobe(p, time, flash);
        return;
    }
    if (theme == Theme::Moon) {
        MoonBase(p, time, flash);
        return;
    }
    if (theme == Theme::Citadel) {
        Lighthouse(p, time, flash);
        return;
    }
    bool nebula = theme == Theme::Nebula;
    Draw::GlowStretched(Add(p, {6, 52}), 72, 26, 0, WithAlpha(BLACK, 0.5f));
    Color hurt = WithAlpha({255, 60, 60, 255}, flash * 0.8f);
    if (nebula) {
        // Plasma Spark Tower
        Draw::RoundedPanel({p.x - 46, p.y + 20, 92, 40}, 10, {60, 80, 150, 255}, {150, 200, 255, 255}, 2);
        Draw::Quad({p.x - 22, p.y + 24}, {p.x + 22, p.y + 24}, {p.x + 10, p.y - 58}, {p.x - 10, p.y - 58}, {150, 180, 230, 255});
        Draw::Quad({p.x - 22, p.y + 24}, {p.x - 6, p.y + 24}, {p.x - 3, p.y - 58}, {p.x - 10, p.y - 58}, {200, 220, 250, 255});
        for (int i = 0; i < 3; ++i) DrawRing({p.x, p.y - 20.0f - i * 14}, 16.0f - i * 3, 19.0f - i * 3, 200, 340, 16, {220, 236, 255, 255});
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
        DrawRectangleRec({p.x - 42 + i * 15.0f, p.y + 30, 9, 8},
                         (int)(time * 2 + i) % 5 == 0 ? Color{255, 240, 180, 255} : Color{255, 200, 100, 255});
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

RenderTexture2D RenderMap(const MapData& map, float scale, bool builtinGroundOnly, bool withLandmarks) {
    RenderTexture2D target = LoadRenderTexture((int)(kMapW * scale), (int)(kMapH * scale));
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(target);
    ClearBackground(BLACK);
    rlPushMatrix();
    rlScalef(scale, scale, 1);
    rlTranslatef(-kMapX, -kMapY, 0);
    Texture2D ground = builtinGroundOnly ? Texture2D{} : Skins::MapGround(*map.level);
    if (ground.id) {
        DrawTexturePro(ground, {0, 0, (float)ground.width, (float)ground.height}, {kMapX, kMapY, kMapW, kMapH}, {0, 0}, 0, WHITE);
    } else {
        Ground(map);
    }
    if (!builtinGroundOnly) {
        if (!ground.id || Skins::MapPaths()) Paths(map);
        if (map.level->HasCastle()) CastleWalls(map);
        if (!ground.id || Skins::MapDecor()) DecorLayer(map);
        if (withLandmarks)
            for (const auto& spot : map.landmarks) DrawLandmark(spot, 0, true);
        Lighting(map);
    }
    rlPopMatrix();
    EndTextureMode();
    return target;
}

void DrawMapTexture(const RenderTexture2D& texture, Rectangle dest) { Display::PresentOpaque(texture.texture, dest); }

void DrawMapLive(const MapData& map, float time, float baseDamageFlash) {
    for (const auto& spot : map.landmarks) DrawLandmark(spot, time, true);
    Color tone = ThemeAccent(map.level->theme);
    if (map.level->theme == Theme::City || map.level->theme == Theme::Mountain) tone = {200, 40, 120, 255};
    std::vector<Vector2> drawn;
    for (const auto& path : map.paths) {
        Vector2 p = path.Sample(kTile * 0.9f);
        p.x = std::clamp(p.x, kMapX + 18, kMapX + kMapW - 18);
        p.y = std::clamp(p.y, kMapY + 18, kMapY + kMapH - 18);
        bool dup = false;
        for (auto& q : drawn) dup |= Len(Sub(q, p)) < 4;
        if (dup) continue;
        drawn.push_back(p);
        Portal(p, time, tone);
    }
    Base(map, time, baseDamageFlash);
}

} // namespace UG
