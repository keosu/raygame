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
    
    virtual ~Scene() = default;
    
    virtual void OnLoad() {}
    virtual void OnUnload() {}
    virtual void Update() {
        // Remove inactive objects
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                [](const Ref<GameObject>& obj) { return !obj->active; }),
            gameObjects.end()
        );
        
        for (auto& obj : gameObjects) {
            obj->Update();
        }
    }
    
    virtual void Render() {
        for (auto& obj : gameObjects) {
            obj->Render();
        }
    }
    
    Ref<GameObject> CreateGameObject(const std::string& name = "GameObject") {
        auto obj = CreateRef<GameObject>(name);
        gameObjects.push_back(obj);
        return obj;
    }
    
    void DestroyGameObject(Ref<GameObject> obj) {
        auto it = std::find(gameObjects.begin(), gameObjects.end(), obj);
        if (it != gameObjects.end()) {
            gameObjects.erase(it);
        }
    }
    
    template<typename T>
    std::vector<Ref<GameObject>> FindGameObjectsWithComponent() {
        std::vector<Ref<GameObject>> result;
        for (auto& obj : gameObjects) {
            if (obj->GetComponent<T>()) {
                result.push_back(obj);
            }
        }
        return result;
    }
    
    std::vector<Ref<GameObject>> FindGameObjectsByTag(const std::string& tag) {
        std::vector<Ref<GameObject>> result;
        for (auto& obj : gameObjects) {
            if (obj->tag == tag) {
                result.push_back(obj);
            }
        }
        return result;
    }
    
    Ref<GameObject> FindGameObjectByName(const std::string& name) {
        for (auto& obj : gameObjects) {
            if (obj->name == name) {
                return obj;
            }
        }
        return nullptr;
    }
    
    void Start() {
        if (!started) {
            for (auto& obj : gameObjects) {
                obj->Start();
            }
            started = true;
        }
    }
    
    const std::vector<Ref<GameObject>>& GetAllGameObjects() const {
        return gameObjects;
    }
    
protected:
    std::vector<Ref<GameObject>> gameObjects;
    bool started;
};

}

