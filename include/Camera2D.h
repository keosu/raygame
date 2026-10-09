#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"

namespace RayGame {

class RAYGAME_API Camera2DComponent : public Component {
public:
    Vector2 offset = {0, 0};
    float zoom = 1.0f;
    float smoothSpeed = 5.0f;
    bool followEnabled = false;
    GameObject* target = nullptr;
    
    Camera2DComponent() {
        camera.target = {0, 0};
        camera.offset = offset;
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
    }
    
    void SetTarget(GameObject* obj) {
        target = obj;
        followTransform = obj ? obj->transform : std::weak_ptr<Transform>{};
        followEnabled = (obj != nullptr);
    }
    
    void LateUpdate() override {
        auto follow = followTransform.lock();
        auto* owner = follow ? follow->GetGameObject() : nullptr;
        if (followEnabled && owner && owner->active && !owner->IsDestroyed()) {
            Vector2 targetPos = follow->GetRenderPosition();
            Vector2 currentTarget = camera.target;
            
            // Smooth follow
            camera.target = Lerp(currentTarget, targetPos, 
                               1.0f - std::exp(-std::max(0.0f, smoothSpeed) * GameTime::DeltaTime()));
        } else if (!owner) {
            target = nullptr;
            followEnabled = false;
        }
        
        camera.zoom = zoom;
        camera.offset = offset;
    }
    
    void Begin() {
        BeginMode2D(camera);
    }
    
    void End() {
        EndMode2D();
    }
    
    Vector2 ScreenToWorld(Vector2 screenPos) {
        return GetScreenToWorld2D(screenPos, camera);
    }
    
    Vector2 WorldToScreen(Vector2 worldPos) {
        return GetWorldToScreen2D(worldPos, camera);
    }
    
    Camera2D& GetCamera() { return camera; }
    
private:
    Camera2D camera;
    std::weak_ptr<Transform> followTransform;
};

}

