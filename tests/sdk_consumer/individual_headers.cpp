// Consumer include paths must leave the C/C++ standard library intact on Windows.
#include <ctime>
#include <chrono>
#include <filesystem>
#include <raygame/Scene.h>
#include <raygame/Input.h>
#include <raygame/SaveSystem.h>
#include <raygame/CanvasLayout.h>
#include <raygame/TouchTracker.h>
#include <raygame/PointerGesture.h>
#include <raygame/UIButton.h>
#include <raygame/TextToSpeech.h>
#include <raygame/Effects.h>
#include <raygame/Projectile.h>
#include <raygame/Physics2D.h>

bool CheckIndividualHeaders() {
#ifdef RAYGAME_WITH_BOX2D
    RayGame::PhysicsScene2D joints("installed Box2D");
    RayGame::Body2DOptions fixed;
    fixed.motion = RayGame::Motion2D::Static;
    auto anchor = joints.dynamics.Circle({0, 0}, 10, fixed);
    auto ball = joints.dynamics.Circle({50, 0}, 10);
    auto link = joints.dynamics.DistanceJoint(anchor, ball);
    joints.IntegratePhysics(1.0f / 60);
    if (!joints.dynamics.Valid(link) || joints.dynamics.Position(ball).y <= 0) return false;
    joints.Clear();
    if (joints.dynamics.Valid(ball) || joints.dynamics.Valid(link)) return false;
#endif
    // Invalid input exercises the installed implementation without speaking.
    bool speechValid = !RayGame::TextToSpeech::Speak("") && !RayGame::TextToSpeech::GetLastError().empty();
    RayGame::TextToSpeech::Shutdown();
    RayGame::Scene scene("individual headers");
    auto object = scene.CreateGameObject("ball");
    object->AddComponent<RayGame::CircleCollider>(8.0f);
    auto effects = scene.CreateGameObject("fx")->AddComponent<RayGame::EffectLayer>();
    effects->Ring({0, 0}, {});
    effects->Advance(1.0f);
    auto shot = scene.CreateGameObject("shot")->AddComponent<RayGame::Projectile>();
    scene.Start();
    RayGame::Input::Reset();
    RayGame::InputFrame input;
    input.keys.pressed[KEY_SPACE] = true;
    RayGame::Input::SetFrame(input);
    RayGame::TouchTracker touches;
    RayGame::PointerGesture pointer;
    pointer.Update(input, RayGame::CanvasViewport::Fit(800, 600, 1600, 900));
    touches.Update(input, RayGame::CanvasViewport::Fit(800, 600, 1600, 900));
    bool valid = scene.GetPhysicsWorld().GetColliderCount() == 1 && effects->RingCount() == 0 && !shot->Finished()
                 && RayGame::Input::GetKeyDown(KEY_SPACE) && sizeof(std::time_t) > 0;
    scene.Clear();
    return valid && speechValid && scene.GetPhysicsWorld().GetColliderCount() == 0;
}
