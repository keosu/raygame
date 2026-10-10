#include "Game.h"
#include <fstream>

namespace Forge {
using namespace RayGame;
namespace {
Vector2 MenuOffset() {
    CanvasLayout layout{{1280, 720}, {0, 0, Display::GetWidth(), Display::GetHeight()}};
    return layout.Offset({0.5f, 0.5f});
}
void BeginMenuLayout() {
    auto offset = MenuOffset();
    rlPushMatrix();
    rlTranslatef(offset.x, offset.y, 0);
}
void DrawMenuShade(Color color) {
    auto offset = MenuOffset();
    DrawRectangleRec({-offset.x, -offset.y, Display::GetWidth(), Display::GetHeight()}, color);
}
Command ReadCommand() {
    Command c;
    c.move = (Input::GetKey(KEY_D) || Input::GetKey(KEY_RIGHT) ? 1.0f : 0.0f) -
             (Input::GetKey(KEY_A) || Input::GetKey(KEY_LEFT) ? 1.0f : 0.0f);
    float stick = Input::GetGamepadAxis(GAMEPAD_AXIS_LEFT_X);
    if (std::fabs(stick) > 0.2f)
        c.move = stick;
    c.jump = Input::GetKeyDown(KEY_SPACE) || Input::GetKeyDown(KEY_W) ||
             Input::GetGamepadButtonDown(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    c.jumpHeld = Input::GetKey(KEY_SPACE) || Input::GetKey(KEY_W) ||
                 Input::GetGamepadButton(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    c.dash = Input::GetKeyDown(KEY_LEFT_SHIFT) || Input::GetKeyDown(KEY_X) ||
             Input::GetGamepadButtonDown(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    c.down = Input::GetKey(KEY_S) || Input::GetKey(KEY_DOWN) ||
             Input::GetGamepadAxis(GAMEPAD_AXIS_LEFT_Y) > 0.6f;
    return c;
}
class Pilot : public Component {
public:
    std::shared_ptr<World> world;
    bool* playing;
    TouchControls* touch;
    Pilot(std::shared_ptr<World> value, bool* enabled, TouchControls* controls)
        : world(std::move(value)), playing(enabled), touch(controls) {}
    void Start() override {
        gameObject->transform->SetInterpolationEnabled(true);
    }
    void FixedUpdate() override {
        if (!*playing)
            return;
        auto oldTimer = world->deathTimer;
        auto command = ReadCommand(), mobile = touch->Consume();
        if (mobile.move != 0)
            command.move = mobile.move;
        command.jump |= mobile.jump;
        command.jumpHeld |= mobile.jumpHeld;
        command.dash |= mobile.dash;
        command.down |= mobile.down;
        world->Step(command, GameTime::DeltaTime());
        if (oldTimer > 0 && world->deathTimer == 0)
            gameObject->transform->Teleport(world->player.position);
        else
            gameObject->transform->position = world->player.position;
    }
};
const char* LevelFiles[] = {"01_foundry.level", "02_conduit.level", "03_core.level"};
bool Press(int key, int pad) {
    return Input::GetKeyDown(key) || Input::GetGamepadButtonDown(pad);
}
} // namespace
Stage::Stage(std::shared_ptr<World> value, bool* enabled, TouchControls* controls)
    : Scene("FORGE stage"), world(std::move(value)), playing(enabled), touch(controls) {}
void Stage::OnLoad() {
    actor = CreateGameObject("courier");
    actor->transform->position = world->player.position;
    actor->AddComponent<Pilot>(world, playing, touch);
}
Game::Game(std::string assetDirectory, std::filesystem::path data, bool test)
    : GameEngine("FORGE / The last delivery", 1280, 720), assets(std::move(assetDirectory)),
      profile(std::move(data)), smoke(test) {
    SetDefaultShortcutsEnabled(false);
    SetCanvasScaleMode(CanvasScaleMode::Expand);
    targetFPS = 120;
}
Game::~Game() {
    if (!savedExit && !smoke)
        Persist();
    Shutdown();
}
void Game::OnInit() {
    profile.Load();
    if (smoke)
        profile =
            Profile(profile.directory); // Repeated smoke runs have a stable isolated baseline.
    art.Init(assets);
    ApplySettings();
#if !defined(__ANDROID__)
    Image icon = LoadImage((assets + "/icon.png").c_str());
    if (IsImageValid(icon))
        SetWindowIcon(icon);
    UnloadImage(icon);
#endif
    if (IsAudioDeviceReady()) {
        for (const char* name :
             {"jump", "dash", "land", "cell", "checkpoint", "death", "finish", "ui"})
            AudioManager::LoadSound(name, assets + "/audio/" + name + ".wav");
        AudioManager::LoadMusic("forge", assets + "/audio/forge.wav");
        AudioManager::PlayMusic("forge", profile.settings.music, true);
    }
    if (smoke)
        targetFPS = 0;
    world = std::make_shared<World>(Level::Load(assets + "/levels/01_foundry.level", 0));
    art.camera.target = {1100, 550};
    if (!profile.active)
        focus = 1;
    if (profile.recovered) {
        toast = "YOUR CHECKPOINT WAS RECOVERED";
        toastTime = 5;
    }
}
void Game::ApplySettings() {
    art.settings = profile.settings;
    art.touchControls = touchEnabled;
#if !defined(__ANDROID__)
    SetWindowTitle(profile.settings.language == Language::Chinese ? "FORGE / 最后的信使"
                                                                  : "FORGE / The last delivery");
#endif
    targetFPS = smoke ? 0 : profile.settings.fps;
    SetTargetFPS(targetFPS);
    if (!smoke && !touchEnabled)
        SetFullscreen(profile.settings.fullscreen);
    if (IsAudioDeviceReady())
        AudioManager::SetMusicVolume("forge", profile.settings.music);
}
void Game::Switch(Page next) {
    page = next;
    playing = next == Page::Play;
    focus = 0;
    pageTime = 0;
    activate = false;
    mousePress = false;
    settingsDirection = 0;
    Input::DiscardPending();
    touch.Reset(Input::GetFrame());
    if (stage)
        stage->ResetInterpolation();
}
void Game::StartLevel(int index, bool resume) {
    if (index < 0 || index >= 3)
        throw std::runtime_error("Invalid campaign sector");
    world = std::make_shared<World>(Level::Load(assets + "/levels/" + LevelFiles[index], index));
    if (resume) {
        world->Restore(profile.checkpoint, profile.collected);
        world->deaths = profile.deaths;
        world->elapsed = profile.elapsed;
    }
    stage = CreateRef<Stage>(world, &playing, &touch);
    LoadScene(stage);
    art.particles.clear();
    art.shake = 0;
    art.Follow(*world, world->player.position, 0, true);
    Switch(Page::Play);
    Persist();
}
void Game::Persist() {
    bool ok = true;
    if (world && stage)
        ok = profile.Record(*world);
    ok = profile.SaveSettings() && ok;
    if (!ok) {
        toast = "SAVE FAILED / CHECK YOUR DATA FOLDER";
        toastTime = 6;
    }
}
void Game::OnUnscaledUpdate() {
    ++frame;
    if (smoke)
        SmokeInput();
    if (touchEnabled)
        touch.Update(Input::GetFrame(), Display::GetViewport(), page == Page::Play);
    float dt = std::min(GameTime::UnscaledDeltaTime(), 0.05f);
    art.Update(dt);
    pageTime += dt;
    toastTime = std::max(0.0f, toastTime - dt);
    activate = Input::GetFrame().focused && Press(KEY_ENTER, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    settingsDirection = 0;
    if (!Input::GetFrame().focused)
        mousePress = false;
    if (!touchEnabled && Press(KEY_F11, GAMEPAD_BUTTON_UNKNOWN)) {
        ToggleFullscreen();
        profile.settings.fullscreen = IsFullscreen();
        Persist();
    }
    bool back = touchEnabled && Input::GetKeyDown(KEY_BACK);
    if (Press(KEY_ESCAPE, GAMEPAD_BUTTON_MIDDLE_RIGHT) || Input::GetKeyDown(KEY_P) || back ||
        touch.pausePressed) {
        if (page == Page::Play) {
            Persist();
            Switch(Page::Pause);
        } else if (page == Page::Pause)
            Switch(Page::Play);
        else if (page == Page::Settings) {
            ApplySettings();
            Persist();
            Switch(settingsReturn);
        } else if (page != Page::Home && page != Page::Debrief)
            Switch(Page::Home);
        else if (back && page == Page::Debrief) {
            Persist();
            Switch(Page::Home);
        } else if (back && page == Page::Home) {
            Persist();
            savedExit = true;
            Quit();
        }
    }
    if (page == Page::Play) {
        if (Input::GetKeyDown(KEY_R)) {
            world->Kill();
        }
        if (!Input::GetFrame().focused) {
            Persist();
            Switch(Page::Pause);
        }
    } else {
        int count = page == Page::Home         ? (touchEnabled ? 5 : 4)
                    : page == Page::Pause      ? 4
                    : page == Page::Settings   ? 7
                    : page == Page::Debrief    ? 2
                    : page == Page::ConfirmNew ? 2
                                               : 1;
        if (Press(KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || Input::GetKeyDown(KEY_S))
            focus = (focus + 1) % count;
        if (Press(KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP) || Input::GetKeyDown(KEY_W))
            focus = (focus + count - 1) % count;
        if (Press(KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
            settingsDirection = -1;
        if (Press(KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
            settingsDirection = 1;
    }
    if (smoke)
        SmokeStep();
}
void Game::Audio(unsigned event) {
    if (!IsAudioDeviceReady() || smoke)
        return;
    struct Cue {
        unsigned flag;
        const char* name;
    };
    for (auto c : {Cue{Jump, "jump"}, Cue{Dash, "dash"}, Cue{Land, "land"}, Cue{Cell, "cell"},
                   Cue{Checkpoint, "checkpoint"}, Cue{Death, "death"}, Cue{Finish, "finish"}})
        if (event & c.flag)
            AudioManager::PlaySound(c.name, profile.settings.sfx);
}
void Game::OnUpdate() {
    if (!world)
        return;
    unsigned events = world->TakeEvents();
    art.React(events, world->player.position);
    Audio(events);
    if (events & Checkpoint) {
        toast = "CHECKPOINT SYNCHRONIZED";
        toastTime = 2.5f;
        Persist();
    }
    if (events & (Cell | Death | Finish))
        Persist();
    if (events & Finish)
        Switch(Page::Debrief);
    if (stage && stage->actor &&
        (page == Page::Play || page == Page::Pause || page == Page::Debrief ||
         page == Page::Settings))
        art.Follow(*world, stage->actor->transform->GetRenderPosition(),
                   GameTime::UnscaledDeltaTime(), world->deathTimer > 0.6f);
}
bool Game::Button(int index, Rectangle r, const std::string& label, const std::string& detail,
                  bool disabled) {
    auto mouse = touchEnabled ? touch.pointerPosition : Input::GetMousePosition();
    auto offset = MenuOffset();
    mouse = Vector2Subtract(mouse, offset);
    bool in =
        (touchEnabled ? (touch.pointerHeld || touch.pointerReleased) : Input::IsMouseInCanvas()) &&
        CheckCollisionPointRec(mouse, r) && Input::GetFrame().focused;
    bool moved = !touchEnabled && Vector2Distance(mouse, lastMouse) > 0.3f;
    if (in && moved)
        focus = index;
    if (touchEnabled && touch.pointerPressed && in)
        focus = index;
    if (!touchEnabled && Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) && in) {
        pressedAt = mouse;
        mousePress = true;
        focus = index;
    }
    bool selected = focus == index && !disabled;
    DrawRectangleRec(r, selected ? Color{38, 64, 61, 245} : Color{16, 29, 35, 225});
    DrawRectangleLinesEx(r, 1, selected ? Fade(Mint, 0.65f) : Color{48, 65, 68, 180});
    if (selected) {
        DrawRectangleRec({r.x, r.y, 3, r.height}, Mint);
        art.Text(">", {r.x + 15, r.y + 12}, 24, Mint);
    }
    float labelSize = 24,
          available = r.width - 60 - (detail.empty() ? 0 : art.Measure(detail, 14) + 20);
    while (labelSize > 16 && art.Measure(label, labelSize, true) > available)
        labelSize -= 1;
    art.Text(label, {r.x + 40, r.y + (r.height - labelSize) / 2 - 1}, labelSize,
             disabled   ? Fade(Muted, 0.5f)
             : selected ? Mint
                        : Light,
             true);
    if (!detail.empty())
        art.Text(detail, {r.x + r.width - art.Measure(detail, 14) - 20, r.y + 18}, 14, Muted);
    bool clicked = touchEnabled ? touch.Click({r.x + offset.x, r.y + offset.y, r.width, r.height})
                                : (mousePress && in && Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT) &&
                                   CheckCollisionPointRec(pressedAt, r));
    if ((selected && activate && Input::GetFrame().focused) || clicked) {
        activate = false;
        mousePress = false;
        pointerDirection = clicked && mouse.x < r.x + r.width / 2 ? -1 : 1;
        if (!disabled) {
            AudioManager::PlaySound("ui", profile.settings.sfx * 0.6f);
            return true;
        }
    }
    return false;
}
void Game::DrawHome() {
    auto offset = MenuOffset();
    DrawRectangleGradientH((int)-offset.x, (int)-offset.y, (int)(1000 + offset.x),
                           (int)Display::GetHeight(), Fade(Ink, 0.97f), Fade(Ink, 0));
    art.Glow({980, 370}, 240, Mint, 0.16f);
    Runner pose;
    pose.grounded = true;
    pose.facing = -1;
    art.DrawRunner({990, 480}, pose, 3.3f);
    DrawRectangleRec({845, 482, 295, 8}, Ember);
    DrawRectangleRec({850, 490, 285, 40}, Steel);
    art.Text("A RAYGAME ORIGINAL", {90, 74}, 13, Muted);
    art.Text("FORGE", {81, 109}, 142, Light, true);
    DrawRectangleRec({91, 258, 51, 3}, Mint);
    art.Text("THE LAST DELIVERY", {158, 247}, 19, Mint);
    art.Text("A sleeping factory. One courier. One spark.", {91, 290}, 19, Muted);
    if (Button(0, {90, 365, 405, 52}, "CONTINUE",
               profile.active ? art.Localize("SECTOR") + " 0" + std::to_string(profile.level + 1)
                              : "NO SAVED RUN",
               !profile.active))
        StartLevel(profile.level, true);
    if (Button(1, {90, 429, 405, 52}, "NEW DELIVERY", "3 SECTORS")) {
        if (profile.active)
            Switch(Page::ConfirmNew);
        else {
            if (!profile.NewRun()) {
                toast = "SAVE FAILED / PROGRESS WILL STAY IN THIS SESSION";
                toastTime = 6;
            }
            StartLevel(0, false);
        }
    }
    if (Button(2, {90, 493, 195, 52}, "SETTINGS")) {
        settingsReturn = Page::Home;
        Switch(Page::Settings);
    }
    if (Button(3, {300, 493, 195, 52}, "EXIT")) {
        Persist();
        savedExit = true;
        Quit();
    }
    art.Text(touchEnabled ? "LEFT: MOVE / DROP    RIGHT: JUMP / DASH"
                          : "MOVE  A/D     JUMP  SPACE     DASH  SHIFT",
             {91, 592}, 14, Muted);
    art.Text(touchEnabled ? "HOLD JUMP FOR HEIGHT / TAP AGAIN TO DOUBLE JUMP"
                          : "KEYBOARD + CONTROLLER",
             {91, 619}, 12, Fade(Muted, 0.6f));
    if (touchEnabled)
        art.Text("HOLD DOWN + JUMP TO DROP THROUGH", {91, 646}, 12, Fade(Muted, 0.6f));
    art.Text("01 / THE FOUNDRY", {935, 555}, 14, Ember);
    art.Text("CARRY THE LIGHT", {902, 579}, 28, Light, true);
    if (Input::GetKeyDown(KEY_C))
        Switch(Page::Credits);
    if (touchEnabled) {
        if (Button(4, {1000, 640, 225, 52}, "CREDITS"))
            Switch(Page::Credits);
    } else
        art.Text("C  CREDITS", {1100, 672}, 12, Muted);
    art.Text("FORGE  /  1.0", {90, 672}, 12, Muted);
}
void Game::DrawHUD() {
    float width = Display::GetWidth(), height = Display::GetHeight();
    DrawRectangleGradientV(0, 0, (int)width, 115, Fade(Ink, 0.95f), Fade(Ink, 0));
    art.Text("FORGE", {30, 20}, 30, Light, true);
    DrawRectangleRec({141, 25, 1, 24}, Muted);
    art.Text("0" + std::to_string(world->level.index + 1) + "  /  " +
                 art.Localize(world->level.name),
             {160, 29}, 14, Muted);
    rlPushMatrix();
    rlTranslatef(width - 1280, 0, 0);
    art.Text("POWER CELLS", {858, 18}, 11, Muted);
    for (size_t i = 0; i < world->level.cells.size(); ++i) {
        bool found = world->collected & (std::uint64_t{1} << i);
        DrawRectangleRec({858 + i * 16.0f, 39, 10, 10}, found ? Mint : Color{49, 68, 67, 255});
    }
    char timer[32];
    std::snprintf(timer, sizeof(timer), "%02d:%02d", (int)world->elapsed / 60,
                  (int)world->elapsed % 60);
    art.Text(timer, {1130, 20}, 29, Light, true);
    if (!touchEnabled)
        art.Text("ESC  PAUSE", {1140, 59}, 11, Muted);
    rlPopMatrix();
    rlPushMatrix();
    rlTranslatef(0, height - 720, 0);
    DrawRectangleRec({30, 675, 80, 3}, Color{47, 65, 63, 255});
    DrawRectangleRec({30, 675, 80 * (1 - Clamp(world->player.dashCooldown / 0.65f, 0.0f, 1.0f)), 3},
                     Mint);
    art.Text("DASH", {30, 651}, 11, world->player.dashCooldown == 0 ? Mint : Muted);
    rlPopMatrix();
    if (pageTime < 4 && page == Page::Play) {
        float a = Clamp(4 - pageTime, 0.0f, 1.0f);
        art.Text(world->level.subtitle,
                 {width / 2 - art.Measure(world->level.subtitle, 18) / 2, 90}, 18, Fade(Light, a));
    }
    if (world->deathTimer > 0) {
        art.Text("SIGNAL LOST", {width / 2 - art.Measure("SIGNAL LOST", 48, true) / 2, 270}, 48,
                 Red, true);
        art.Text("RETURNING TO CHECKPOINT",
                 {width / 2 - art.Measure("RETURNING TO CHECKPOINT", 14) / 2, 327}, 14, Muted);
    }
    if (world->player.position.x > world->level.goal.x - 160 &&
        world->CellCount() != (int)world->level.cells.size())
        art.Text("RETRIEVE THE REMAINING CELLS",
                 {width / 2 - art.Measure("RETRIEVE THE REMAINING CELLS", 20) / 2, 615}, 20, Ember);
}
void Game::DrawTouchControls() {
    auto panel = [&](Rectangle r, bool down) {
        DrawRectangleRounded(r, 0.25f, 8, down ? Color{46, 89, 76, 205} : Color{15, 30, 37, 170});
        DrawRectangleRoundedLinesEx(r, 0.25f, 8, 2, down ? Mint : Fade(Muted, 0.65f));
    };
    const auto& held = touch.Held();
    auto left = touch.Bounds(TouchControls::Left), right = touch.Bounds(TouchControls::Right);
    auto down = touch.Bounds(TouchControls::Down), jump = touch.Bounds(TouchControls::JumpButton);
    auto dash = touch.Bounds(TouchControls::DashButton),
         pause = touch.Bounds(TouchControls::PauseButton);
    panel(left, held.move < 0);
    panel(right, held.move > 0);
    panel(down, held.down);
    panel(jump, held.jumpHeld);
    panel(dash, touch.dashHeld);
    panel(pause, false);
    DrawTriangle({left.x + 34, left.y + 48}, {left.x + 62, left.y + 72}, {left.x + 62, left.y + 24},
                 Light);
    DrawTriangle({right.x + 62, right.y + 48}, {right.x + 34, right.y + 24},
                 {right.x + 34, right.y + 72}, Light);
    DrawTriangle({down.x + 40, down.y + 48}, {down.x + 62, down.y + 20}, {down.x + 18, down.y + 20},
                 Light);
    art.Text("JUMP", {jump.x + (jump.width - art.Measure("JUMP", 20, true)) / 2, jump.y + 44}, 20,
             Light, true);
    art.Text("DASH", {dash.x + (dash.width - art.Measure("DASH", 20, true)) / 2, dash.y + 32}, 20,
             touch.dashHeld ? Mint : Light, true);
    DrawRectangleRec({pause.x + 24, pause.y + 17, 7, 26}, Light);
    DrawRectangleRec({pause.x + 41, pause.y + 17, 7, 26}, Light);
}
void Game::DrawModal() {
    DrawMenuShade(Fade(Ink, 0.82f));
    DrawRectangleRec({390, 125, 500, 490}, Color{15, 28, 34, 248});
    DrawRectangleLinesEx({390, 125, 500, 490}, 1, Muted);
    DrawRectangleRec({390, 125, 500, 3}, Mint);
    if (page == Page::Pause) {
        art.Text("DELIVERY PAUSED", {440, 165}, 42, Light, true);
        art.Text("YOUR CHECKPOINT IS SAFE", {442, 221}, 13, Muted);
        if (Button(0, {440, 270, 400, 52}, "RESUME", touchEnabled ? "" : "ESC"))
            Switch(Page::Play);
        if (Button(1, {440, 334, 400, 52}, "SETTINGS")) {
            settingsReturn = Page::Pause;
            Switch(Page::Settings);
        }
        if (Button(2, {440, 398, 400, 52}, "RESTART CHECKPOINT")) {
            world->Respawn();
            stage->actor->transform->Teleport(world->player.position);
            Switch(Page::Play);
        }
        if (Button(3, {440, 462, 400, 52}, "SAVE & RETURN")) {
            Persist();
            Switch(Page::Home);
        }
        art.Text("A MOMENT TO BREATHE.", {442, 558}, 12, Muted);
    } else if (page == Page::ConfirmNew) {
        art.Text("A NEW DELIVERY?", {440, 175}, 42, Light, true);
        art.Text("Your current checkpoint will be replaced.", {442, 250}, 18, Muted);
        art.Text("Sector records and settings stay with you.", {442, 282}, 18, Muted);
        if (Button(0, {440, 360, 400, 52}, "KEEP CURRENT RUN"))
            Switch(Page::Home);
        if (Button(1, {440, 424, 400, 52}, "START NEW DELIVERY")) {
            if (!profile.NewRun()) {
                toast = "SAVE FAILED / PROGRESS WILL STAY IN THIS SESSION";
                toastTime = 6;
            }
            StartLevel(0, false);
        }
    } else {
        art.Text("BUILT WITH CARE", {440, 166}, 42, Light, true);
        art.Text("A RAYGAME REFERENCE GAME", {442, 227}, 14, Mint);
        art.Text("Original code, procedural artwork & audio", {442, 286}, 17, Muted);
        art.Text("Engine: RayGame / raylib 5.5", {442, 323}, 17, Muted);
        art.Text("Barlow: Jeremy Tribby / SIL OFL", {442, 360}, 17, Muted);
        art.Text("Rajdhani: Indian Type Foundry / SIL OFL", {442, 397}, 17, Muted);
        art.Text("Noto Sans SC: Adobe / Google / SIL OFL", {442, 434}, 15, Muted);
        if (Button(0, {440, 490, 400, 52}, "RETURN"))
            Switch(Page::Home);
    }
}
void Game::DrawSettings() {
    DrawMenuShade(Fade(Ink, 0.91f));
    art.Text("CONTROL ROOM", {245, 78}, 58, Light, true);
    art.Text("YOUR DELIVERY. YOUR PACE.", {249, 145}, 14, Mint);
    struct Row {
        const char* title;
        std::string value;
    };
    auto& s = profile.settings;
    Row rows[] = {{"MUSIC", std::to_string((int)std::lround(s.music * 100)) + "%"},
                  {"SOUND EFFECTS", std::to_string((int)std::lround(s.sfx * 100)) + "%"},
                  {"SCREEN SHAKE", s.shake ? "ON" : "OFF"},
                  {"DISPLAY", (touchEnabled || s.fullscreen) ? "FULLSCREEN" : "WINDOWED"},
                  {"FRAME LIMIT", std::to_string(s.fps) + " FPS"},
                  {"LANGUAGE", s.language == Language::Chinese ? "简体中文" : "English"}};
    for (int i = 0; i < 6; ++i) {
        bool disabled = touchEnabled && i == 3;
        bool clicked = Button(i, {245, 182 + i * 55.0f, 790, 46}, rows[i].title,
                              art.Localize(rows[i].value) + (disabled ? "" : "    <  >"), disabled);
        if (!disabled && (clicked || (focus == i && settingsDirection))) {
            int direction = settingsDirection ? settingsDirection : pointerDirection;
            if (i == 0)
                s.music = Clamp(s.music + direction * 0.1f, 0.0f, 1.0f);
            if (i == 1)
                s.sfx = Clamp(s.sfx + direction * 0.1f, 0.0f, 1.0f);
            if (i == 2)
                s.shake = !s.shake;
            if (i == 3)
                s.fullscreen = !s.fullscreen;
            if (i == 4) {
                int rates[] = {60, 120, 144};
                int n = s.fps == 60 ? 0 : s.fps == 120 ? 1 : 2;
                s.fps = rates[(n + direction + 3) % 3];
            }
            if (i == 5) {
                s.language =
                    s.language == Language::Chinese ? Language::English : Language::Chinese;
            }
            settingsDirection = 0;
            ApplySettings();
            if (i == 5)
                Persist();
        }
    }
    if (Button(6, {245, 540, 790, 52}, "SAVE & RETURN", touchEnabled ? "" : "ESC")) {
        Persist();
        Switch(settingsReturn);
    }
    art.Text(touchEnabled ? "TAP THE LEFT / RIGHT HALF TO ADJUST"
                          : "UP / DOWN TO SELECT    LEFT / RIGHT TO ADJUST",
             {245, 626}, 13, Muted);
}
void Game::DrawDebrief() {
    DrawMenuShade(Fade(Ink, 0.84f));
    art.Glow({640, 310}, 300, Mint, 0.16f);
    bool final = world->level.index == 2;
    std::string title = final ? "THE FACTORY LIVES" : "DELIVERY COMPLETE";
    auto sector = art.Localize("SECTOR") + " 0" + std::to_string(world->level.index + 1) + " / " +
                  art.Localize("SYNCHRONIZED");
    art.Text(sector, {640 - art.Measure(sector, 14) / 2, 130}, 14, Mint);
    art.Text(title, {640 - art.Measure(title, 76, true) / 2, 173}, 76, Light, true);
    std::string line = final ? "You carried the last light. Now the whole world can wake."
                             : "One step closer to the heart of the machine.";
    art.Text(line, {640 - art.Measure(line, 20) / 2, 264}, 20, Muted);
    DrawRectangleRec({330, 325, 620, 1}, Fade(Muted, 0.5f));
    char duration[32];
    std::snprintf(duration, sizeof(duration), "%02d:%02d", (int)world->elapsed / 60,
                  (int)world->elapsed % 60);
    art.Text("DELIVERY TIME", {340, 350}, 12, Muted);
    art.Text(duration, {340, 376}, 44, Light, true);
    art.Text("POWER CELLS", {565, 350}, 12, Muted);
    art.Text(std::to_string(world->CellCount()), {565, 376}, 44, Mint, true);
    art.Text("RESTARTS", {790, 350}, 12, Muted);
    art.Text(std::to_string(world->deaths), {790, 376}, 44, Ember, true);
    if (Button(0, {385, 474, 510, 56}, final ? "RETURN TO THE FOUNDRY" : "ENTER NEXT SECTOR")) {
        if (final) {
            Persist();
            stage.reset();
            LoadScene(nullptr);
            Switch(Page::Home);
        } else
            StartLevel(world->level.index + 1, false);
    }
    if (Button(1, {385, 546, 510, 50}, "SAVE & RETURN")) {
        Persist();
        Switch(Page::Home);
    }
    art.Text(
        final ? "THANK YOU FOR PLAYING" : "THE LAST DELIVERY CONTINUES",
        {640 - art.Measure(final ? "THANK YOU FOR PLAYING" : "THE LAST DELIVERY CONTINUES", 12) / 2,
         643},
        12, Muted);
}
void Game::OnRender() {
    if (page == Page::Home) {
        art.camera.target = {1100 + std::sin(art.clock * 0.07f) * 90, 570};
        art.camera.offset = {Display::GetWidth() / 2, Display::GetHeight() / 2};
        art.DrawWorld(*world, world->player.position, true);
        BeginMenuLayout();
        DrawHome();
        rlPopMatrix();
    } else {
        Vector2 p = stage && stage->actor ? stage->actor->transform->GetRenderPosition()
                                          : world->player.position;
        art.DrawWorld(*world, p, false);
        if (page == Page::Play || page == Page::Pause)
            DrawHUD();
        if (page == Page::Play && touchEnabled)
            DrawTouchControls();
        BeginMenuLayout();
        if (page == Page::Pause || page == Page::ConfirmNew || page == Page::Credits)
            DrawModal();
        if (page == Page::Settings)
            DrawSettings();
        if (page == Page::Debrief)
            DrawDebrief();
        rlPopMatrix();
    }
    if (toastTime > 0) {
        BeginMenuLayout();
        float width = art.Measure(toast, 14) + 48;
        DrawRectangleRec({640 - width / 2, 620, width, 38}, Color{23, 45, 43, 245});
        art.Text(toast, {640 - art.Measure(toast, 14) / 2, 630}, 14, Mint);
        rlPopMatrix();
    }
    lastMouse = Vector2Subtract(Input::GetMousePosition(), MenuOffset());
    if (Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT))
        mousePress = false;
    activate = false;
    if (smoke) {
        if (frame == 2)
            Capture("home");
        if (frame == 8)
            Capture("settings");
        if (frame == 16)
            Capture("foundry");
        if (frame == 24)
            Capture("pause");
        if (frame == 36)
            Capture("conduit");
        if (frame == 48)
            Capture("heart");
        if (frame == 56)
            Capture("complete");
        if (frame == 112)
            Capture("settings-en");
        if (frame == 116)
            Capture("home-en");
        if (frame == 120)
            Capture("foundry-en");
        if (frame == 124)
            Capture("pause-en");
        if (frame == 137)
            Capture("settings-zh");
        if (frame == 142)
            Capture("pause-zh");
        if (frame == 148)
            Capture("home-zh");
        if (frame == 153)
            Capture("confirm-zh");
        if (frame == 158)
            Capture("credits-zh");
        if (frame == 170)
            Capture("complete-en");
        if (frame == 177 || frame == 218)
            Capture("touch-play");
        if (frame == 181)
            Capture("touch-pause");
        if (frame == 195)
            Capture("touch-settings");
        if (frame == 206)
            Capture("touch-home");
        if (frame == 209)
            Capture("touch-credits");
        if (frame == 213)
            Capture("touch-confirm");
        if (frame >= 224)
            Quit();
    }
}
void Game::Expect(bool value, const std::string& message) {
    if (!value) {
        ++failures;
        Diagnostics::Write(LogLevel::Error, "FORGE smoke: " + message);
    }
}
void Game::Capture(const std::string& name) {
    std::filesystem::create_directories(profile.directory / "screenshots");
    rlDrawRenderBatchActive();
    Image image = LoadImageFromTexture(Display::GetCanvasTexture());
    Expect(IsImageValid(image), "framebuffer can be read");
    if (!IsImageValid(image))
        return;
    ImageFlipVertical(&image);
    ImageResize(&image, (int)std::lround(720 * Display::GetWidth() / Display::GetHeight()), 720);
    auto filename = (profile.directory / "screenshots" / (name + ".png")).string();
    Expect(IsImageValid(image) && ExportImage(image, filename.c_str()), "screenshot " + name);
    UnloadImage(image);
}
void Game::SmokeInput() {
    Input::Reset();
    InputFrame input;
    input.mousePosition = {-100, -100};
    auto key = [&](int code) { input.keys.pressed[code] = true; };
    auto pad = [&](int button) {
        input.gamepads[0].available = true;
        input.gamepads[0].buttons.pressed[button] = true;
    };
    switch (frame) {
    case 65:
    case 68:
    case 72:
    case 78:
    case 82:
    case 89:
    case 100:
    case 150:
        key(KEY_ENTER);
        break;
    case 67:
    case 71:
    case 75:
    case 77:
    case 81:
    case 88:
    case 103:
        key(KEY_DOWN);
        break;
    case 70:
    case 79:
    case 92:
    case 93:
    case 95:
    case 102:
    case 106:
    case 114:
    case 123:
    case 141:
    case 154:
        key(KEY_ESCAPE);
        break;
    case 73:
    case 76:
    case 110:
        key(KEY_RIGHT);
        break;
    case 84:
    case 85:
    case 86:
        pad(GAMEPAD_BUTTON_MIDDLE_RIGHT);
        break;
    case 90:
    case 104:
        pad(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        break;
    case 97:
    case 98:
        input.mousePosition = Display::CanvasToScreen(Vector2Add({180, 510}, MenuOffset()));
        if (frame == 97)
            input.mouse.held[MOUSE_BUTTON_LEFT] = true;
        else
            input.mouse.released[MOUSE_BUTTON_LEFT] = true;
        break;
    case 99:
        input.focused = false;
        break;
    case 126:
    case 128:
    case 130:
    case 132:
    case 134:
        pad(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
        break;
    case 135:
        pad(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        break;
    case 136:
        pad(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
        break;
    case 156:
        key(KEY_C);
        break;
    default:
        break;
    }
    if (frame == 100) {
        InputFrame lost;
        lost.focused = false;
        Input::SetFrame(lost);
    }
    auto finger = [&](int id, Vector2 position) {
        if (page == Page::Play) {
            if (position.x > 640)
                position.x += Display::GetWidth() - 1280;
            if (position.y > 360)
                position.y += Display::GetHeight() - 720;
        } else
            position = Vector2Add(position, MenuOffset());
        input.touches[input.touchCount++] = {id, Display::CanvasToScreen(position)};
    };
    if (frame >= 174 && frame <= 179)
        finger(17, {200, 590});
    if (frame >= 174 && frame <= 176)
        finger(29, {1168, 570});
    if (frame == 176)
        finger(41, {1028, 590});
    if (frame == 180)
        finger(55, {1188, 122});
    if (frame == 182 || frame == 199)
        finger(65, {640, 296});
    if (frame >= 182 && frame <= 184)
        finger(88, {1168, 570});
    if (frame == 188)
        finger(65, {640, 360});
    if (frame == 191)
        finger(65, {300, 205});
    if (frame == 193)
        finger(65, {980, 205});
    if (frame == 196)
        finger(65, {980, 370});
    if (frame == 201)
        input.focused = false;
    if (frame == 202)
        finger(88, {1168, 570});
    if (frame == 204)
        finger(65, {640, 488});
    if (frame == 207)
        finger(65, {1100, 665});
    if (frame == 211)
        finger(65, {200, 455});
    if (frame == 214)
        finger(65, {640, 384});
    if (frame == 216)
        finger(65, {200, 390});
    if (frame == 186 || frame == 198 || frame == 210 || frame == 221)
        key(KEY_BACK);
    Input::SetFrame(input);
}
void Game::SmokeStep() {
    if (frame == 5) {
        settingsReturn = Page::Home;
        Switch(Page::Settings);
    }
    if (frame == 12)
        StartLevel(0, false);
    if (frame == 14) {
        world->player.position = {1390, 590};
        stage->actor->transform->Teleport(world->player.position);
        art.Follow(*world, world->player.position, 0, true);
    }
    if (frame == 20) {
        Expect(stage && stage->actor && art.body && art.glow,
               "stage and graphics resources are ready");
        Persist();
        Switch(Page::Pause);
        pausedWorldTime = world->time;
    }
    if (frame == 28) {
        float before = world->time;
        Expect(before > 0, "fixed simulation advances");
        Expect(world->time == pausedWorldTime, "custom pause freezes world simulation");
        Switch(Page::Play);
    }
    if (frame == 32)
        StartLevel(1, false);
    if (frame == 34) {
        world->player.position = {2200, 565};
        stage->actor->transform->Teleport(world->player.position);
        art.Follow(*world, world->player.position, 0, true);
    }
    if (frame == 42)
        StartLevel(2, false);
    if (frame == 44) {
        world->player.position = {4330, 540};
        stage->actor->transform->Teleport(world->player.position);
        art.Follow(*world, world->player.position, 0, true);
    }
    if (frame == 52) {
        world->collected = (std::uint64_t{1} << world->level.cells.size()) - 1;
        world->player.position = world->level.goal;
        world->Step({}, 1.0f / 60);
        Expect(world->complete, "final exit requires and accepts all cells");
        profile.Record(*world);
        Switch(Page::Debrief);
    }
    if (frame == 60) {
        Profile reloaded(profile.directory);
        reloaded.Load();
        Expect(reloaded.completed && reloaded.level == 2, "campaign persists across processes");
        Expect(!AreDefaultShortcutsEnabled(), "reference game owns menu shortcuts");
    }
    if (frame == 61)
        SetWindowSize(960, 720);
    if (frame == 62) {
        auto v = Display::GetViewport();
        Expect(v.bounds.x == 0 && v.bounds.y == 0 && v.logicalSize.y > 720,
               "4:3 expands the world without stretching");
        auto p = Display::ScreenToCanvas(
            {v.bounds.x + v.bounds.width / 2, v.bounds.y + v.bounds.height / 2});
        Expect(std::fabs(p.x - v.logicalSize.x / 2) < 0.01f &&
                   std::fabs(p.y - v.logicalSize.y / 2) < 0.01f,
               "expanded canvas maps the pointer into logical UI");
        SetWindowSize(1680, 720);
    }
    if (frame == 63) {
        auto v = Display::GetViewport();
        Expect(v.bounds.x == 0 && v.logicalSize.x > 1280,
               "ultrawide fills the surface with extra world space");
    }
    if (frame == 66)
        Expect(page == Page::Home && !profile.active,
               "final result returns home without reactivating campaign");
    if (frame == 69)
        Expect(page == Page::Play && world->level.index == 0 && world->collected == 0,
               "keyboard starts a fresh delivery");
    if (frame == 71)
        Expect(page == Page::Pause, "Escape opens custom pause");
    if (frame == 72) {
        smokeMusic = profile.settings.music;
        smokeShake = profile.settings.shake;
    }
    if (frame == 73)
        Expect(page == Page::Settings && settingsReturn == Page::Pause,
               "pause menu opens settings");
    if (frame == 74)
        Expect(profile.settings.music > smokeMusic, "arrow input adjusts music");
    if (frame == 80) {
        Expect(page == Page::Pause && profile.settings.shake != smokeShake,
               "settings return to pause");
        Profile loaded(profile.directory);
        loaded.Load();
        Expect(loaded.settings.shake == profile.settings.shake &&
                   std::fabs(loaded.settings.music - profile.settings.music) < 0.001f,
               "menu persists settings");
        focus = 1;
    }
    if (frame == 83)
        Expect(page == Page::Play, "checkpoint restart returns to play");
    if (frame == 84)
        Expect(page == Page::Pause, "controller pauses gameplay");
    if (frame == 85)
        Expect(page == Page::Play, "controller resumes gameplay");
    if (frame == 87)
        Expect(page == Page::Pause, "controller resumes then pauses again");
    if (frame == 87)
        focus = 2;
    if (frame == 90)
        Expect(page == Page::Home && profile.active, "pause saves and returns home");
    if (frame == 91)
        Expect(page == Page::Play && profile.active, "controller continues saved delivery");
    if (frame == 94)
        Expect(page == Page::Play, "continued game can pause and resume");
    if (frame == 96) {
        Persist();
        Switch(Page::Home);
    }
    if (frame == 99)
        Expect(page == Page::Settings && settingsReturn == Page::Home,
               "mouse opens settings from home");
    if (frame == 101)
        Expect(page == Page::Settings, "regaining focus does not activate a menu key");
    if (frame == 105)
        Expect(page == Page::ConfirmNew && profile.active,
               "starting over asks before replacing a saved delivery");
    if (frame == 107)
        Expect(page == Page::Home && profile.active, "cancelling new delivery preserves the save");
    if (frame == 108) {
        auto points = art.catalog.ChineseCodepoints();
        for (bool heading : {false, true}) {
            auto font = art.ChineseFont(24, heading);
            bool complete = std::all_of(points.begin(), points.end(), [&](int code) {
                for (int i = 0; i < font->glyphCount; ++i)
                    if (font->glyphs[i].value == code)
                        return true;
                return false;
            });
            Expect(complete, "every translated character is present in the Chinese font atlas");
        }
        auto target = LoadRenderTexture(320, 80);
        BeginTextureMode(target);
        ClearBackground(BLACK);
        art.Text("继续旅程", {12, 12}, 24, WHITE);
        EndTextureMode();
        Image pixels = LoadImageFromTexture(target.texture);
        int lit = 0;
        if (IsImageValid(pixels))
            for (int y = 0; y < pixels.height; ++y)
                for (int x = 0; x < pixels.width; ++x)
                    if (GetImageColor(pixels, x, y).r > 80)
                        ++lit;
        Expect(lit > 250, "Chinese glyphs produce visible framebuffer pixels");
        UnloadImage(pixels);
        UnloadRenderTexture(target);
    }
    if (frame == 109) {
        settingsReturn = Page::Home;
        Switch(Page::Settings);
        focus = 5;
    }
    if (frame == 111) {
        Expect(profile.settings.language == Language::English &&
                   art.Localize("CONTINUE") == "CONTINUE",
               "keyboard switches the UI to English");
        Profile loaded(profile.directory);
        loaded.Load();
        Expect(loaded.settings.language == Language::English, "English language persists");
    }
    if (frame == 118)
        StartLevel(0, true);
    if (frame == 122) {
        smokeElapsed = world->elapsed;
        smokeCollected = world->collected;
    }
    if (frame == 125) {
        Expect(page == Page::Pause, "English pause works");
        settingsReturn = Page::Pause;
        Switch(Page::Settings);
    }
    if (frame == 137) {
        Expect(profile.settings.language == Language::Chinese &&
                   art.Localize("CONTINUE") == "继续旅程",
               "controller switches the UI to Chinese");
        Profile loaded(profile.directory);
        loaded.Load();
        Expect(loaded.settings.language == Language::Chinese, "Chinese language persists");
        Expect(world->collected == smokeCollected && world->elapsed >= smokeElapsed,
               "language switching preserves the campaign");
    }
    if (frame == 143)
        Expect(page == Page::Pause, "language settings return to their original page");
    if (frame == 146) {
        Persist();
        Switch(Page::Home);
    }
    if (frame == 149)
        focus = 1;
    if (frame == 157)
        Expect(page == Page::Credits, "localized credits are reachable");
    if (frame == 161) {
        profile.settings.language = Language::English;
        ApplySettings();
        StartLevel(2, false);
    }
    if (frame == 166) {
        world->collected = (std::uint64_t{1} << world->level.cells.size()) - 1;
        world->player.position = world->level.goal;
        world->Step({}, 1.0f / 60);
        Persist();
        Switch(Page::Debrief);
    }
    if (frame == 172) {
        touchEnabled = true;
        profile.settings.language = Language::Chinese;
        ApplySettings();
        StartLevel(0, false);
        targetFPS = 60;
        SetTargetFPS(60);
        Expect(art.Localize("SHIFT TO DASH") == art.Localize("TOUCH DASH"),
               "touch mode replaces keyboard signs");
    }
    if (frame == 173) {
        smokeTouchX = world->player.position.x;
        smokeTouchY = world->player.position.y;
    }
    if (frame == 180) {
        Expect(page == Page::Pause && world->player.position.x > smokeTouchX &&
                   world->player.position.y < smokeTouchY,
               "multiple touches drive the pilot and the pause button opens the menu");
        pausedWorldTime = world->time;
    }
    if (frame == 182)
        Expect(world->time == pausedWorldTime, "touch pause freezes the simulation");
    if (frame == 184)
        Expect(page == Page::Play && !touch.Held().jumpHeld,
               "touch resume rejects a finger held across the page change");
    if (frame == 187)
        Expect(page == Page::Pause, "Android Back opens the pause menu");
    if (frame == 190) {
        Expect(page == Page::Settings && settingsReturn == Page::Pause, "touch opens settings");
        smokeMusic = profile.settings.music;
        smokeFullscreen = profile.settings.fullscreen;
    }
    if (frame == 193)
        Expect(profile.settings.music < smokeMusic, "left side of a setting decreases its value");
    if (frame == 195)
        Expect(std::fabs(profile.settings.music - smokeMusic) < 0.001f,
               "right side of a setting increases its value");
    if (frame == 198)
        Expect(page == Page::Pause && profile.settings.fullscreen == smokeFullscreen,
               "mobile display stays fullscreen and Back returns from settings");
    if (frame == 201)
        Expect(page == Page::Pause, "focus loss saves and pauses touch gameplay");
    if (frame == 203)
        Expect(!touch.Held().jumpHeld, "focus regain does not create a touch jump");
    if (frame == 206) {
        Profile loaded(profile.directory);
        loaded.Load();
        Expect(page == Page::Home && loaded.active && loaded.collected == world->collected,
               "touch save-and-return persists the delivery");
    }
    if (frame == 209)
        Expect(page == Page::Credits, "credits can be opened without a keyboard");
    if (frame == 213)
        Expect(page == Page::ConfirmNew, "touch new delivery requires overwrite confirmation");
    if (frame == 216)
        Expect(page == Page::Home && profile.active, "touch cancellation retains the existing run");
    if (frame == 218)
        Expect(page == Page::Play && profile.active, "touch continues the saved delivery");
    if (frame == 220) {
        Persist();
        Switch(Page::Home);
    }
    if (frame == 221)
        Expect(savedExit, "Android Back saves and exits from the home screen");
}
} // namespace Forge
