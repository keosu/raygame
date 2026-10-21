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

// Big theme set pieces that make the park stages recognisable; they reserve their cells.
enum class Landmark { FerrisWheel, Safari, WhaleShark, BetaCapsule, JetBeetle, Castle, Dinosaur, Soundstage };

struct LandmarkSpot {
    Landmark kind;
    int col, row, w, h;
    Rectangle Area() const { return {kMapX + col * kTile, kMapY + row * kTile, w * kTile, h * kTile}; }
};

constexpr Vector2 kLanternOffset{0, -84}; // siege lighthouse lamp, relative to the base position

struct MapData {
    const LevelDef* level = nullptr;
    std::array<std::array<Cell, kCols>, kRows> cells{};
    std::vector<PathLine> paths;
    std::vector<Decor> decor;
    std::vector<LandmarkSpot> landmarks;
    Vector2 basePos{};

    void Build(const LevelDef& def);
    void PlaceLandmarks();
    bool InGrid(int col, int row) const { return col >= 0 && row >= 0 && col < kCols && row < kRows; }
    bool Buildable(int col, int row) const { return InGrid(col, row) && cells[row][col] == Cell::Open; }
    bool InCastle(int col, int row) const {
        return level->HasCastle() && col >= level->castleMin.col && col <= level->castleMax.col && row >= level->castleMin.row &&
               row <= level->castleMax.row;
    }
};

// Static background in canvas coordinates, rendered at `scale` for supersampling. A skin's
// maps/<level id or theme>.png replaces the ground; `builtinGroundOnly` exports the plain ground.
// Landmarks animate, so battles draw them live (`withLandmarks` false) and previews bake them.
RenderTexture2D RenderMap(const MapData& map, float scale, bool builtinGroundOnly = false, bool withLandmarks = true);
// Opaque copy: translucent decoration lowered the texture's alpha while baking.
void DrawMapTexture(const RenderTexture2D& texture, Rectangle dest);
// Animated elements above the static layer: portals, the base and ambience.
void DrawMapLive(const MapData& map, float time, float baseDamageFlash);

} // namespace UG
