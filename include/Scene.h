#pragma once

#include "Core.h"
#include "GameObject.h"
#include "Camera2D.h"

namespace RayGame {

class RAYGAME_API Scene {
public:
    std::string name;

    Scene(const std::string& sceneName = "New Scene")
        : name(sceneName), started(false) {}

    virtual ~Scene() { Clear(); }

    virtual void OnLoad() {}
    virtual void OnUnload() {}

    virtual void Update() {
        if (!started) Start();
        FlushPending();
        updating = true;
        for (size_t i = 0; i < gameObjects.size(); i++) {
            if (gameObjects[i] && gameObjects[i]->active && !gameObjects[i]->IsDestroyed()) {
                gameObjects[i]->Update();
            }
        }
        updating = false;

        FlushPending();
    }

    virtual void FixedUpdate() {
        if (!started) Start();
        FlushPending();
        updating = true;
        for (size_t i = 0; i < gameObjects.size(); i++) {
            if (gameObjects[i] && gameObjects[i]->active && !gameObjects[i]->IsDestroyed()) {
                gameObjects[i]->FixedUpdate();
            }
        }
        updating = false;
        FlushPending();
    }

    virtual void Render() {
        if (!started) Start();
        updating = true;
        auto camera = activeCamera.lock();
        bool cam = camera && camera->GetGameObject() && camera->GetGameObject()->active
                   && camera->IsEnabled();
        if (cam) camera->Begin();

        // Build a flat, layer-sorted draw list across all objects.
        drawList.clear();
        for (auto& obj : gameObjects) {
            if (!obj || !obj->active || obj->IsDestroyed()) continue;
            obj->Start();
            for (auto& comp : obj->GetAllComponents()) {
                if (comp && comp->IsEnabled() && comp->HasStarted()) drawList.push_back(comp);
            }
        }
        std::stable_sort(drawList.begin(), drawList.end(),
            [](const Ref<Component>& a, const Ref<Component>& b) {
                if (a->RenderLayer() != b->RenderLayer()) return a->RenderLayer() < b->RenderLayer();
                return a->renderOrder < b->renderOrder;
            });
        for (const auto& comp : drawList) {
            auto* obj = comp->GetGameObject();
            if (obj && obj->active && !obj->IsDestroyed() && comp->IsEnabled()) comp->Render();
        }
        drawList.clear();

        if (cam) camera->End();
        updating = false;
        FlushPending();
    }

    Ref<GameObject> CreateGameObject(const std::string& name = "GameObject") {
        auto obj = CreateRef<GameObject>(name);
        obj->scene = this;
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
            updating = true;
            // Index-based: components may spawn new objects during Start().
            for (size_t i = 0; i < gameObjects.size(); i++) {
                if (gameObjects[i]) gameObjects[i]->Start();
            }
            updating = false;
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
        updating = true;
        std::vector<Ref<GameObject>> additions;
        additions.swap(pendingAdd);
        for (const auto& obj : additions) {
            gameObjects.push_back(obj);
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
        updating = false;
    }

private:
    std::vector<Ref<GameObject>> pendingAdd;
    std::vector<Ref<Component>> drawList;
    std::weak_ptr<Camera2DComponent> activeCamera;
    bool updating = false;
};

}
