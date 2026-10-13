#pragma once
#include "ExampleUI.h"
#include <fstream>

// Shared shell for the two campaign examples. Game rules remain in their own models.
namespace Campaign {
using namespace RayGame;
using ExampleUI::Triangle;
enum class Screen { Menu, Levels, Help, Play, Pause, Won, Lost };
inline constexpr Color Ink{19, 29, 35, 255}, Cream{245, 235, 207, 255};
struct Button {
    int id;
    Rectangle rect;
    std::string label;
    bool enabled = true;
};
inline Vector2 Origin() {
    return {(Display::GetWidth() - 1280) / 2, (Display::GetHeight() - 720) / 2};
}
inline Vector2 Local(Vector2 p) {
    auto o = Origin();
    return {p.x - o.x, p.y - o.y};
}
class Game : public GameEngine {
public:
    Screen screen = Screen::Menu;
    int level = 0, unlocked = 1;
    std::array<int, 3> best{};
    bool muted = false;
    std::filesystem::path data;
    std::string id, title, tagline, hint;
    std::array<std::string, 3> levelNames;
    std::vector<Button> buttons;
    TouchTracker tracker;
    int capture = -1, finger = -1;
    bool mouseDown = false;
    Vector2 aim{640, 280};
    std::string storageError;

    Game(std::string name, std::string heading, std::string subtitle,
         std::array<std::string, 3> stages, std::filesystem::path directory)
        : GameEngine(heading, 1280, 720), data(std::move(directory)), id(std::move(name)),
          title(std::move(heading)), tagline(std::move(subtitle)), levelNames(std::move(stages)) {
        SetDefaultShortcutsEnabled(false);
        SetCanvasScaleMode(CanvasScaleMode::Expand);
        SetClearColor(Ink);
    }
    virtual void StartLevel(int selected) = 0;
    virtual void PlayClick(Vector2 point) = 0;
    virtual void PlayCommand(int command) = 0;
    virtual void ExtraButtons() = 0;
    virtual void DrawHUD() = 0;
    virtual void DrawHelp() = 0;
    virtual int Score() const = 0;
    virtual void Advance(float dt) = 0;
    virtual int Outcome() const = 0; // 0 playing, 1 won, -1 lost

    std::string Asset(const std::string& file) const {
#if defined(__ANDROID__)
        return file;
#else
        return std::string(GetApplicationDirectory()) + "assets/" + id + "/" + file;
#endif
    }
    void Sound(const std::string& sound) {
        if (!muted)
            AudioManager::PlaySound(id + sound, 0.32f);
    }
    void Save() {
        SaveSystem save;
        save.SetInt("unlocked", unlocked);
        save.SetBool("muted", muted);
        for (int i = 0; i < 3; ++i)
            save.SetInt("best" + std::to_string(i), best[i]);
        storageError =
            save.Save(data / "campaign.save") ? "" : "Could not save progress. See game.log.";
    }
    void OnInit() override {
        ExampleUI::Initialize({172, 221, 123, 255}, id);
        for (const auto& name : {"click", "shot", "clear", "hurt", "win"})
            AudioManager::LoadSound(id + name, Asset(std::string(name) + ".wav"));
        SaveSystem save;
        if (save.Load(data / "campaign.save")) {
            unlocked = std::clamp(save.GetInt("unlocked", 1), 1, 3);
            muted = save.GetBool("muted");
            for (int i = 0; i < 3; ++i)
                best[i] = std::max(0, save.GetInt("best" + std::to_string(i)));
        }
        StartLevel(0);
        SetScreen(Screen::Menu);
    }
    void SetScreen(Screen next) {
        screen = next;
        SetPaused(next != Screen::Play);
        capture = finger = -1;
        mouseDown = false;
        tracker.Reset(Input::GetFrame());
    }
    void Begin(int selected) {
        level = std::clamp(selected, 0, 2);
        StartLevel(level);
        SetScreen(Screen::Play);
    }
    void Add(int idValue, Rectangle rect, std::string label, bool enabled = true) {
        buttons.push_back({idValue, rect, std::move(label), enabled});
    }
    void BuildButtons() {
        buttons.clear();
        if (screen == Screen::Play) {
            Add(10, {1040, 28, 100, 42}, "PAUSE");
            Add(11, {1152, 28, 100, 42}, muted ? "SOUND OFF" : "SOUND ON");
            ExtraButtons();
            return;
        }
        if (screen == Screen::Menu) {
            Add(1, {462, 352, 356, 54}, unlocked == 1 ? "START ADVENTURE" : "CONTINUE ADVENTURE");
            Add(2, {462, 420, 356, 50}, "CHOOSE A LEVEL");
            Add(3, {462, 484, 356, 50}, "HOW TO PLAY");
            Add(11, {462, 548, 170, 46}, muted ? "SOUND OFF" : "SOUND ON");
            Add(4, {646, 548, 172, 46}, "EXIT");
        } else if (screen == Screen::Levels) {
            for (int i = 0; i < 3; ++i)
                Add(20 + i, {382, 290.0f + i * 82, 516, 64},
                    std::to_string(i + 1) + "  " + levelNames[i] +
                        (i < unlocked ? "" : "   / LOCKED"),
                    i < unlocked);
            Add(5, {490, 560, 300, 48}, "BACK");
        } else if (screen == Screen::Help)
            Add(5, {490, 605, 300, 48}, "BACK");
        else {
            Add(6, {440, 392, 400, 52},
                screen == Screen::Pause              ? "RESUME"
                : screen == Screen::Won && level < 2 ? "NEXT LEVEL"
                                                     : "PLAY AGAIN");
            Add(7, {440, 458, 194, 48}, "RESTART");
            Add(5, {646, 458, 194, 48}, "MAIN MENU");
        }
    }
    int Hit(Vector2 p) const {
        for (const auto& b : buttons)
            if (b.enabled && CheckCollisionPointRec(p, b.rect))
                return b.id;
        return screen == Screen::Play && CheckCollisionPointRec(p, {28, 90, 1224, 590}) ? 100 : -1;
    }
    void Activate(int action, Vector2 point) {
        if (action < 0)
            return;
        if (action == 100) {
            PlayClick(point);
            return;
        }
        Sound("click");
        if (action == 1)
            Begin(unlocked - 1);
        else if (action == 2)
            SetScreen(Screen::Levels);
        else if (action == 3)
            SetScreen(Screen::Help);
        else if (action == 4) {
            Save();
            Quit();
        } else if (action == 5) {
            Save();
            SetScreen(Screen::Menu);
        } else if (action == 6) {
            if (screen == Screen::Pause)
                SetScreen(Screen::Play);
            else
                Begin(screen == Screen::Won && level < 2 ? level + 1 : level);
        } else if (action == 7)
            Begin(level);
        else if (action == 10)
            SetScreen(Screen::Pause);
        else if (action == 11) {
            muted = !muted;
            Save();
        } else if (action >= 20 && action < 23 && action - 20 < unlocked)
            Begin(action - 20);
        else
            PlayCommand(action);
    }
    void OnUnscaledUpdate() override {
        BuildButtons();
        const auto& frame = Input::GetFrame();
        tracker.Update(frame, Display::GetViewport());
        if (tracker.Cancelled()) {
            capture = finger = -1;
            mouseDown = false;
        }
        if (!frame.focused) {
            if (screen == Screen::Play)
                SetScreen(Screen::Pause);
            return;
        }
        if (Input::GetKeyDown(KEY_F11))
            ToggleFullscreen();
        if (Input::GetKeyDown(KEY_P) && (screen == Screen::Play || screen == Screen::Pause)) {
            SetScreen(screen == Screen::Play ? Screen::Pause : Screen::Play);
            return;
        }
        if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK)) {
            if (screen == Screen::Play)
                SetScreen(Screen::Pause);
            else if (screen == Screen::Pause)
                SetScreen(Screen::Play);
            else if (screen != Screen::Menu)
                SetScreen(Screen::Menu);
            else {
                Save();
                Quit();
            }
            return;
        }
        if (screen == Screen::Play) {
            if (Input::GetKeyDown(KEY_SPACE) || Input::GetMouseButtonDown(MOUSE_BUTTON_RIGHT))
                PlayCommand(30);
            for (int i = 0; i < 7; ++i)
                if (Input::GetKeyDown(KEY_ONE + i))
                    PlayCommand(40 + i);
        }
        if ((screen == Screen::Play || screen == Screen::Won || screen == Screen::Lost) &&
            Input::GetKeyDown(KEY_R)) {
            Begin(level);
            return;
        }
        if (finger < 0 && !mouseDown)
            for (const auto& c : tracker.Contacts())
                if (c.Active() && c.pressed) {
                    finger = c.id;
                    aim = Local(c.position);
                    capture = Hit(aim);
                    break;
                }
        for (const auto& c : tracker.Contacts())
            if (c.id == finger && c.Active())
                aim = Local(c.position);
        for (const auto& c : tracker.Released())
            if (c.Active() && c.id == finger) {
                auto p = Local(c.position);
                int action = capture;
                capture = finger = -1;
                if (action == Hit(p))
                    Activate(action, p);
                return;
            }
        if (finger >= 0 && !tracker.IsActive(finger))
            capture = finger = -1;
#if !defined(__ANDROID__)
        if (frame.touchCount == 0 && finger < 0) {
            aim = Local(Input::GetMousePosition());
            if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                capture = Input::IsMouseInCanvas() ? Hit(aim) : -1;
                mouseDown = true;
            }
            if (Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT)) {
                int action = capture;
                capture = -1;
                mouseDown = false;
                if (Input::IsMouseInCanvas() && action == Hit(aim))
                    Activate(action, aim);
            }
        }
#endif
    }
    // A real Component supplies the fixed-step simulation and renderer in each game scene.
    void FixedStep(float dt) {
        if (screen != Screen::Play)
            return;
        Advance(dt);
        int outcome = Outcome();
        if (outcome) {
            if (outcome > 0) {
                best[level] = std::max(best[level], Score());
                unlocked = std::max(unlocked, std::min(3, level + 2));
                Sound("win");
            } else
                Sound("hurt");
            Save();
            SetScreen(outcome > 0 ? Screen::Won : Screen::Lost);
        }
    }
    void OnRender() override {
        auto o = Origin();
        BeginMode2D({{}, o, 0, 1});
        if (screen == Screen::Play)
            DrawHUD();
        else {
            DrawRectangle(0, 0, 1280, 720, {10, 18, 24, 205});
            DrawRectangleRounded({330, 148, 620, 516}, 0.07f, 10, {29, 43, 44, 248});
            DrawRectangleRoundedLines({330, 148, 620, 516}, 0.07f, 10, {112, 142, 112, 220});
            if (screen == Screen::Menu) {
                UI::LabelCentered(640, 183, "RAYGAME  /  ARCADE ADVENTURE", 18,
                                  {186, 199, 167, 255});
                UI::TitleCentered(640, 218, title, 66, Cream);
                UI::LabelCentered(640, 302, tagline, 22, Cream);
            } else if (screen == Screen::Levels) {
                UI::TitleCentered(640, 194, "CHOOSE A LEVEL", 46, Cream);
                UI::LabelCentered(640, 248, "Clear each stage to unlock the next.", 20, Cream);
            } else if (screen == Screen::Help) {
                UI::TitleCentered(640, 185, "HOW TO PLAY", 46, Cream);
                DrawHelp();
            } else {
                UI::LabelCentered(640, 198, levelNames[level], 24, Cream);
                UI::TitleCentered(640, 241,
                                  screen == Screen::Pause ? "PAUSED"
                                  : screen == Screen::Won
                                      ? (level == 2 ? "ADVENTURE COMPLETE" : "LEVEL CLEARED")
                                      : "TRY AGAIN",
                                  48, screen == Screen::Lost ? Color{245, 153, 122, 255} : Cream);
                UI::LabelCentered(640, 315,
                                  TextFormat("SCORE  %05d     BEST  %05d", Score(), best[level]),
                                  26, Cream);
                UI::LabelCentered(640, 353,
                                  screen == Screen::Lost ? hint
                                  : screen == Screen::Won
                                      ? "Progress saved. Your next challenge awaits."
                                      : "Take a break. The world is waiting.",
                                  18, Cream);
            }
        }
        BuildButtons();
        for (const auto& b : buttons) {
            bool hover = b.enabled && CheckCollisionPointRec(aim, b.rect);
            DrawRectangleRounded(b.rect, 0.18f, 8,
                                 !b.enabled ? Color{40, 48, 46, 255}
                                 : hover    ? Color{107, 139, 78, 255}
                                            : Color{67, 100, 66, 255});
            DrawRectangleRoundedLines(b.rect, 0.18f, 8, {140, 163, 109, 180});
            UI::LabelCentered(b.rect.x + b.rect.width / 2, b.rect.y + (b.rect.height - 20) / 2,
                              b.label, b.rect.width < 135 ? 15 : 20,
                              b.enabled ? Cream : Color{128, 138, 121, 255});
        }
        UI::Label(30, 691,
                  storageError.empty() ? "R  restart     P / Back  pause     F11  fullscreen"
                                       : storageError,
                  16, Cream);
        EndMode2D();
    }
    void Step(InputFrame input = {}, float dt = 1.0f / 60) {
        Display::Refresh();
        Input::SetFrame(input);
        Tick(dt);
        Display::Begin();
        ClearBackground(Ink);
        if (GetCurrentScene())
            GetCurrentScene()->Render();
        OnRender();
        Display::End();
    }
    void Click(Vector2 local, bool touch = false) {
        auto o = Origin();
        Vector2 p = Display::CanvasToScreen({local.x + o.x, local.y + o.y});
        InputFrame f;
        if (touch) {
            f.touchCount = 1;
            f.touches[0] = {9, p};
        } else {
            f.mousePosition = p;
            f.mouse.held[0] = f.mouse.pressed[0] = true;
        }
        Step(f);
        f = {};
        f.mousePosition = p;
        if (!touch)
            f.mouse.released[0] = true;
        Step(f);
        Step();
    }
    void Capture(const char* name) {
        auto image = LoadImageFromTexture(Display::GetCanvasTexture());
        ImageFlipVertical(&image);
        bool ok = ExportImage(image, (data / (std::string(name) + ".png")).u8string().c_str());
        UnloadImage(image);
        if (!ok)
            throw std::runtime_error("Screenshot export failed");
    }
};

template <class T> int Run(int argc, char** argv) {
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
    int result = 0;
    try {
        if (data.empty())
            data = FileSystem::UserDataDirectory(T::AppId);
        std::filesystem::create_directories(data);
        Diagnostics::Initialize(data / "logs/game.log", false);
        Diagnostics::InstallRaylibCallback();
        T game(data);
        if (smoke)
            result = game.Smoke();
        else
            game.Run();
    } catch (const std::exception& e) {
        Diagnostics::Write(LogLevel::Error, e.what());
        result = 1;
    }
    Diagnostics::Shutdown();
    return result;
}
} // namespace Campaign
