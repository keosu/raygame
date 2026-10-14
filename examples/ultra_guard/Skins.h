#pragma once

#include "Data.h"

// Art packs under assets/skins/<folder>/ (listed in assets/skins/skins.txt). A skin may
// contain skin.txt (name, palette, sprite options), avatars/<hero>.png, kaiju/<kaiju>.png
// and maps/<level id or theme>.png; anything missing falls back to the built-in art.
namespace UG::Skins {

struct Skin {
    std::string folder; // empty for the built-in look
    std::string name;
};

// Frames are laid out left to right; the feet sit at `feet` (fraction of the image height).
struct KaijuSprite {
    Texture2D texture{};
    int frames = 1;
    float fps = 8;
    float size = 80; // drawn height in canvas pixels at kaiju scale 1
    float feet = 0.94f;
};

// Reads the index and applies `folder` (or the saved choice when empty). Needs a window.
void Load(const std::string& folder = "");
const std::vector<Skin>& All(); // [0] is the built-in look
int Current();
// Applies a skin (and saves the choice); scenes holding baked textures must be reloaded.
void Select(int index, bool save = true);

// Asset path of `relative` inside the current skin, or "" if the skin does not provide it.
std::string Find(const std::string& relative);
const KaijuSprite* Kaiju(EnemyId id);
// Replacement ground for a level (by id, then theme name); id 0 when the skin has none.
Texture2D MapGround(const LevelDef& level);
bool MapPaths(); // draw the built-in roads over a replacement ground
bool MapDecor(); // draw the built-in buildings, trees and crystals over it
const std::vector<std::string>& Errors();

// Writes the built-in art as an editable skin: skin.txt, avatars/, kaiju/ strips, maps/.
bool ExportTemplate(const std::string& dir);

} // namespace UG::Skins
