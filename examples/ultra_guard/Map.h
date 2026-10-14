#pragma once

#include "Data.h"

namespace UG {

enum class Cell : std::uint8_t { Open, Path, Blocked, Base };

struct PathLine {
    std::vector<Vector2> points;
    std::vector<float> cumulative;
    float length = 0;
    Vector2 Sample(float distance, Vector2* direction = nullptr) const;
};

struct Decor {
    int kind;
    int col, row;
    Vector2 pos;
    float size;
    std::uint32_t seed;
};

struct MapData {
    const LevelDef* level = nullptr;
    std::array<std::array<Cell, kCols>, kRows> cells{};
    std::vector<PathLine> paths;
    std::vector<Decor> decor;
    Vector2 basePos{};

    void Build(const LevelDef& def);
    bool InGrid(int col, int row) const { return col >= 0 && row >= 0 && col < kCols && row < kRows; }
    bool Buildable(int col, int row) const { return InGrid(col, row) && cells[row][col] == Cell::Open; }
};

// Static background in canvas coordinates, rendered at `scale` for supersampling.
RenderTexture2D RenderMap(const MapData& map, float scale);
// Opaque copy: translucent decoration lowered the texture's alpha while baking.
void DrawMapTexture(const RenderTexture2D& texture, Rectangle dest);
// Animated elements above the static layer: portals, the base and ambience.
void DrawMapLive(const MapData& map, float time, float baseDamageFlash);

} // namespace UG
