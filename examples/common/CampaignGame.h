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
// Plain: secondary actions; Primary: the one obvious next step; Card: drawn by its screen.
enum class Look { Plain, Primary, Card };
struct Button {
    int id;
    Rectangle rect;
    std::string label;
    bool enabled = true;
    Look look = Look::Plain;
    ButtonIcon icon = ButtonIcon::None;
};
inline float EaseOut(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1 - (1 - t) * (1 - t) * (1 - t);
}
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
    Color accent{176, 215, 126, 255}; // the game's signature colour for primary actions and highlights
    float screenTime = 0, clock = 0;  // real seconds on this screen / since launch, for menu motion
    bool newRecord = false;

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
    // Something drawn above the HUD (a falling token, say) claims this press before any button.
    virtual bool PlayGrab(Vector2) { return false; }
    // Title-screen mark drawn around `center` within about 60 units.
    virtual void DrawEmblem(Vector2 center, float time) {
        DrawCircleV(center, 46, accent);
    }
    // Level-card artwork filling `area` (already clipped).
    virtual void DrawLevelArt(int stage, Rectangle area, float time) {
        DrawRectangleGradientV(static_cast<int>(area.x), static_cast<int>(area.y), static_cast<int>(area.width),
                               static_cast<int>(area.height), Fade(accent, 0.55f - stage * 0.12f), Ink);
        DrawEmblem({area.x + area.width / 2, area.y + area.height / 2}, time);
    }
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
        if (next != screen)
            screenTime = 0;
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
    void Add(int idValue, Rectangle rect, std::string label, bool enabled = true, Look look = Look::Plain,
             ButtonIcon icon = ButtonIcon::None) {
        buttons.push_back({idValue, rect, std::move(label), enabled, look, icon});
    }
    // Menus are laid out in the 1280 × 720 design box but keep to its central 860 units, the width
    // MenuScale guarantees on portrait screens.
    void BuildButtons() {
        buttons.clear();
        if (screen == Screen::Play) {
            Add(10, {1040, 20, 100, 60}, "PAUSE");
            Add(11, {1152, 20, 100, 60}, muted ? "SOUND OFF" : "SOUND ON");
            ExtraButtons();
            return;
        }
        if (screen == Screen::Menu) {
            Add(1, {230, 388, 380, 64}, unlocked == 1 ? "START ADVENTURE" : "CONTINUE ADVENTURE", true,
                Look::Primary, ButtonIcon::Right);
            Add(2, {230, 466, 380, 54}, "CHOOSE A LEVEL");
            Add(3, {230, 530, 380, 54}, "HOW TO PLAY");
            Add(12, {230, 594, 186, 50}, "SETTINGS", true, Look::Plain, ButtonIcon::Settings);
            Add(4, {424, 594, 186, 50}, "EXIT");
        } else if (screen == Screen::Levels) {
            for (int i = 0; i < 3; ++i)
                Add(20 + i, LevelCard(i), levelNames[i], i < unlocked, Look::Card);
            Add(5, {210, 588, 200, 52}, "BACK", true, Look::Plain, ButtonIcon::Left);
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
        } else if (screen == Screen::Pause) {
            Add(6, {460, 392, 360, 56}, "RESUME", true, Look::Primary, ButtonIcon::Right);
            Add(7, {460, 460, 174, 48}, "RESTART");
            Add(5, {646, 460, 174, 48}, "MAIN MENU");
            Add(12, {460, 520, 360, 46}, "SETTINGS", true, Look::Plain, ButtonIcon::Settings);
        } else {
            Add(6, {440, 466, 400, 58}, screen == Screen::Won && level < 2 ? "NEXT LEVEL" : "PLAY AGAIN", true,
                Look::Primary, ButtonIcon::Right);
            Add(7, {440, 536, 194, 48}, "RESTART");
            Add(5, {646, 536, 194, 48}, "MAIN MENU");
        }
    }
    static Rectangle LevelCard(int stage) {
        return {210.0f + stage * 295, 200, 270, 350};
    }
    int Hit(Vector2 p) {
        if (screen == Screen::Play && PlayGrab(p))
            return 100;
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
        float real = std::min(0.1f, GameTime::UnscaledDeltaTime());
        screenTime += real;
        clock += real;
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
            newRecord = outcome > 0 && Score() > best[level];
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
    // Each kind of screen has its own look: the title and level select own the whole screen,
    // settings and help are sheets, pause is a small card over the frozen game, results a banner.
    void OnRender() override {
        auto o = Origin();
        float scale = MenuScale();
        float width = Display::GetWidth(), height = Display::GetHeight();
        bool front = screen == Screen::Menu || screen == Screen::Levels;
        if (front) {
            // Darken toward the left where the title sits; the scene stays visible on the right.
            DrawRectangleGradientH(0, 0, static_cast<int>(width), static_cast<int>(height), {8, 14, 18, 238},
                                   {8, 14, 18, 120});
            DrawRectangleGradientV(0, static_cast<int>(height * 0.6f), static_cast<int>(width),
                                   static_cast<int>(height * 0.4f + 1), {8, 14, 18, 0}, {8, 14, 18, 200});
        } else if (screen == Screen::Pause)
            DrawRectangleRec({0, 0, width, height}, {6, 10, 14, 215});
        else if (screen != Screen::Play)
            DrawRectangleRec({0, 0, width, height}, {10, 18, 24, 205});
        rlPushMatrix();
        rlTranslatef(o.x + 640, o.y + 360, 0);
        rlScalef(scale, scale, 1);
        rlTranslatef(-640, -360, 0);
        if (front)
            DrawMotes();
        if (screen == Screen::Play)
            DrawHUD();
        else if (screen == Screen::Menu)
            DrawTitleScreen();
        else if (screen == Screen::Levels)
            DrawLevelSelect(o, scale);
        else if (screen == Screen::Settings) {
            DrawSheet("SETTINGS", ButtonIcon::Settings);
            UI::LabelCentered(640, 238, "Changes apply immediately and are saved.", 18, Fade(Cream, 0.8f));
        } else if (screen == Screen::Help) {
            DrawSheet("HOW TO PLAY", ButtonIcon::None);
            DrawHelp();
            UI::LabelCentered(640, 655, "R  restart     P / Back  pause     F11  fullscreen", 16, Fade(Cream, 0.7f));
        }
        float rise = screen == Screen::Pause || screen == Screen::Won || screen == Screen::Lost
                         ? 24 * (1 - EaseOut(screenTime / 0.3f))
                         : 0;
        rlPushMatrix();
        rlTranslatef(0, rise, 0);
        if (screen == Screen::Pause)
            DrawPauseCard();
        else if (screen == Screen::Won || screen == Screen::Lost)
            DrawResultCard();
        DrawButtons();
        rlPopMatrix();
        if (!storageError.empty())
            UI::LabelCentered(640, 685, storageError, 16, Cream);
        rlPopMatrix();
    }
    void DrawButtons() {
        BuildButtons();
        bool menu = screen == Screen::Menu;
        for (size_t i = 0; i < buttons.size(); ++i) {
            const auto& b = buttons[i];
            if (b.look == Look::Card)
                continue;
            bool hover = b.enabled && CheckCollisionPointRec(aim, b.rect);
            bool pressed = capture == b.id && pointer.held;
            bool focused = screen != Screen::Play && menuFocus == b.id;
            Rectangle r = b.rect;
            if (menu) // title buttons slide in one after another
                r.x -= 40 * (1 - EaseOut((screenTime - 0.15f - i * 0.06f) / 0.35f));
            bool primary = b.look == Look::Primary;
            ButtonStyle style = primary ? ButtonStyle{accent, Cream, Ink} : ButtonStyle{{40, 71, 55, 248}, accent, Cream};
            ButtonPainter::Draw(r, {hover, pressed, focused, !b.enabled}, style);
            if (menu && (hover || focused)) // a pointer beside the chosen title entry
                Triangle({r.x - 22, r.y + r.height / 2 - 9}, {r.x - 22, r.y + r.height / 2 + 9},
                         {r.x - 10, r.y + r.height / 2}, accent);
            Color ink = !b.enabled ? Color{128, 138, 121, 255} : primary ? Ink : Cream;
            std::string label = Tr(b.label);
            int size = primary && r.height >= 56 ? 24 : r.width < 135 ? 16 : 20;
            float room = r.width - 20 - (b.icon != ButtonIcon::None ? 34 : 0);
            while (size > 13 && RayGame::UI::TextWidth(label, size) > room)
                --size;
            float textWidth = RayGame::UI::TextWidth(label, size);
            float cx = r.x + r.width / 2, y = r.y + (r.height - size) / 2 + (pressed ? 2 : 0);
            if (b.icon != ButtonIcon::None) {
                bool trailing = b.icon == ButtonIcon::Right; // forward arrows follow the label
                cx += trailing ? -14 : 14;
                float iconX = trailing ? cx + textWidth / 2 + 20 : cx - textWidth / 2 - 20;
                ButtonPainter::Icon(b.icon, {iconX, r.y + r.height / 2 + (pressed ? 2 : 0)}, size * 0.9f, ink);
            }
            UI::LabelCentered(cx, y, label, size, ink);
        }
    }
    void DrawMotes() {
        for (int i = 0; i < 26; ++i) {
            float speed = 14 + (i * 7 % 11) * 3, x = 160.0f + (i * 151 % 980);
            float y = 760 - std::fmod(clock * speed + i * 97.0f, 820.0f);
            DrawCircleV({x + std::sin(clock * 0.8f + i) * 12, y}, 1.5f + (i % 3),
                        Fade(accent, 0.18f + 0.12f * std::sin(clock * 2 + i)));
        }
    }
    void DrawTitleScreen() {
        auto slide = [&](int order) { return -40 * (1 - EaseOut((screenTime - order * 0.07f) / 0.4f)); };
        UI::Label(232 + slide(0), 108, "RAYGAME  /  ARCADE ADVENTURE", 16, accent);
        DrawEmblem({292 + slide(1), 194 + std::sin(clock * 1.6f) * 5}, clock);
        float x = 230 + slide(2);
        UI::Title(x + 3, 259, title, 70, Fade(BLACK, 0.45f));
        UI::Title(x, 256, title, 70, Cream);
        UI::Label(234 + slide(3), 347, tagline, 21, Fade(Cream, 0.85f));
        // Progress at a glance: one medallion per stage.
        Rectangle panel{690, 470 - slide(4) * 0.5f, 360, 174};
        DrawRectangleRounded(panel, 0.12f, 8, {12, 22, 26, 200});
        DrawRectangleRoundedLines(panel, 0.12f, 8, Fade(accent, 0.35f));
        UI::Label(panel.x + 20, panel.y + 16, "ADVENTURE PROGRESS", 16, accent);
        for (int i = 0; i < 3; ++i) {
            Vector2 c{panel.x + 60 + i * 120, panel.y + 90};
            bool open = i < unlocked, cleared = best[i] > 0;
            DrawCircleV(c, 33, cleared ? accent : Color{36, 52, 54, 255});
            DrawRing(c, 31, 34, 0, 360, 48, open ? accent : Color{86, 98, 96, 255});
            if (open)
                UI::TitleCentered(c.x, c.y - 19, std::to_string(i + 1), 32, cleared ? Ink : Cream);
            else
                DrawPadlock(c, 1, {150, 160, 152, 255});
            UI::LabelCentered(c.x, c.y + 42,
                              cleared ? std::to_string(best[i]) : Tr(open ? "READY" : "LOCKED"), 15,
                              Fade(Cream, cleared ? 0.95f : 0.6f));
        }
#if !defined(__ANDROID__)
        UI::Label(232, 664, "UP / DOWN  SELECT     ENTER  CONFIRM", 14, Fade(Cream, 0.5f));
#endif
    }
    void DrawLevelSelect(Vector2 origin, float scale) {
        UI::Title(210, 98, "CHOOSE A LEVEL", 50, Cream);
        UI::Label(212, 160, "Clear each stage to unlock the next.", 19, Fade(Cream, 0.8f));
        for (int i = 0; i < 3; ++i) {
            Rectangle r = LevelCard(i);
            bool open = i < unlocked, cleared = best[i] > 0;
            bool lit = open && (menuFocus == 20 + i || CheckCollisionPointRec(aim, r));
            r.y += 30 * (1 - EaseOut((screenTime - i * 0.08f) / 0.4f)) - (lit ? 6 : 0);
            DrawRectangleRounded({r.x + 4, r.y + 10, r.width, r.height}, 0.06f, 8, Fade(BLACK, 0.35f));
            DrawRectangleRounded(r, 0.06f, 8, {24, 38, 40, 250});
            Rectangle art{r.x + 10, r.y + 10, r.width - 20, 180};
            {
                // Clip in canvas units: the menu space is translated and scaled.
                Rectangle canvas{origin.x + 640 + (art.x - 640) * scale, origin.y + 360 + (art.y - 360) * scale,
                                 art.width * scale, art.height * scale};
                Display::ClipScope clip(canvas);
                DrawLevelArt(i, art, clock);
                if (!open)
                    DrawRectangleRec(art, {10, 16, 20, 170});
            }
            if (!open)
                DrawPadlock({art.x + art.width / 2, art.y + art.height / 2}, 1.6f, Cream);
            DrawCircleV({art.x + 30, art.y + 30}, 22, open ? accent : Color{70, 80, 78, 255});
            UI::TitleCentered(art.x + 30, art.y + 13, std::to_string(i + 1), 28, Ink);
            UI::Title(r.x + 20, r.y + 204, levelNames[i], 25, open ? Cream : Color{150, 158, 150, 255});
            UI::Label(r.x + 20, r.y + 246,
                      cleared ? Tr("BEST") + "  " + std::to_string(best[i]) : Tr(open ? "Not played yet" : "LOCKED"),
                      17, Fade(Cream, 0.75f));
            const char* state = cleared ? "CLEARED" : open ? "READY" : "LOCKED";
            Color chip = cleared ? accent : open ? Color{226, 160, 82, 255} : Color{86, 96, 94, 255};
            std::string text = Tr(state);
            float w = RayGame::UI::TextWidth(text, 15) + 28;
            DrawRectangleRounded({r.x + 20, r.y + 298, w, 30}, 1, 12, chip);
            UI::Label(r.x + 34, r.y + 305, state, 15, Ink);
            if (lit)
                DrawRectangleRoundedLinesEx(r, 0.06f, 8, 3, accent);
        }
    }
    void DrawSheet(const char* heading, ButtonIcon icon) {
        Rectangle panel{330, 148, 620, 516};
        DrawRectangleRounded({panel.x + 4, panel.y + 10, panel.width, panel.height}, 0.07f, 10, Fade(BLACK, 0.3f));
        DrawRectangleRounded(panel, 0.07f, 10, {29, 43, 44, 250});
        DrawRectangleRounded({panel.x, panel.y, panel.width, 74}, 0.35f, 10, Fade(accent, 0.22f));
        DrawRectangle(static_cast<int>(panel.x), static_cast<int>(panel.y + 50), static_cast<int>(panel.width), 24,
                      {29, 43, 44, 250});
        DrawRectangle(static_cast<int>(panel.x + 24), static_cast<int>(panel.y + 72), static_cast<int>(panel.width - 48),
                      2, Fade(accent, 0.5f));
        if (icon != ButtonIcon::None)
            ButtonPainter::Icon(icon, {panel.x + 44, panel.y + 38}, 28, accent);
        UI::TitleCentered(640, panel.y + 14, heading, 42, Cream);
    }
    void DrawPauseCard() {
        Rectangle card{430, 170, 420, 410};
        DrawRectangleRounded({card.x + 4, card.y + 10, card.width, card.height}, 0.08f, 10, Fade(BLACK, 0.4f));
        DrawRectangleRounded(card, 0.08f, 10, {33, 49, 50, 252});
        DrawRectangleRounded({card.x + 150, card.y, 120, 5}, 1, 8, accent);
        Vector2 icon{640, 224};
        DrawCircleV(icon, 30, accent);
        DrawRectangleRounded({icon.x - 11, icon.y - 13, 8, 26}, 0.4f, 4, Ink);
        DrawRectangleRounded({icon.x + 3, icon.y - 13, 8, 26}, 0.4f, 4, Ink);
        UI::TitleCentered(640, 262, "PAUSED", 40, Cream);
        UI::LabelCentered(640, 318, levelNames[level], 18, Fade(Cream, 0.7f));
        UI::LabelCentered(640, 346,
                          Tr("SCORE") + "  " + std::to_string(Score()) + "     " + Tr("BEST") + "  " +
                              std::to_string(best[level]),
                          18, Cream);
    }
    void DrawResultCard() {
        bool won = screen == Screen::Won;
        Rectangle card{390, 130, 500, 480};
        DrawRectangleRounded({card.x + 4, card.y + 12, card.width, card.height}, 0.07f, 10, Fade(BLACK, 0.4f));
        DrawRectangleRounded(card, 0.07f, 10, {29, 43, 44, 252});
        Color ribbon = won ? Color{236, 186, 74, 255} : Color{190, 96, 82, 255};
        if (won) { // slowly turning rays behind the banner star
            Vector2 c{640, 136};
            for (int i = 0; i < 12; ++i) {
                float a = clock * 0.4f + i * PI / 6;
                Triangle(c, {c.x + std::cos(a - 0.09f) * 120, c.y + std::sin(a - 0.09f) * 120},
                         {c.x + std::cos(a + 0.09f) * 120, c.y + std::sin(a + 0.09f) * 120}, Fade(ribbon, 0.18f));
            }
            DrawStar(c, 34 * EaseOut(screenTime / 0.4f), ribbon);
        }
        // Banner across the top with notched tails.
        Rectangle band{360, 168, 560, 66};
        for (float side : {-1.0f, 1.0f}) {
            float x = side < 0 ? band.x : band.x + band.width;
            Triangle({x, band.y + 10}, {x + side * 34, band.y + 10}, {x + side * 20, band.y + 43}, Fade(ribbon, 0.75f));
            Triangle({x, band.y + 10}, {x + side * 20, band.y + 43}, {x, band.y + 76}, Fade(ribbon, 0.75f));
            Triangle({x, band.y + 76}, {x + side * 20, band.y + 43}, {x + side * 34, band.y + 76}, Fade(ribbon, 0.75f));
        }
        DrawRectangleRounded(band, 0.2f, 8, ribbon);
        UI::TitleCentered(640, band.y + 12,
                          won ? (level == 2 ? "ADVENTURE COMPLETE" : "LEVEL CLEARED") : "TRY AGAIN", 38, Ink);
        UI::LabelCentered(640, 250, levelNames[level], 20, Fade(Cream, 0.75f));
        int shown = static_cast<int>(std::round(Score() * EaseOut((screenTime - 0.15f) / 0.9f)));
        UI::LabelCentered(640, 284, "SCORE", 16, Fade(Cream, 0.6f));
        UI::TitleCentered(640, 304, std::to_string(shown), 54, won ? Color{255, 226, 140, 255} : Cream);
        UI::LabelCentered(640, 372, Tr("BEST") + "  " + std::to_string(best[level]), 17, Fade(Cream, 0.75f));
        if (newRecord && won && screenTime > 0.9f) {
            float pulse = 1 + 0.06f * std::sin(clock * 6);
            std::string text = Tr("NEW RECORD!");
            float w = (RayGame::UI::TextWidth(text, 16) + 30) * pulse, h = 28 * pulse;
            DrawRectangleRounded({640 - w / 2, 410 - h / 2, w, h}, 1, 12, ribbon);
            UI::LabelCentered(640, 402, "NEW RECORD!", 16, Ink);
        } else
            UI::LabelCentered(640, 402, won ? "Progress saved. Your next challenge awaits." : hint, 16,
                              Fade(Cream, 0.8f));
    }
    static void DrawPadlock(Vector2 c, float s, Color color) {
        DrawRing({c.x, c.y - 6 * s}, 7 * s, 10 * s, 180, 360, 24, color);
        DrawRectangleRounded({c.x - 12 * s, c.y - 6 * s, 24 * s, 19 * s}, 0.25f, 6, color);
        DrawCircleV({c.x, c.y + 2 * s}, 2.5f * s, Ink);
    }
    static void DrawStar(Vector2 c, float r, Color color) {
        for (int i = 0; i < 5; ++i) {
            float a = -PI / 2 + i * 2 * PI / 5, b = a + PI / 5, prev = a - PI / 5;
            Vector2 tip{c.x + std::cos(a) * r, c.y + std::sin(a) * r};
            Vector2 left{c.x + std::cos(prev) * r * 0.45f, c.y + std::sin(prev) * r * 0.45f};
            Vector2 right{c.x + std::cos(b) * r * 0.45f, c.y + std::sin(b) * r * 0.45f};
            Triangle(c, left, tip, color);
            Triangle(c, tip, right, color);
        }
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
    // Clicks the centre of the current screen's button `idValue`, enabled or not.
    void ClickButton(int idValue, bool touch = false) {
        BuildButtons();
        for (const auto& b : buttons)
            if (b.id == idValue) {
                Click({b.rect.x + b.rect.width / 2, b.rect.y + b.rect.height / 2}, touch);
                return;
            }
    }
    void Capture(const char* name) {
        // Menus are paused, so letting their entrance animation finish changes no game state.
        for (int i = 0; i < 90 && screen != Screen::Play && screenTime < 1.3f; ++i)
            Step();
        auto image = LoadImageFromTexture(Display::GetCanvasTexture());
        ImageFlipVertical(&image);
        bool ok = ExportImage(image, (data / (std::string(name) + ".png")).u8string().c_str());
        UnloadImage(image);
        if (!ok)
            throw std::runtime_error("Screenshot export failed");
    }
    // The first button of the menu is drawn at full brightness: nothing such as an engine pause
    // shade covers the menu, and it sits where its hit box is.
    bool MenuPanelVisible() {
        BuildButtons();
        if (buttons.empty())
            return false;
        auto texture = Display::GetCanvasTexture();
        auto img = LoadImageFromTexture(texture);
        if (!IsImageValid(img))
            return false;
        ImageFlipVertical(&img);
        auto o = Origin();
        float scale = MenuScale();
        auto r = buttons.front().rect;
        Vector2 local{r.x + 12, r.y + r.height * 0.75f}; // inside the face, clear of label and icon
        auto p = Vector2{o.x + 640 + (local.x - 640) * scale, o.y + 360 + (local.y - 360) * scale};
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
        ClickButton(12, true);
        check(screen == Screen::Settings && settingsReturn == Screen::Pause,
              "pause opens settings without stacking menus");
        check(MenuPanelVisible(), "pause settings have no upper engine overlay");
        Capture("pause-settings");
        Click({640, 624}, true);
        check(screen == Screen::Pause, "settings return to the custom pause page");
        ClickButton(6, true);
    }
    template <class Check> void CheckSettings(Check check) {
        ClickButton(12, true);
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
        ClickButton(6, true);
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
