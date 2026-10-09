#include "Physics.h"
#include "Scene.h"

namespace RayGame {

void PhysicsWorld::AddCollider(Collider* collider) {
    if (!collider || !collider->GetGameObject() || registered.count(collider)) return;
    auto* scene = collider->GetGameObject()->scene;
    if (scene && &scene->GetPhysicsWorld() != this)
        throw std::invalid_argument("A scene collider belongs to that scene's physics world");
    if (owner && scene != owner)
        throw std::invalid_argument("Cannot register a foreign object in a scene physics world");
    if (collider->world && collider->world != this)
        throw std::logic_error("Remove a collider from its previous world before registering it");
    for (const auto& comp : collider->GetGameObject()->GetAllComponents()) {
        if (comp.get() == collider) {
            colliders.push_back(std::static_pointer_cast<Collider>(comp));
            registered.insert(collider);
            collider->world = this;
            break;
        }
    }
}

void PhysicsWorld::RemoveCollider(Collider* collider) {
    if (!collider || collider->world != this) return;
    collider->world = nullptr;
    registered.erase(collider);
    colliders.erase(std::remove_if(colliders.begin(), colliders.end(),
        [collider](const Ref<Collider>& c) { return c.get() == collider; }), colliders.end());
    // Contact references survive until the next step sends Exit to surviving objects.
}

void Collider::Start() {
    if (gameObject && gameObject->scene) gameObject->scene->GetPhysicsWorld().AddCollider(this);
}
void Collider::OnEnable() { if (HasStarted() && !world) Start(); }
void Collider::OnDestroy() { if (world) world->RemoveCollider(this); }

} // namespace RayGame

