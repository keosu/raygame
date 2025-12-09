#pragma once

#include "Core.h"
#include "Scene.h"
#include "Time.h"
#include "Input.h"
#include "ResourceManager.h"
#include "Physics.h"

namespace RayGame {

class RAYGAME_API GameEngine {
public:
    GameEngine(const std::string& title, int width, int height) 
        : windowTitle(title), screenWidth(width), screenHeight(height) {
        currentScene = nullptr;
        running = false;
        showDebug = false;
    }
    
    virtual ~GameEngine() = default;
    
    void Init() {
        InitWindow(screenWidth, screenHeight, windowTitle.c_str());
        SetTargetFPS(60);
        
        AudioManager::Init();
        ResourceManager::Init();
        
        OnInit();
    }
    
    void Run() {
        Init();
        
        if (currentScene) {
            currentScene->OnLoad();
            currentScene->Start();
        }
        
        running = true;
        
        while (!WindowShouldClose() && running) {
            Update();
            Render();
        }
        
        Shutdown();
    }
    
    void Shutdown() {
        if (currentScene) {
            currentScene->OnUnload();
        }
        
        PhysicsWorld::Clear();
        ResourceManager::Shutdown();
        AudioManager::Shutdown();
        
        CloseWindow();
    }
    
    void LoadScene(Ref<Scene> scene) {
        if (currentScene) {
            currentScene->OnUnload();
        }
        
        PhysicsWorld::Clear();
        
        currentScene = scene;
        
        if (currentScene) {
            currentScene->OnLoad();
            currentScene->Start();
        }
    }
    
    Ref<Scene> GetCurrentScene() { return currentScene; }
    
    void Quit() { running = false; }
    
    void ToggleDebug() { showDebug = !showDebug; }
    bool IsDebugEnabled() const { return showDebug; }
    
protected:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    
private:
    std::string windowTitle;
    int screenWidth, screenHeight;
    Ref<Scene> currentScene;
    bool running;
    bool showDebug;
    
    void Update() {
        GameTime::Update();
        AudioManager::UpdateMusic();
        
        // Debug toggle
        if (Input::GetKeyDown(KEY_F1)) {
            ToggleDebug();
        }
        
        if (currentScene) {
            currentScene->Update();
        }
        
        PhysicsWorld::CheckCollisions();
        
        OnUpdate();
    }
    
    void Render() {
        BeginDrawing();
        ClearBackground(BLACK);
        
        if (currentScene) {
            currentScene->Render();
        }
        
        OnRender();
        
        // Debug info
        if (showDebug) {
            DrawFPS(10, 10);
            DrawText(TextFormat("Objects: %d", 
                currentScene ? currentScene->GetAllGameObjects().size() : 0), 
                10, 30, 20, YELLOW);
            
            PhysicsWorld::DebugDraw();
        }
        
        EndDrawing();
    }
};

}

