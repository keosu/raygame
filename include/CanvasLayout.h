#pragma once

#include "Core.h"

namespace RayGame {

// Anchor a reference layout inside the current logical canvas or a supplied safe
// content rectangle. Insets must already be converted from pixels to canvas units.
// Anchor {0,0} is top-left, {1,1} bottom-right, and {.5,.5} the center.
struct CanvasLayout {
    Vector2 reference{1280, 720};
    Rectangle content{0, 0, 1280, 720};

    Vector2 Offset(Vector2 anchor) const {
        return {content.x + (content.width - reference.x) * anchor.x,
                content.y + (content.height - reference.y) * anchor.y};
    }

    Vector2 Place(Vector2 point, Vector2 anchor) const {
        auto offset = Offset(anchor);
        return {point.x + offset.x, point.y + offset.y};
    }

    Rectangle Place(Rectangle rect, Vector2 anchor) const {
        auto offset = Offset(anchor);
        return {rect.x + offset.x, rect.y + offset.y, rect.width, rect.height};
    }
};

} // namespace RayGame
