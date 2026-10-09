#pragma once

#include "Core.h"
#include "Component.h"
#include "Diagnostics.h"

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
        DestroyNow();
    }

    template<typename T, typename... Args>
    Ref<T> AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value,
                     "T must derive from Component");

        if (destroyed) return nullptr;
        auto component = CreateRef<T>(std::forward<Args>(args)...);
        component->gameObject = this;
        component->callbacks.SetParent(callbacks.Token());
        components.push_back(component);
        ScopeActivation scope(component->GetCallbackScope());
        component->Awake();
        if (component->IsEnabled() && !destroyed) component->OnEnable();
        // Start is deferred so callers can configure the component first.
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

    const std::vector<Ref<Component>>& GetAllComponents() const { return components; }

    void Start() {
        if (!active || destroyed) return;
        const auto snapshot = components;
        for (const auto& comp : snapshot) {
            if (!active || destroyed) break;
            if (comp->IsEnabled() && !comp->HasStarted()) {
                comp->started = true;
                ScopeActivation scope(comp->GetCallbackScope());
                comp->Start();
            }
        }
    }

    void Update() {
        Visit(&Component::Update);
    }

    void FixedUpdate() {
        Visit(&Component::FixedUpdate);
    }

    void PostFixedUpdate() { Visit(&Component::PostFixedUpdate); }
    void LateUpdate() { Visit(&Component::LateUpdate); }

    void Render() {
        Visit(&Component::Render);
    }

    bool IsDestroyed() const { return destroyed; }
    void MarkDestroyed() { destroyed = true; active = false; callbacks.Cancel(); }
    ScopeToken GetCallbackScope() const { return callbacks.Token(); }

    Ref<Transform> transform;
    Scene* scene = nullptr;

private:
    friend class Scene;
    void IntegratePhysics(float dt);

    void Visit(void (Component::*callback)()) {
        Start();
        const auto snapshot = components;
        for (const auto& comp : snapshot) {
            if (!active || destroyed) break;
            if (comp->IsEnabled() && comp->HasStarted()) {
                ScopeActivation scope(comp->GetCallbackScope());
                (comp.get()->*callback)();
            }
        }
    }

    void DestroyNow() noexcept {
        if (finalized) return;
        finalized = true;
        MarkDestroyed();
        for (const auto& comp : components) {
            ScopeActivation scope(comp->GetCallbackScope());
            try { if (comp->IsEnabled()) comp->SetEnabled(false); }
            catch (...) { Diagnostics::Write(LogLevel::Error, "Component OnDisable failed during destruction"); }
            try { comp->OnDestroy(); }
            catch (...) { Diagnostics::Write(LogLevel::Error, "Component OnDestroy failed during destruction"); }
        }
        if (transform) transform->DetachHierarchy();
        for (const auto& comp : components) comp->gameObject = nullptr;
        scene = nullptr;
    }

    CallbackScope callbacks;
    std::vector<Ref<Component>> components;
    bool destroyed = false;
    bool finalized = false;
};

}
