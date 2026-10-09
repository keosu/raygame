#include "GameObject.h"
#include "Physics.h"

namespace RayGame {

void GameObject::IntegratePhysics(float dt) {
    Start();
    const auto snapshot = components;
    for (const auto& comp : snapshot) {
        if (!active || destroyed) break;
        auto body = std::dynamic_pointer_cast<Rigidbody>(comp);
        if (body && body->IsEnabled() && body->HasStarted()) body->Integrate(dt);
    }
}

// GameObject implementation - template methods must stay in header
// This file exists for future non-template implementations if needed

} // namespace RayGame

