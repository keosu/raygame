#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API Component {
public:
    Component() : gameObject(nullptr), enabled(true) {}
    virtual ~Component() = default;
    
    virtual void Start() {}
    virtual void Update() {}
    virtual void Render() {}
    virtual void OnDestroy() {}
    
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool value) { enabled = value; }
    
    GameObject* GetGameObject() const { return gameObject; }
    
protected:
    friend class GameObject;
    GameObject* gameObject;
    bool enabled;
};

class RAYGAME_API Transform : public Component {
public:
    Vector2 position = {0, 0};
    float rotation = 0.0f;  // degrees
    Vector2 scale = {1, 1};
    
    Transform() = default;
    Transform(Vector2 pos) : position(pos) {}
    Transform(Vector2 pos, float rot, Vector2 scl) 
        : position(pos), rotation(rot), scale(scl) {}
    
    void Translate(Vector2 offset) {
        position.x += offset.x;
        position.y += offset.y;
    }
    
    void Rotate(float degrees) {
        rotation += degrees;
    }
    
    void Scale(Vector2 factor) {
        scale.x *= factor.x;
        scale.y *= factor.y;
    }
    
    Vector2 Forward() const {
        float rad = rotation * DEG2RAD;
        return {std::cos(rad), std::sin(rad)};
    }
    
    Vector2 Right() const {
        float rad = (rotation + 90) * DEG2RAD;
        return {std::cos(rad), std::sin(rad)};
    }
};

}

