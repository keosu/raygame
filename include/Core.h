#pragma once

#include <raylib.h>
#include <raymath.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>

// DLL Export/Import macros
#ifdef _WIN32
    #ifdef RAYGAME_EXPORTS
        #define RAYGAME_API __declspec(dllexport)
    #else
        #define RAYGAME_API __declspec(dllimport)
    #endif
#else
    #define RAYGAME_API
#endif

// Forward declarations
namespace RayGame {
    class GameObject;
    class Component;
    class Transform;
    class Scene;
    class GameEngine;
    
    // Smart pointer types
    template<typename T>
    using Ref = std::shared_ptr<T>;
    
    template<typename T, typename... Args>
    Ref<T> CreateRef(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    // Math utilities
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    inline Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
        return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
    }
    
    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    
    inline float Random(float min, float max) {
        // Use raylib's GetRandomValue for better reliability
        // Get random int in range 0-10000, then scale to float range
        if (min >= max) return min;
        int randomInt = GetRandomValue(0, 10000);
        float normalized = static_cast<float>(randomInt) / 10000.0f;
        return min + normalized * (max - min);
    }
    
    inline Vector2 Random(const Vector2& min, const Vector2& max) {
        return { Random(min.x, max.x), Random(min.y, max.y) };
    }
    
    inline float Distance(const Vector2& a, const Vector2& b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    inline Vector2 Normalize(const Vector2& v) {
        float length = std::sqrt(v.x * v.x + v.y * v.y);
        if (length > 0) {
            return { v.x / length, v.y / length };
        }
        return { 0, 0 };
    }
}

