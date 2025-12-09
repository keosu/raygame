#pragma once

#include "Core.h"
#include "Component.h"

namespace RayGame {

class RAYGAME_API GameObject {
public:
    std::string name;
    std::string tag;
    bool active = true;
    
    GameObject(const std::string& name = "GameObject") 
        : name(name), tag("Default") {
        transform = AddComponent<Transform>();
    }
    
    virtual ~GameObject() {
        for (auto& comp : components) {
            comp->OnDestroy();
        }
    }
    
    template<typename T, typename... Args>
    Ref<T> AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, 
                     "T must derive from Component");
        
        auto component = CreateRef<T>(std::forward<Args>(args)...);
        component->gameObject = this;
        components.push_back(component);
        return component;
    }
    
    template<typename T>
    Ref<T> GetComponent() {
        for (auto& comp : components) {
            auto casted = std::dynamic_pointer_cast<T>(comp);
            if (casted) return casted;
        }
        return nullptr;
    }
    
    template<typename T>
    std::vector<Ref<T>> GetComponents() {
        std::vector<Ref<T>> result;
        for (auto& comp : components) {
            auto casted = std::dynamic_pointer_cast<T>(comp);
            if (casted) result.push_back(casted);
        }
        return result;
    }
    
    void Start() {
        for (auto& comp : components) {
            if (comp->IsEnabled()) {
                comp->Start();
            }
        }
    }
    
    void Update() {
        if (!active) return;
        
        // Use index-based loop to avoid iterator invalidation
        for (size_t i = 0; i < components.size(); i++) {
            if (components[i] && components[i]->IsEnabled()) {
                components[i]->Update();
            }
        }
    }
    
    void Render() {
        if (!active) return;
        
        // Use index-based loop to avoid iterator invalidation
        for (size_t i = 0; i < components.size(); i++) {
            if (components[i] && components[i]->IsEnabled()) {
                components[i]->Render();
            }
        }
    }
    
    Ref<Transform> transform;
    
private:
    std::vector<Ref<Component>> components;
};

}

