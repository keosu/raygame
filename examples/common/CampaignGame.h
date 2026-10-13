#pragma once
#include "ExampleUI.h"
#include "CampaignText.h"
#include <fstream>

// Shared shell for the two campaign examples. Game rules remain in their own models.
namespace Campaign {
using namespace RayGame;
using ExampleUI::Triangle;
using UI = TextUI;
enum class Screen { Menu, Levels, Help, Settings, Play, Pause, Won, Lost };
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
    bool chinese = true, smooth = true, fullscreen = false;
    float volume = 0.6f;
    int fps = 60, menuFocus = 1;
    Screen settingsReturn = Screen::Menu;
    std::filesystem::path data;
    std::string id, title, tagline, hint;
    std::array<std::string, 3> levelNames;
    std::vector<Button> buttons;
    int capture = -1;
    PointerGesture pointer;
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
    virtual void PointerBegin(int, Vector2) {}
    virtual void PointerMove(Vector2) {}
    virtual bool PointerEnd(int, Vector2, bool) {
        return false;
    }
    virtual void PointerCancel() {}
    std::string Tr(const std::string& text) const {
        return UI::Tr(text);
    }
    float MenuScale() const {
        return screen == Screen::Play
                   ? 1
                   : std::min({1.5f, Display::GetWidth() / 860.0f, Display::GetHeight() / 740.0f});
    }
    Vector2 PointerLocal(Vector2 p) const {
        auto local = Local(p);
        float scale = MenuScale();
        return {640 + (local.x - 640) / scale, 360 + (local.y - 360) / scale};
    }
    void ApplySettings() {
        UI::chinese = chinese;
        Display::SetQuality(smooth ? CanvasQuality::Smooth : CanvasQuality::Standard);
        SetTargetFPS(fps);
#if !defined(__ANDROID__)
        SetFullscreen(fullscreen);
        SetWindowTitle(Tr(title).c_str());
#endif
    }

    std::string Asset(const std::string& file) const {
#if defined(__ANDROID__)
        return file;
#else
        return std::string(GetApplicationDirectory()) + "assets/" + id + "/" + file;
#endif
    }
    void Sound(const std::string& sound) {
        if (!muted)
            AudioManager::PlaySound(id + sound, volume * 0.55f);
    }
    void Save() {
        SaveSystem save;
        save.SetInt("unlocked", unlocked);
        save.SetBool("muted", muted);
        save.SetString("language", chinese ? "zh-CN" : "en");
        save.SetFloat("volume", volume);
        save.SetBool("smooth", smooth);
        save.SetBool("fullscreen", fullscreen);
        save.SetInt("fps", fps);
        for (int i = 0; i < 3; ++i)
            save.SetInt("best" + std::to_string(i), best[i]);
        storageError =
            save.Save(data / "campaign.save") ? "" : "Could not save progress. See game.log.";
    }
    void OnInit() override {
        ExampleUI::Initialize({172, 221, 123, 255}, id);
        UI::Load(Asset("locales/zh-CN.tsv"), Asset("fonts/"), id);
        for (const auto& name : {"click", "shot", "clear", "hurt", "win"})
            AudioManager::LoadSound(id + name, Asset(std::string(name) + ".wav"));
        SaveSystem save;
        if (save.Load(data / "campaign.save")) {
            unlocked = std::clamp(save.GetInt("unlocked", 1), 1, 3);
            muted = save.GetBool("muted");
            chinese = save.GetString("language", "zh-CN") != "en";
            volume = std::clamp(save.GetFloat("volume", 0.6f), 0.0f, 1.0f);
            smooth = save.GetBool("smooth", true);
            fullscreen = save.GetBool("fullscreen", false);
            fps = save.GetInt("fps", 60) == 120 ? 120 : 60;
            for (int i = 0; i < 3; ++i)
                best[i] = std::max(0, save.GetInt("best" + std::to_string(i)));
        }
        ApplySettings();
        StartLevel(0);
        SetScreen(Screen::Menu);
    }
    void SetScreen(Screen next) {
        screen = next;
        SetPaused(next != Screen::Play);
        capture = -1;
        pointer.Reset(Input::GetFrame());
        PointerCancel();
        menuFocus = 1;
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
            Add(10, {1040, 20, 100, 60}, "PAUSE");
            Add(11, {1152, 20, 100, 60}, muted ? "SOUND OFF" : "SOUND ON");
            ExtraButtons();
            return;
        }
        if (screen == Screen::Menu) {
            Add(1, {462, 352, 356, 54}, unlocked == 1 ? "START ADVENTURE" : "CONTINUE ADVENTURE");
            Add(2, {462, 420, 356, 50}, "CHOOSE A LEVEL");
            Add(3, {462, 484, 356, 50}, "HOW TO PLAY");
            Add(12, {462, 548, 170, 46}, "SETTINGS");
            Add(4, {646, 548, 172, 46}, "EXIT");
        } else if (screen == Screen::Levels) {
            for (int i = 0; i < 3; ++i)
                Add(20 + i, {382, 290.0f + i * 82, 516, 64},
                    std::to_string(i + 1) + "  " + Tr(levelNames[i]) +
                        (i < unlocked ? "" : "   / " + Tr("LOCKED")),
                    i < unlocked);
            Add(5, {490, 560, 300, 48}, "BACK");
        } else if (screen == Screen::Help)
            Add(5, {490, 605, 300, 48}, "BACK");
        else if (screen == Screen::Settings) {
            Add(13, {382, 270, 516, 54},
                Tr("LANGUAGE") + "    " + (chinese ? "简体中文" : "English"));
            Add(11, {382, 334, 516, 54}, Tr("SOUND") + "    " + Tr(muted ? "OFF" : "ON"));
            Add(14, {382, 398, 250, 54},
                Tr("VOLUME -") + "    " + std::to_string(static_cast<int>(volume * 100)) + "%");
            Add(15, {648, 398, 250, 54}, "VOLUME +");
            Add(16, {382, 462, 250, 54},
                Tr("QUALITY") + "    " + Tr(smooth ? "SMOOTH" : "STANDARD"));
            Add(17, {648, 462, 250, 54}, std::to_string(fps) + " FPS");
#if !defined(__ANDROID__)
            Add(18, {382, 526, 516, 54},
                Tr("DISPLAY") + "    " + Tr(fullscreen ? "FULLSCREEN" : "WINDOWED"));
#endif
            Add(5, {490, 600, 300, 48}, "SAVE & RETURN");
        } else {
            Add(6, {440, 392, 400, 52},
                screen == Screen::Pause              ? "RESUME"
                : screen == Screen::Won && level < 2 ? "NEXT LEVEL"
                                                     : "PLAY AGAIN");
            Add(7, {440, 458, 194, 48}, "RESTART");
            Add(5, {646, 458, 194, 48}, "MAIN MENU");
            if (screen == Screen::Pause)
                Add(12, {440, 524, 400, 48}, "SETTINGS");
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
            SetScreen(screen == Screen::Settings ? settingsReturn : Screen::Menu);
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
        } else if (action == 12) {
            settingsReturn = screen;
            SetScreen(Screen::Settings);
        } else if (action >= 13 && action <= 18) {
            if (action == 13)
                chinese = !chinese;
            if (action == 14)
                volume = std::max(0.0f, volume - 0.1f);
            if (action == 15)
                volume = std::min(1.0f, volume + 0.1f);
            if (action == 16)
                smooth = !smooth;
            if (action == 17)
                fps = fps == 60 ? 120 : 60;
            if (action == 18)
                fullscreen = !fullscreen;
            ApplySettings();
            Save();
        } else if (action >= 20 && action < 23 && action - 20 < unlocked)
            Begin(action - 20);
        else
            PlayCommand(action);
    }
    void OnUnscaledUpdate() override {
        BuildButtons();
        const auto& frame = Input::GetFrame();
        if (!frame.focused) {
            pointer.Reset(frame);
            PointerCancel();
            capture = -1;
            if (screen == Screen::Play)
                SetScreen(Screen::Pause);
            return;
        }
        if (Input::GetKeyDown(KEY_F11)) {
            ToggleFullscreen();
            fullscreen = IsFullscreen();
            Save();
        }
        if (Input::GetKeyDown(KEY_P) && (screen == Screen::Play || screen == Screen::Pause)) {
            SetScreen(screen == Screen::Play ? Screen::Pause : Screen::Play);
            return;
        }
        if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK)) {
            if (screen == Screen::Play)
                SetScreen(Screen::Pause);
            else if (screen == Screen::Pause)
                SetScreen(Screen::Play);
            else if (screen != Screen::Menu) {
                Save();
                SetScreen(screen == Screen::Settings ? settingsReturn : Screen::Menu);
            } else {
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
        if (screen != Screen::Play) {
            std::vector<int> choices;
            for (const auto& b : buttons)
                if (b.enabled)
                    choices.push_back(b.id);
            auto found = std::find(choices.begin(), choices.end(), menuFocus);
            int index = found == choices.end() ? 0 : static_cast<int>(found - choices.begin());
            if (!choices.empty()) {
                if (found == choices.end())
                    menuFocus = choices.front();
                if (Input::GetKeyDown(KEY_DOWN) || Input::GetKeyDown(KEY_TAB))
                    menuFocus = choices[(index + 1) % choices.size()];
                if (Input::GetKeyDown(KEY_UP))
                    menuFocus = choices[(index + choices.size() - 1) % choices.size()];
                if (Input::GetKeyDown(KEY_ENTER)) {
                    Activate(menuFocus, aim);
                    return;
                }
            }
        }
        pointer.Update(frame, Display::GetViewport());
        if (pointer.cancelled) {
            capture = -1;
            PointerCancel();
        }
        if (pointer.held || pointer.released)
            aim = PointerLocal(pointer.position);
#if !defined(__ANDROID__)
        else if (frame.touchCount == 0)
            aim = PointerLocal(Input::GetMousePosition());
#endif
        if (pointer.pressed) {
            capture = Hit(aim);
            PointerBegin(capture, aim);
        }
        if (pointer.held)
            PointerMove(aim);
        if (pointer.released) {
            int action = capture;
            capture = -1;
            if (!PointerEnd(action, aim, pointer.dragging) && action == Hit(aim))
                Activate(action, aim);
        }
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
        float scale = MenuScale();
        if (screen != Screen::Play)
            DrawRectangleRec({0, 0, Display::GetWidth(), Display::GetHeight()}, {10, 18, 24, 205});
        rlPushMatrix();
        rlTranslatef(o.x + 640, o.y + 360, 0);
        rlScalef(scale, scale, 1);
        rlTranslatef(-640, -360, 0);
        if (screen == Screen::Play)
            DrawHUD();
        else {
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
            } else if (screen == Screen::Settings) {
                UI::TitleCentered(640, 185, "SETTINGS", 46, Cream);
                UI::LabelCentered(640, 242, "Changes apply immediately and are saved.", 18, Cream);
            } else if (screen == Screen::Help) {
                UI::TitleCentered(640, 185, "HOW TO PLAY", 46, Cream);
                DrawHelp();
                UI::LabelCentered(640, 655, "R  restart     P / Back  pause     F11  fullscreen",
                                  16, Cream);
            } else {
                UI::LabelCentered(640, 198, levelNames[level], 24, Cream);
                UI::TitleCentered(640, 241,
                                  screen == Screen::Pause ? "PAUSED"
                                  : screen == Screen::Won
                                      ? (level == 2 ? "ADVENTURE COMPLETE" : "LEVEL CLEARED")
                                      : "TRY AGAIN",
                                  48, screen == Screen::Lost ? Color{245, 153, 122, 255} : Cream);
                UI::LabelCentered(640, 315,
                                  Tr("SCORE") + "  " + std::to_string(Score()) + "     " +
                                      Tr("BEST") + "  " + std::to_string(best[level]),
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
            bool pressed = capture == b.id && pointer.held;
            ButtonPainter::Draw(
                b.rect, {hover, pressed, screen != Screen::Play && menuFocus == b.id, !b.enabled},
                {{40, 71, 55, 248}, {176, 215, 126, 255}, Cream});
            std::string label = Tr(b.label);
            int size = b.rect.width < 135 ? 16 : 20;
            while (size > 13 && RayGame::UI::TextWidth(label, size) > b.rect.width - 20)
                --size;
            UI::LabelCentered(b.rect.x + b.rect.width / 2,
                              b.rect.y + (b.rect.height - size) / 2 + (pressed ? 2 : 0), label,
                              size, b.enabled ? Cream : Color{128, 138, 121, 255});
        }
        if (!storageError.empty())
            UI::LabelCentered(640, 685, storageError, 16, Cream);
        rlPopMatrix();
    }
    void Step(InputFrame input = {}, float dt = 1.0f / 60) {
        Display::Refresh();
        Input::SetFrame(input);
        Tick(dt);
        Render();
    }
    void Click(Vector2 local, bool touch = false) {
        auto o = Origin();
        float scale = MenuScale();
        Vector2 p = Display::CanvasToScreen(
            {o.x + 640 + (local.x - 640) * scale, o.y + 360 + (local.y - 360) * scale});
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
    bool MenuPanelVisible() const {
        auto texture = Display::GetCanvasTexture();
        auto img = LoadImageFromTexture(texture);
        if (!IsImageValid(img))
            return false;
        ImageFlipVertical(&img);
        auto o = Origin();
        float scale = MenuScale();
        auto p = Vector2{o.x + 640 + (350 - 640) * scale, o.y + 360 + (400 - 360) * scale};
        auto c = GetImageColor(img, static_cast<int>(p.x / Display::GetWidth() * img.width),
                               static_cast<int>(p.y / Display::GetHeight() * img.height));
        UnloadImage(img);
        return c.r >= 25 && c.g >= 38 && c.b >= 38;
    }
    bool HudVisible() const {
        auto texture = Display::GetCanvasTexture();
        auto img = LoadImageFromTexture(texture);
        if (!IsImageValid(img))
            return false;
        ImageFlipVertical(&img);
        auto o = Origin();
        // Inspect the pause button near both vertical edges, away from its text and rim.
        bool visible = true;
        for (float y : {27.0f, 73.0f}) {
            auto c =
                GetImageColor(img, static_cast<int>((o.x + 1060) / Display::GetWidth() * img.width),
                              static_cast<int>((o.y + y) / Display::GetHeight() * img.height));
            visible &= c.r >= 30 && c.g >= 60 && c.b >= 45;
        }
        UnloadImage(img);
        return visible;
    }
    template <class Check> void CheckStartup(Check check) {
        InputFrame unfocused;
        unfocused.focused = false;
        Step(unfocused);
        check(screen == Screen::Menu && IsPaused(), "initial unfocused frame retains the menu");
        check(MenuPanelVisible(), "startup menu has no engine pause overlay");
        Step();
        Capture("menu");
    }
    template <class Check> void CheckPauseSettings(Check check) {
        Click({1090, 49}, true);
        check(screen == Screen::Pause, "HUD opens the custom pause page");
        Click({640, 548}, true);
        check(screen == Screen::Settings && settingsReturn == Screen::Pause,
              "pause opens settings without stacking menus");
        check(MenuPanelVisible(), "pause settings have no upper engine overlay");
        Capture("pause-settings");
        Click({640, 624}, true);
        check(screen == Screen::Pause, "settings return to the custom pause page");
        Click({640, 418}, true);
    }
    template <class Check> void CheckSettings(Check check) {
        Click({540, 570}, true);
        check(screen == Screen::Settings, "touch opens settings");
        check(MenuPanelVisible(), "settings draw above the background shade");
        Capture("settings-zh");
        Click({640, 295}, true);
        check(!chinese && Tr("SETTINGS") == "SETTINGS", "language switches to English immediately");
        Capture("settings-en");
        Click({640, 295});
        check(chinese && Tr("SETTINGS") == "设置", "language switches back without restarting");
        check(RayGame::UI::unicodeFont && RayGame::UI::unicodeHeadingFont,
              "Chinese body and title fonts loaded");
        float before = volume;
        Click({500, 423});
        check(volume < before, "volume setting applies");
        Click({760, 423});
        Click({500, 487});
        int standardWidth = Display::GetCanvasTexture().width;
        check(!smooth && Display::GetQuality() == CanvasQuality::Standard,
              "standard quality applies");
        Click({500, 487});
        check(smooth && Display::GetQuality() == CanvasQuality::Smooth,
              "smooth supersampling applies");
        check(Display::GetCanvasTexture().width > standardWidth,
              "smooth allocates more render samples than standard");
        Click({760, 487});
        check(fps == 120, "frame limit setting applies");
#if !defined(__ANDROID__)
        int previousWidth = ::GetScreenWidth(), previousHeight = ::GetScreenHeight();
        auto previousPosition = GetWindowPosition();
        Click({640, 553});
        check(fullscreen && IsFullscreen(), "settings enters fullscreen");
        check(MenuPanelVisible(), "fullscreen settings panel is fully visible");
        Capture("settings-fullscreen");
        Click({640, 553});
        check(!fullscreen && !IsFullscreen() && ::GetScreenWidth() == previousWidth &&
                  ::GetScreenHeight() == previousHeight,
              "settings restores window dimensions");
        SetWindowPosition(static_cast<int>(previousPosition.x),
                          static_cast<int>(previousPosition.y));
#endif
        Click({640, 624}, true);
        check(screen == Screen::Menu, "settings returns to menu");
        Capture("menu-zh");
        SetTargetFPS(0);
    }
    template <class Check> void CheckFullscreenHud(Check check) {
#if !defined(__ANDROID__)
        SetFullscreen(true);
        Step();
        Step();
        check(HudVisible(), "fullscreen shows the complete top HUD");
        Capture("play-fullscreen");
        Click({1090, 49}, true);
        check(screen == Screen::Pause, "fullscreen HUD touch position matches the button");
        Click({640, 418}, true);
        SetFullscreen(false);
        Step();
#endif
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
