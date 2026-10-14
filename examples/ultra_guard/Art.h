#pragma once

#include "Data.h"

namespace UG::Art {

// Renders the hero busts into textures. Needs a live window; call outside Display::Begin/End.
void BuildPortraits();

// Portraits are 256x256 with a transparent background, head centered near (128, 108).
// A PNG at assets/avatars/<key>.png replaces the drawn portrait.
Texture2D Portrait(HeroId id);
bool HasCustomAvatar(HeroId id);
void DrawPortrait(HeroId id, Rectangle dest, Color tint = WHITE);
// Eye positions in portrait space, used for attack flashes.
Vector2 EyePoint(HeroId id, int side);
// Vector bust in portrait space (center 128,108); exposed for previews.
void DrawBust(HeroId id);

struct KaijuPose {
    Vector2 feet{};
    float scale = 1;
    float time = 0;
    float face = 1; // +1 faces right
    float flash = 0;
    float alpha = 1;
    float chill = 0; // frozen tint
    float burn = 0;
    bool shielded = false;
    bool enraged = false;
};

void DrawKaiju(EnemyId id, const KaijuPose& pose);
// Approximate visual height in pixels at scale 1.
float KaijuHeight(EnemyId id);

} // namespace UG::Art
