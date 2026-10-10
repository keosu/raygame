// Consumer include paths must leave the C/C++ standard library intact on Windows.
#include <ctime>
#include <chrono>
#include <filesystem>
#include <raygame/Scene.h>
#include <raygame/Input.h>
#include <raygame/SaveSystem.h>
#include <raygame/CanvasLayout.h>
#include <raygame/TouchTracker.h>

bool CheckIndividualHeaders() {
    RayGame::Scene scene("individual headers");
    auto object = scene.CreateGameObject("ball");
    object->AddComponent<RayGame::CircleCollider>(8.0f);
    scene.Start();
    RayGame::Input::Reset();
    RayGame::InputFrame input;
    input.keys.pressed[KEY_SPACE] = true;
    RayGame::Input::SetFrame(input);
    RayGame::TouchTracker touches;
    touches.Update(input, RayGame::CanvasViewport::Fit(800, 600, 1600, 900));
    bool valid = scene.GetPhysicsWorld().GetColliderCount() == 1
                 && RayGame::Input::GetKeyDown(KEY_SPACE) && sizeof(std::time_t) > 0;
    scene.Clear();
    return valid && scene.GetPhysicsWorld().GetColliderCount() == 0;
}
