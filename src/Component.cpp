#include "Component.h"
#include "GameObject.h"

namespace RayGame {

float Component::RenderDepth() const {
    if (!ySort || !gameObject || !gameObject->transform) return 0;
    float y = gameObject->transform->GetRenderPosition().y;
    return std::isfinite(y) ? y : 0.0f; // NaN would break the sort's ordering contract

}

} // namespace RayGame
