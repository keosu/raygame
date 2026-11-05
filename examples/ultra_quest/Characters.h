#pragma once

#include "Common.h"

namespace UQ {

// Drawn people for the street and the dialogue portraits. No photos of the cast: everyone
// human is a simple, stylised figure built from shapes.
struct Look {
    Color skin = {250, 212, 178, 255};
    Color hair = {28, 26, 34, 255};
    Color top = {44, 56, 96, 255};
    Color accent = {255, 140, 40, 255};
    Color legs = {52, 56, 70, 255};
    enum class Hair { Spiky, Short, Grey, Cap, Helmet, Band } hair_style = Hair::Spiky;
    Color hat = {0, 0, 0, 0};
    bool apron = false;
    bool backpack = false;
    float height = 150; // standing height in street pixels
};

const Look& LookFor(const std::string& key);

// `feet` is the ground point; `walk` is the walk-cycle phase in radians; `react` > 0 makes the
// figure flinch (tremors); `pose` 1 raises the right arm (the Z Riser).
void DrawPerson(const Look& look, Vector2 feet, int facing, float walk, float react = 0, float pose = 0);
void DrawPortrait(const std::string& key, Rectangle r);

} // namespace UQ
