#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "Time.h"

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
        camera.offset = {400, 300};  // Screen center by default
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
    }
    
    void SetTarget(GameObject* obj) {
        target = obj;
        followEnabled = (obj != nullptr);
    }
    
    void Update() override {
        if (followEnabled && target && target->active) {
            Vector2 targetPos = target->transform->position;
            Vector2 currentTarget = camera.target;
            
            // Smooth follow
            camera.target = Lerp(currentTarget, targetPos, 
                               smoothSpeed * GameTime::DeltaTime());
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
};

}

