#pragma once

#include "Core.h"
#include "ResourceHandle.h"
#include "Component.h"
#include "GameObject.h"
#include <set>

namespace RayGame {

// Grid-based tilemap. Tile id -1 = empty. Tiles index into a tileset texture
// laid out left-to-right, top-to-bottom.
class RAYGAME_API Tilemap : public Component {
public:
    TextureHandle tileset = nullptr;
    int tileWidth = 32;
    int tileHeight = 32;
    int columns = 0;   // grid columns
    int rows = 0;      // grid rows
    Color tint = WHITE;
    std::set<int> solidTiles;  // tile ids treated as solid

    Tilemap() = default;

    void Resize(int cols, int rws, int fill = -1) {
        columns = std::max(0, cols); rows = std::max(0, rws);
        tiles.assign((size_t)columns * rows, fill);
    }

    void SetTile(int x, int y, int id) {
        if (!InBounds(x, y)) return;
        tiles[Index(x, y)] = id;
    }

    int GetTile(int x, int y) const {
        if (!InBounds(x, y)) return -1;
        return tiles[Index(x, y)];
    }

    bool IsSolid(int x, int y) const {
        int id = GetTile(x, y);
        return id >= 0 && solidTiles.count(id) > 0;
    }

    bool IsSolidAtWorld(Vector2 world) const {
        if (tileWidth <= 0 || tileHeight <= 0) return false;
        Vector2 local = gameObject ? gameObject->transform->InverseTransformPoint(world) : world;
        int tx = (int)std::floor(local.x / tileWidth);
        int ty = (int)std::floor(local.y / tileHeight);
        return IsSolid(tx, ty);
    }

    Vector2 TileToWorld(int x, int y) const {
        Vector2 local = {x * (float)tileWidth, y * (float)tileHeight};
        return gameObject ? gameObject->transform->TransformPoint(local) : local;
    }

    void Render() override {
        if (columns <= 0 || rows <= 0 || tileWidth <= 0 || tileHeight <= 0) return;
        int tilesetCols = tileset ? tileset->width / tileWidth : 1;
        if (tilesetCols <= 0) return;

        Vector2 scale = gameObject ? gameObject->transform->GetWorldScale() : Vector2{1, 1};
        float rotation = gameObject ? gameObject->transform->GetWorldRotation() : 0;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < columns; x++) {
                int id = tiles[Index(x, y)];
                if (id < 0) continue;
                int sx = (id % tilesetCols) * tileWidth;
                int sy = (id / tilesetCols) * tileHeight;
                Rectangle src = {(float)sx, (float)sy, (float)tileWidth, (float)tileHeight};
                Vector2 pos = TileToWorld(x, y);
                if (scale.x < 0) src.width = -src.width;
                if (scale.y < 0) src.height = -src.height;
                Rectangle dest = {pos.x, pos.y, std::fabs(tileWidth * scale.x), std::fabs(tileHeight * scale.y)};
                Vector2 origin = {scale.x < 0 ? dest.width : 0, scale.y < 0 ? dest.height : 0};
                if (tileset) DrawTexturePro(*tileset, src, dest, origin, rotation, tint);
                else DrawRectanglePro(dest, origin, rotation, MAGENTA);
            }
        }
    }

    const std::vector<int>& GetTiles() const { return tiles; }

private:
    std::vector<int> tiles;

    int Index(int x, int y) const { return y * columns + x; }
    bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < columns && y < rows; }
};

}
