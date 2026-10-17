#pragma once

#include "Data.h"

namespace UG::Art {

// Renders the hero busts into textures. Needs a live window; call outside Display::Begin/End.
void BuildPortraits();
// Baking onto a transparent target needs coverage-correct alpha: standard blending would let
// translucent highlights make opaque shapes underneath see-through. End with EndBlendMode().
void BeginBakeBlend();
// Rebuilds every portrait, e.g. after switching skins.
void ReloadPortraits();

// Portraits are 256x256 with a transparent background, head centered near (128, 108).
// A PNG at <skin>/avatars/<key>.png or assets/avatars/<key>.png replaces the drawn portrait.
Texture2D Portrait(HeroId id);
bool HasCustomAvatar(HeroId id);
void DrawPortrait(HeroId id, Rectangle dest, Color tint = WHITE);
// Database portrait by codex id: a hero's portrait, else assets/avatars/<id>.png; empty if neither.
Texture2D CodexPortrait(const std::string& id);
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

// Uses the current skin's sprite when it has one.
void DrawKaiju(EnemyId id, const KaijuPose& pose);
void DrawBuiltinKaiju(EnemyId id, const KaijuPose& pose);
// Approximate visual height in pixels at scale 1.
float KaijuHeight(EnemyId id);

} // namespace UG::Art
