#include "Scene.h"

namespace RayGame {

void Scene::BeginFixedStep() {
    if (!started) Start();
    FlushPending();
    for (const auto& obj : gameObjects)
        if (obj && obj->active && !obj->IsDestroyed()) obj->transform->CapturePreviousPose();
}

void Scene::IntegratePhysics(float dt) {
    FlushPending();
    Traversal traversal(*this);
    for (const auto& obj : gameObjects)
        if (obj && obj->active && !obj->IsDestroyed()) obj->IntegratePhysics(dt);
    traversal.End();
    FlushPending();
}

void Scene::PostFixedUpdate() {
    FlushPending();
    Traversal traversal(*this);
    for (const auto& obj : gameObjects)
        if (obj && obj->active && !obj->IsDestroyed()) obj->PostFixedUpdate();
    traversal.End();
    FlushPending();
}

void Scene::LateUpdate() {
    FlushPending();
    Traversal traversal(*this);
    for (const auto& obj : gameObjects)
        if (obj && obj->active && !obj->IsDestroyed()) obj->LateUpdate();
    traversal.End();
    FlushPending();
}

void Scene::SetInterpolationAlpha(float alpha) {
    interpolationAlpha = std::isfinite(alpha) ? Clamp(alpha, 0.0f, 1.0f) : 1;
    for (const auto& obj : gameObjects) if (obj) obj->transform->SetRenderAlpha(alpha);
}

void Scene::ResetInterpolation() {
    interpolationAlpha = 1;
    for (const auto& obj : gameObjects) if (obj) obj->transform->ResetInterpolation();
    for (const auto& obj : pendingAdd) if (obj) obj->transform->ResetInterpolation();
}

} // namespace RayGame

