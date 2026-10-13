#pragma once

#include "Core.h"
#include "GameObject.h"
#include "Camera2D.h"
#include "Physics.h"
#include <stdexcept>

namespace RayGame {

class RAYGAME_API Scene {
public:
    std::string name;

    Scene(const std::string& sceneName = "New Scene")
        : name(sceneName), started(false) {}

    virtual ~Scene() { Clear(); }

    virtual void OnLoad() {}
    virtual void OnUnload() {}
    virtual void OnViewportChanged(Vector2 previous, Vector2 current) {}

    ScopeToken GetCallbackScope() const { return callbacks.Token(); }
    void AttachCallbackScope(ScopeToken parent) { parentScope = parent; callbacks.SetParent(parent); }
    // Use this wrapper for loading/restarting so registrations inherit the scene.
    void Load() {
        if (clearing) throw std::logic_error("Cannot load a clearing scene");
        ScopeActivation scope(GetCallbackScope());
        OnLoad();
        Start();
    }

    bool IsClearing() const { return clearing; }
    PhysicsWorld& GetPhysicsWorld() { return physics; }
    const PhysicsWorld& GetPhysicsWorld() const { return physics; }
    unsigned long GetGeneration() const { return generation; }

    // Engine phases are separate from virtual callbacks, so every object's
    // pre-physics logic completes before any rigidbody integrates.
    void BeginFixedStep();
    void IntegratePhysics(float dt);
    virtual void PostFixedUpdate();
    virtual void LateUpdate();
    void SetInterpolationAlpha(float alpha);
    float GetInterpolationAlpha() const { return interpolationAlpha; }
    void ResetInterpolation();

    virtual void Update() {
        if (!started) Start();
        FlushPending();
        Traversal traversal(*this);
        for (size_t i = 0; i < gameObjects.size(); i++) {
            if (gameObjects[i] && gameObjects[i]->active && !gameObjects[i]->IsDestroyed()) {
                gameObjects[i]->Update();
            }
        }
        traversal.End();

        FlushPending();
    }

    virtual void FixedUpdate() {
        if (!started) Start();
        FlushPending();
        Traversal traversal(*this);
        for (size_t i = 0; i < gameObjects.size(); i++) {
            if (gameObjects[i] && gameObjects[i]->active && !gameObjects[i]->IsDestroyed()) {
                gameObjects[i]->FixedUpdate();
            }
        }
        traversal.End();
        FlushPending();
    }

    virtual void Render() {
        if (!started) Start();
        Traversal traversal(*this);
        auto camera = activeCamera.lock();
        bool cam = camera && camera->GetGameObject() && camera->GetGameObject()->active
                   && camera->IsEnabled();
        if (cam) camera->Begin();
        struct CameraScope {
            Camera2DComponent* value;
            ~CameraScope() { if (value) value->End(); }
        } cameraScope{cam ? camera.get() : nullptr};

        // Build a flat, layer-sorted draw list across all objects. Keys are read once so
        // the comparator stays consistent even if a component computes them.
        drawList.clear();
        for (auto& obj : gameObjects) {
            if (!obj || !obj->active || obj->IsDestroyed()) continue;
            obj->Start();
            for (auto& comp : obj->GetAllComponents()) {
                if (comp && comp->IsEnabled() && comp->HasStarted())
                    drawList.push_back({comp->RenderLayer(), comp->renderOrder, comp->RenderDepth(), comp});
            }
        }
        std::stable_sort(drawList.begin(), drawList.end(), [](const DrawItem& a, const DrawItem& b) {
            if (a.layer != b.layer) return a.layer < b.layer;
            if (a.order != b.order) return a.order < b.order;
            return a.depth < b.depth;
        });
        for (const auto& item : drawList) {
            const auto& comp = item.component;
            auto* obj = comp->GetGameObject();
            if (obj && obj->active && !obj->IsDestroyed() && comp->IsEnabled()) {
                ScopeActivation scope(comp->GetCallbackScope());
                comp->Render();
            }
        }
        drawList.clear();

        if (cam) { camera->End(); cameraScope.value = nullptr; }
        traversal.End();
        FlushPending();
    }

    Ref<GameObject> CreateGameObject(const std::string& name = "GameObject") {
        if (clearing) throw std::logic_error("Cannot create objects in a scene being cleared");
        auto obj = CreateRef<GameObject>(name);
        obj->scene = this;
        obj->callbacks.SetParent(GetCallbackScope());
        if (updating) {
            pendingAdd.push_back(obj);
        } else {
            gameObjects.push_back(obj);
        }
        return obj;
    }

    // Deferred destruction — safe to call during Update.
    void Destroy(Ref<GameObject> obj) {
        Destroy(obj.get());
    }
    void Destroy(GameObject* obj) {
        if (obj && obj->scene == this) obj->MarkDestroyed();
    }

    // Backward-compatible alias.
    void DestroyGameObject(Ref<GameObject> obj) { Destroy(obj); }

    // Also finalizes externally retained objects before releasing scene resources.
    void Clear() {
        if (clearing) { if (!updating) FlushPending(); return; }
        clearing = true;
        ++generation;
        physics.Clear();
        callbacks.Cancel();
        for (auto& obj : gameObjects) if (obj) obj->MarkDestroyed();
        for (auto& obj : pendingAdd) if (obj) obj->MarkDestroyed();
        activeCamera.reset();
        started = false;
        if (!updating) FlushPending();
    }

    template<typename T>
    std::vector<Ref<GameObject>> FindGameObjectsWithComponent() {
        std::vector<Ref<GameObject>> result;
        for (auto& obj : gameObjects) {
            if (obj && !obj->IsDestroyed() && obj->GetComponent<T>()) result.push_back(obj);
        }
        return result;
    }

    std::vector<Ref<GameObject>> FindGameObjectsByTag(const std::string& tag) {
        std::vector<Ref<GameObject>> result;
        for (auto& obj : gameObjects) {
            if (obj && obj->tag == tag && !obj->IsDestroyed()) result.push_back(obj);
        }
        return result;
    }

    Ref<GameObject> FindGameObjectByName(const std::string& name) {
        for (auto& obj : gameObjects) {
            if (obj && obj->name == name && !obj->IsDestroyed()) return obj;
        }
        return nullptr;
    }

    void Start() {
        if (!started) {
            started = true;
            Traversal traversal(*this);
            // Index-based: components may spawn new objects during Start().
            for (size_t i = 0; i < gameObjects.size(); i++) {
                if (gameObjects[i]) gameObjects[i]->Start();
            }
            traversal.End();
            FlushPending();
        }
    }

    void SetActiveCamera(Camera2DComponent* cam) {
        activeCamera.reset();
        if (!cam || !cam->GetGameObject()) return;
        for (const auto& comp : cam->GetGameObject()->GetAllComponents()) {
            if (comp.get() == cam) activeCamera = std::static_pointer_cast<Camera2DComponent>(comp);
        }
    }
    Camera2DComponent* GetActiveCamera() const {
        auto cam = activeCamera.lock();
        return cam && cam->GetGameObject() && !cam->GetGameObject()->IsDestroyed() ? cam.get() : nullptr;
    }

    const std::vector<Ref<GameObject>>& GetAllGameObjects() const {
        return gameObjects;
    }

protected:
    std::vector<Ref<GameObject>> gameObjects;
    bool started;

    void FlushPending() {
        if (updating) return;
        Traversal traversal(*this);
        if (clearing) { Drain(); return; }
        std::vector<Ref<GameObject>> additions;
        additions.swap(pendingAdd);
        gameObjects.insert(gameObjects.end(), additions.begin(), additions.end());
        for (const auto& obj : additions) {
            if (started && obj) obj->Start();
        }
        // Separate removals before callbacks, which may create more objects.
        std::vector<Ref<GameObject>> removed;
        for (const auto& obj : gameObjects) {
            if (obj && obj->IsDestroyed()) removed.push_back(obj);
        }
        gameObjects.erase(std::remove_if(gameObjects.begin(), gameObjects.end(),
            [](const Ref<GameObject>& obj) { return !obj || obj->IsDestroyed(); }), gameObjects.end());
        for (const auto& obj : removed) obj->DestroyNow();
        if (clearing) Drain();
    }

private:
    PhysicsWorld physics{this};
    unsigned long generation = 0;
    float interpolationAlpha = 1;
    struct Traversal {
        Scene& scene;
        explicit Traversal(Scene& value) : scene(value) {
            if (scene.updating) throw std::logic_error("Scene traversal cannot be re-entered");
            scene.updating = true;
        }
        void End() { scene.updating = false; }
        ~Traversal() { End(); }
    };

    void Drain() {
        // clearing remains true throughout user callbacks: no resurrection.
        auto objects = std::move(gameObjects);
        auto pending = std::move(pendingAdd);
        gameObjects.clear(); pendingAdd.clear(); drawList.clear();
        for (const auto& obj : objects) if (obj) obj->DestroyNow();
        for (const auto& obj : pending) if (obj) obj->DestroyNow();
        clearing = false;
        callbacks.Renew(parentScope);
    }

    CallbackScope callbacks;
    ScopeToken parentScope;
    std::vector<Ref<GameObject>> pendingAdd;
    struct DrawItem {
        int layer, order;
        float depth;
        Ref<Component> component;
    };
    std::vector<DrawItem> drawList;
    std::weak_ptr<Camera2DComponent> activeCamera;
    bool updating = false;
    bool clearing = false;
};

}
