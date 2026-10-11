#include "LabScene.h"
#include "../common/ExampleUI.h"
#include <fstream>

namespace Lab {
class Application : public GameEngine {
public:
    Ref<LabScene> lab, nextLab;
    std::filesystem::path data;
    SpeechOptions speech;
    std::string notice = "Choose a page. Change a value. Watch the objects respond.";
    TouchTracker touches;
    int mouseCapture = -1, touchCapture = -1, finger = -1;
    bool switching = false;

    explicit Application(std::filesystem::path directory)
        : GameEngine("RayGame | Engine Lab", 1280, 720), data(std::move(directory)) {
        SetCanvasScaleMode(CanvasScaleMode::Expand);
        SetDefaultShortcutsEnabled(false);
        clearColor = {13, 24, 34, 255};
    }
    Rectangle WorkArea() const {
        return {262, 112, Display::GetWidth() - 290, Display::GetHeight() - 308};
    }
    Rectangle Tab(int i) const {
        return {24, 136.0f + i * 56, 210, 46};
    }
    Rectangle Button(int i) const {
        auto area = WorkArea();
        float width = (area.width - 36) / 4;
        return {area.x + (i % 4) * (width + 12), Display::GetHeight() - 162 + (i / 4) * 54.0f,
                width, 44};
    }
    void Select(int page) {
        if (switching)
            return;
#if defined(__ANDROID__)
        std::string assets;
#else
        std::string assets = std::string(GetApplicationDirectory()) + "assets/engine_lab";
#endif
        nextLab = CreateRef<LabScene>(page, assets);
        LoadScene(nextLab);
        switching = true;
        mouseCapture = touchCapture = finger = -1;
        touches.Reset(Input::GetFrame());
    }
    void OnInit() override {
        ExampleUI::Initialize(Cyan, "engine_lab");
        Select(0);
    }
    int Hit(Vector2 p) const {
        for (int i = 0; i < 6; ++i)
            if (CheckCollisionPointRec(p, Tab(i)))
                return i;
        if (CheckCollisionPointRec(p, {24, 492, 210, 44}))
            return 6;
        int count = lab->page == 5 ? 8 : 3;
        for (int i = 0; i < count; ++i)
            if (CheckCollisionPointRec(p, Button(i)))
                return 10 + i;
        return lab->page == 4 && CheckCollisionPointRec(p, WorkArea()) ? 30 : -1;
    }
    void Activate(int id, Vector2 p) {
        if (id < 0 || switching)
            return;
        if (id <= 6) {
            Select(id == 6 ? lab->page : id);
            return;
        }
        if (id == 30) {
            lab->Paint(p);
            return;
        }
        int action = id - 10;
        if (lab->page != 5) {
            lab->Action(action);
            return;
        }
        if (action <= 1) {
            speech.language = action == 0 ? "en-US" : "zh-CN";
            bool accepted =
                TextToSpeech::Speak(action == 0 ? "Welcome to the RayGame engine laboratory."
                                                : u8"欢迎来到 RayGame 引擎实验室。",
                                    speech);
            notice = accepted ? "Speech requested. The system must have a voice for this language."
                              : TextToSpeech::GetLastError();
        } else if (action == 2) {
            TextToSpeech::Stop();
            notice = "Speech stopped.";
        } else if (action == 3)
            speech.rate = speech.rate >= 2 ? 0.5f : speech.rate + 0.5f;
        else if (action == 4)
            speech.volume = speech.volume >= 1 ? 0 : speech.volume + 0.25f;
        else if (action == 5) {
            SaveSystem save;
            save.SetFloat("rate", speech.rate);
            save.SetFloat("volume", speech.volume);
            notice = save.Save(data / "preferences.save")
                         ? "Saved speech settings to your application data folder."
                         : "Save failed. See the log.";
        } else if (action == 6) {
            SaveSystem save;
            if (save.Load(data / "preferences.save")) {
                speech.rate = std::clamp(save.GetFloat("rate", 1), 0.5f, 2.0f);
                speech.volume = std::clamp(save.GetFloat("volume", 1), 0.0f, 1.0f);
                notice = "Loaded saved speech settings.";
            } else
                notice = "No readable save yet. Save your settings first.";
        } else if (action == 7) {
            AudioManager::PlaySound("lab.chime", 0.4f);
            notice = "Played the bundled chime through AudioManager.";
        }
    }
    void OnUnscaledUpdate() override {
        if (nextLab && GetCurrentScene() == nextLab) {
            lab = std::move(nextLab);
            switching = false;
        }
        if (!lab || !lab->ready)
            return;
        lab->Layout(WorkArea());
        auto& frame = Input::GetFrame();
        touches.Update(frame, Display::GetViewport());
        if (touches.Cancelled())
            mouseCapture = touchCapture = finger = -1;
        if (!frame.focused)
            return;
        if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK))
            Quit();
        if (Input::GetKeyDown(KEY_F11))
            ToggleFullscreen();
        for (int i = 0; i < 6; ++i)
            if (Input::GetKeyDown(KEY_ONE + i)) {
                Select(i);
                return;
            }
        if (Input::GetKeyDown(KEY_R)) {
            Select(lab->page);
            return;
        }
        if (finger < 0)
            for (const auto& c : touches.Contacts()) {
                if (c.Active() && c.pressed) {
                    finger = c.id;
                    touchCapture = Hit(c.position);
                    break;
                }
            }
        for (const auto& c : touches.Released())
            if (c.Active() && c.id == finger) {
                int id = touchCapture;
                finger = touchCapture = -1;
                if (id == Hit(c.position))
                    Activate(id, c.position);
                return;
            }
        if (finger >= 0 && !touches.IsActive(finger))
            finger = touchCapture = -1;
#if !defined(__ANDROID__)
        if (frame.touchCount == 0 && finger < 0) {
            auto p = Input::GetMousePosition();
            if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT))
                mouseCapture = Hit(p);
            if (Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT)) {
                int id = mouseCapture;
                mouseCapture = -1;
                if (id == Hit(p))
                    Activate(id, p);
            }
        }
#endif
    }
    void DrawButton(Rectangle r, const std::string& label, bool selected = false) {
        bool hover =
            Input::GetFrame().focused && CheckCollisionPointRec(Input::GetMousePosition(), r);
        DrawRectangleRounded(r, 0.16f, 6,
                             selected ? Color{31, 75, 85, 255}
                             : hover  ? Color{37, 56, 69, 255}
                                      : Color{25, 42, 55, 255});
        if (selected)
            DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y + 10), 3,
                          static_cast<int>(r.height - 20), Cyan);
        UI::Label(r.x + 16, r.y + 11, label, 20, selected ? Cyan : Color{215, 229, 238, 255});
    }
    void OnRender() override {
        if (!lab || !lab->ready)
            return;
        float w = Display::GetWidth(), h = Display::GetHeight();
        DrawRectangle(0, 0, 248, static_cast<int>(h), {17, 30, 42, 255});
        UI::Label(26, 25, "RAYGAME / EXAMPLES", 16, Cyan);
        UI::Title(24, 47, "ENGINE LAB", 38);
        UI::Label(26, 104, "EXPLORE THE ENGINE", 14, ExampleUI::muted);
        for (int i = 0; i < 6; ++i)
            DrawButton(Tab(i), std::to_string(i + 1) + "   " + Names[i], i == lab->page);
        DrawButton({24, 492, 210, 44}, "Reset page   /   R");
        UI::Label(26, h - 141, "LIVE SCENE", 14, ExampleUI::muted);
        UI::Title(24, h - 120,
                  TextFormat("%02d", static_cast<int>(lab->GetAllGameObjects().size())), 44, Cyan);
        UI::Label(85, h - 106, "objects", 18, ExampleUI::muted);
        UI::Label(26, h - 61, "1 - 6  pages    F11  fullscreen", 16, ExampleUI::muted);
        UI::Label(26, h - 36, "Esc / Back  exit", 16, ExampleUI::muted);
        UI::Title(268, 24, Names[lab->page], 44);
        UI::Label(268, 77, Descriptions[lab->page], 20, ExampleUI::muted);
        auto area = WorkArea();
        DrawRectangleLinesEx(area, 1, {45, 65, 78, 255});
        std::string status;
        auto& m = *lab->metrics;
        if (lab->page == 0)
            status = TextFormat("Awake %d   Start %d   Destroy %d   |   Rotation %.0f deg", m.awake,
                                m.starts, m.destroyed, lab->subject->transform->rotation);
        if (lab->page == 1)
            status =
                TextFormat("Contacts %d   Trigger entries %d   |   Bodies %d", m.contacts,
                           m.triggers, static_cast<int>(lab->FindGameObjectsByTag("body").size()));
        if (lab->page == 2)
            status = TextFormat(
                "Texture revision %d   |   Animation %s   |   Shared resource: lab.sheet",
                static_cast<int>(lab->sheet.Revision()),
                lab->animation->playing ? "playing" : "paused");
        if (lab->page == 3)
            status = TextFormat("Particles %d   Events %d   Pool %d / 16   |   Timer %s",
                                lab->emitter->GetParticleCount(), m.events,
                                static_cast<int>(lab->pool.InUse()),
                                lab->timer->IsEnabled() ? "running" : "paused");
        if (lab->page == 4) {
            int x = static_cast<int>(lab->selectedTile.x),
                y = static_cast<int>(lab->selectedTile.y);
            status =
                TextFormat("Selected [%d, %d]   ID %d   Solid %s   |   Click / tap tiles to paint",
                           x, y, lab->map->GetTile(x, y), lab->map->IsSolid(x, y) ? "yes" : "no");
        }
        if (lab->page == 5) {
            const char* states[] = {"idle", "initializing", "ready", "speaking", "unavailable"};
            status = TextFormat("System TTS: %s   |   Rate %.2fx   Volume %.0f%%",
                                states[static_cast<int>(TextToSpeech::GetState())], speech.rate,
                                speech.volume * 100);
            const char* labels[] = {"TTS", "SAVE", "INPUT", "FILES"};
            for (int i = 0; i < 4; ++i) {
                float a = i * PI / 2;
                auto p = lab->camera->WorldToScreen({std::cos(a) * 190, std::sin(a) * 130 + 28});
                UI::LabelCentered(p.x, p.y, labels[i], 18, ExampleUI::muted);
            }
            auto center = lab->camera->WorldToScreen({0, 55});
            UI::LabelCentered(center.x, center.y, "APPLICATION", 18, Cyan);
        }
        UI::Label(area.x + 12, area.y + area.height - 30, status, 18, Cyan);
        const char* actions[][8] = {{"Toggle rotation", "Create / destroy", "Change scale"},
                                    {"Impulse", "Spawn body", "Camera follow"},
                                    {"Replace texture", "Play / pause", "Flip sprite"},
                                    {"Burst + event", "Toggle timer", "Emitter shape"},
                                    {"Paint selected", "Erase selected", "Toggle solid ID"},
                                    {"Speak English", "Speak Chinese", "Stop speech", "Change rate",
                                     "Change volume", "Save settings", "Load settings",
                                     "Play sound"}};
        for (int i = 0; i < (lab->page == 5 ? 8 : 3); ++i)
            DrawButton(Button(i), actions[lab->page][i]);
        std::string message =
            lab->page == 5
                ? (TextToSpeech::GetLastError().empty() ? notice : TextToSpeech::GetLastError())
                : "Each page creates real engine objects. Reset rebuilds the scene and releases "
                  "its callbacks.";
        // Clip by glyph boundary, preserving UTF-8 error messages from the OS.
        while (message.size() > 4 &&
               MeasureTextEx(UI::font ? *UI::font : GetFontDefault(), message.c_str(), 16, 0.5f).x >
                   w - 298) {
            if (message.size() >= 3 && message.compare(message.size() - 3, 3, "...") == 0)
                message.resize(message.size() - 3);
            size_t last = message.size() - 1;
            while (last > 0 && (static_cast<unsigned char>(message[last]) & 0xc0) == 0x80)
                --last;
            message.resize(last);
            message += "...";
        }
        UI::Label(266, h - 32, message, 16, ExampleUI::muted);
    }

    // Deterministic input and real rendering, with an isolated data directory.
    void Step(InputFrame input = {}) {
        Display::Refresh();
        Input::SetFrame(input);
        Tick(1.0f / 60);
        Display::Begin();
        ClearBackground(clearColor);
        if (GetCurrentScene())
            GetCurrentScene()->Render();
        OnRender();
        Display::End();
    }
    void Click(Vector2 p, bool touch = false) {
        InputFrame input;
        p = Display::CanvasToScreen(p);
        if (touch) {
            input.touchCount = 1;
            input.touches[0] = {7, p};
        } else {
            input.mousePosition = p;
            input.mouse.held[MOUSE_BUTTON_LEFT] = true;
            input.mouse.pressed[MOUSE_BUTTON_LEFT] = true;
        }
        Step(input);
        input = {};
        input.mousePosition = p;
        if (!touch)
            input.mouse.released[MOUSE_BUTTON_LEFT] = true;
        Step(input);
        Step();
    }
    void ClickButton(int i) {
        auto r = Button(i);
        Click({r.x + 22, r.y + 22});
    }
    void ClickTab(int i) {
        auto r = Tab(i);
        Click({r.x + 22, r.y + 22}, true);
    }
    int Smoke() {
        int failures = 0;
        std::ofstream report(data / "smoke.log");
        auto check = [&](bool ok, const char* name) {
            report << (ok ? "PASS " : "FAIL ") << name << '\n';
            if (!ok)
                ++failures;
        };
        auto capture = [&](const char* name) {
            Image image = LoadImageFromTexture(Display::GetCanvasTexture());
            ImageFlipVertical(&image);
            check(ExportImage(image, (data / (std::string(name) + ".png")).u8string().c_str()),
                  "screenshot export");
            UnloadImage(image);
        };
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        Init();
        SetTargetFPS(0);
        Step();
        Step();
        check(lab->satellite->transform->GetParent() == lab->subject->transform.get(),
              "real transform hierarchy");
        ClickButton(0);
        float angle = lab->subject->transform->rotation;
        Step();
        check(!lab->spin->IsEnabled() && angle == lab->subject->transform->rotation,
              "disabled component stops rotating");
        ClickButton(1);
        check(lab->temporary && !lab->temporary->IsDestroyed(), "deferred creation");
        ClickButton(1);
        Step();
        check(lab->metrics->destroyed == 1, "deferred destruction callback");
        capture("objects");
        ClickTab(1);
        for (int i = 0; i < 180; ++i)
            Step();
        check(lab->metrics->contacts > 0 && lab->metrics->triggers > 0,
              "physics contacts and triggers");
        ClickButton(1);
        check(lab->FindGameObjectsByTag("body").size() == 4, "spawn body through UI");
        capture("physics");
        ClickTab(2);
        auto revision = lab->sheet.Revision();
        check(static_cast<bool>(lab->sheet), "texture loaded");
        int frameBefore = lab->animation->currentFrame;
        for (int i = 0; i < 12; ++i)
            Step();
        check(lab->animation->currentFrame != frameBefore, "animation advances with frame time");
        ClickButton(0);
        check(lab->sheet.Revision() > revision, "existing handle sees replacement");
        ClickButton(1);
        check(!lab->animation->playing, "animation pause through UI");
        capture("sprites");
        ClickTab(3);
        for (int i = 0; i < 90; ++i)
            Step();
        auto oldMetrics = lab->metrics;
        check(oldMetrics->events >= 2 && lab->emitter->GetParticleCount() > 0 &&
                  lab->pool.InUse() > 0,
              "timer drives events, particles and pool");
        capture("effects");
        ClickTab(4);
        int events = oldMetrics->events;
        for (int i = 0; i < 50; ++i)
            Step();
        EventBus::Emit("lab.pulse");
        check(oldMetrics->events == events && TweenManager::Count() == 0,
              "scene swap cancels events and tween");
        auto p = lab->camera->WorldToScreen(lab->map->TileToWorld(2, 3));
        p.x += 8;
        p.y += 8;
        Click(p, true);
        check(lab->selectedTile.x == 2 && lab->selectedTile.y == 3 && lab->map->GetTile(2, 3) == 2,
              "touch maps through canvas and camera to tile");
        ClickButton(1);
        check(lab->map->GetTile(2, 3) == -1, "erase selected tile");
        capture("tilemap");
        ClickTab(5);
        speech.rate = 1.5f;
        speech.volume = 0.25f;
        ClickButton(5);
        speech.rate = 1;
        speech.volume = 1;
        ClickButton(6);
        check(speech.rate == 1.5f && speech.volume == 0.25f, "save/load actual preferences");
        capture("services");
        for (auto size : {Vector2{1024, 768}, {1600, 720}, {720, 1000}}) {
            SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
            Step();
            Step();
            ClickTab(4);
            check(lab->page == 4 && lab->ready, "resized touch navigation");
            auto r = WorkArea();
            check(r.width > 0 && r.height > 0, "adaptive work area");
            capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        }
        Shutdown();
        check(TextToSpeech::GetState() == SpeechState::Uninitialized, "engine releases speech");
        report << failures << " failures\n";
        return failures ? 1 : 0;
    }
};
} // namespace Lab

int main(int argc, char** argv) {
    bool smoke = false;
    std::filesystem::path data;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--smoke")
            smoke = true;
        else if (std::string(argv[i]) == "--data" && i + 1 < argc)
            data = std::filesystem::u8path(argv[++i]);
    }
    if (smoke && data.empty())
        return 2;
    if (data.empty())
        data = RayGame::FileSystem::UserDataDirectory("raygame.engine_lab");
    std::filesystem::create_directories(data);
    RayGame::Diagnostics::Initialize(data / "logs/game.log", false);
    RayGame::Diagnostics::InstallRaylibCallback();
    int result = 0;
    try {
        Lab::Application app(data);
        if (smoke)
            result = app.Smoke();
        else
            app.Run();
    } catch (const std::exception& error) {
        RayGame::Diagnostics::Write(RayGame::LogLevel::Error, error.what());
        result = 1;
    }
    RayGame::Diagnostics::Shutdown();
    return result;
}
