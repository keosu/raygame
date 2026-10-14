#include "Art.h"
#include "Assets.h"
#include "Battle.h"
#include "Draw.h"
#include "Game.h"
#include "Menu.h"
#include <cstring>

namespace UG {

namespace {
class UltraGame;
UltraGame* game = nullptr;
std::string captureDir;

class UltraGame : public GameEngine {
public:
    UltraGame() : GameEngine("奥特曼 · 光之守护者", kCanvasW, kCanvasH) {
        SetClearColor({6, 8, 20, 255});
        SetDefaultShortcutsEnabled(false);
    }

protected:
    void OnInit() override {
        Draw::Init();
        Fonts::Load();
        Sfx::Load();
        Art::BuildPortraits();
#if !defined(__ANDROID__) // Android uses the launcher icon from the manifest
        Image icon = LoadImageFromTexture(Art::Portrait(HeroId::Ultraman));
        if (IsImageValid(icon)) {
            ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            ImageResize(&icon, 64, 64);
            SetWindowIcon(icon);
        }
        UnloadImage(icon);
#endif
        if (Progress::Options().fullscreen && captureDir.empty()) SetFullscreen(true);
        if (!captureDir.empty()) ExportPortraits();
        LoadScene(CreateRef<MenuScene>(MenuPage::Title));
    }

    void OnUnscaledUpdate() override {
        if (Input::GetKeyDown(KEY_F11) ||
            (Input::GetKeyDown(KEY_ENTER) && (Input::GetKey(KEY_LEFT_ALT) || Input::GetKey(KEY_RIGHT_ALT))))
            ToggleFullscreenMode();
        if (Input::GetKeyDown(KEY_F1)) ToggleDebug();
        if (!captureDir.empty()) Capture();
    }

private:
    int frame = 0;

    void ExportPortraits() {
        MakeDirectory((captureDir + "/portraits").c_str());
        for (int i = 0; i < kHeroCount; ++i) {
            Image image = LoadImageFromTexture(Art::Portrait((HeroId)i));
            ExportImage(image, (captureDir + "/portraits/" + HeroInfo((HeroId)i).key + ".png").c_str());
            UnloadImage(image);
        }
    }

    void Shot(const char* name) {
        Image image = LoadImageFromTexture(Display::GetCanvasTexture());
        ImageFlipVertical(&image);
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8); // the window shows the canvas opaque
        ImageResize(&image, kCanvasW, kCanvasH);
        ExportImage(image, (captureDir + "/" + name + ".png").c_str());
        UnloadImage(image);
    }

    // Scripted tour used to verify rendering without manual play.
    void Capture() {
        ++frame;
        switch (frame) {
        case 50: Shot("01_title"); GoToMenu(MenuPage::Levels); break;
        case 90: Shot("02_levels"); GoToMenu(MenuPage::Heroes); break;
        case 130: Shot("03_heroes"); GoToMenu(MenuPage::Kaiju); break;
        case 170: Shot("04_kaiju"); GoToMenu(MenuPage::Help); break;
        case 200: Shot("05_help"); GoToBattle(0); break;
        case 240: Shot("06_battle_start"); Debug()->DebugAutoplay(); break;
        case 330: Shot("07_combat_city"); GoToBattle(1); break;
        case 370: Debug()->DebugAutoplay(3); break;
        case 490: Shot("08_combat_mountain"); GoToBattle(2); break;
        case 530: Debug()->DebugAutoplay(4); break;
        case 720: Shot("09_boss_nebula"); Debug()->DebugCast(); break;
        case 732: Shot("10_brothers_beam"); break;
        case 770: Shot("11_airstrike"); GoToBattle(0); break;
        case 810: Debug()->DebugFinish(); break;
        case 950: Shot("12_victory"); GoToBattle(1); break;
        case 990: Debug()->DebugAutoplay(0); break;
        case 1100: Debug()->DebugInspect(false); break;
        case 1104: Shot("13_inspect"); Debug()->DebugInspect(true); break;
        case 1110: Shot("14_pause"); Quit(); break;
        default: break;
        }
    }

    BattleScene* Debug() { return dynamic_cast<BattleScene*>(GetCurrentScene().get()); }
};

void ConfigureLogging() {
#ifdef RAYGAME_QUIET_EXAMPLE
    constexpr bool console = false;
#else
    constexpr bool console = true;
#endif
    try {
        Diagnostics::Initialize(FileSystem::UserDataDirectory("raygame.ultra_guard") / "logs/game.log", console);
        Diagnostics::InstallRaylibCallback();
    } catch (...) {
        if (!console) SetTraceLogLevel(LOG_NONE);
    }
}
} // namespace

void GoToMenu(MenuPage page) {
    if (game) game->LoadScene(CreateRef<MenuScene>(page));
}

void GoToBattle(int level) {
    if (game) game->LoadScene(CreateRef<BattleScene>(level));
}

void ToggleFullscreenMode() {
    if (!game) return;
    game->ToggleFullscreen();
    Progress::Options().fullscreen = game->IsFullscreen();
    Progress::Save();
}

void SetGameSpeed(float scale) {
    if (game) game->SetTimeScale(scale);
}

void QuitGame() {
    if (game) game->Quit();
}

int RunBalanceReport(const std::string& path);

} // namespace UG

int main(int argc, char** argv) {
    using namespace UG;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--capture") && i + 1 < argc) captureDir = argv[++i];
        if (!std::strcmp(argv[i], "--balance") && i + 1 < argc) return RunBalanceReport(argv[++i]);
    }
    ConfigureLogging();
    Progress::Load();
    int result = 0;
    try {
        UltraGame instance;
        game = &instance;
        instance.SetCanvasScaleMode(CanvasScaleMode::Fit);
        instance.Run();
    } catch (const std::exception& error) {
        Diagnostics::Write(LogLevel::Error, error.what());
        result = 1;
    }
    game = nullptr;
    Diagnostics::Shutdown();
    return result;
}
