#include "Art.h"
#include "Assets.h"
#include "Battle.h"
#include "Draw.h"
#include "Game.h"
#include "Menu.h"
#include "Skins.h"
#include <cstring>

namespace UG {

namespace {
class UltraGame;
UltraGame* game = nullptr;
std::string skinArg;   // --skin: use this skin folder without saving the choice
std::string exportDir; // --export-skin: write the built-in art as a skin template and quit

class UltraGame : public GameEngine {
public:
    UltraGame() : GameEngine("奥特曼 · 光之守护者", kCanvasW, kCanvasH) {
        SetClearColor({6, 8, 20, 255});
        SetDefaultShortcutsEnabled(false);
    }

protected:
    void OnInit() override {
        // Letterbox bars continue the scene instead of showing flat black.
        Display::SetMarginStyle(Display::MarginStyle::BlurredCanvas, 0.45f);
        Draw::Init();
        Fonts::Load();
        Sfx::Load();
        Skins::Load(exportDir.empty() ? skinArg : "-");
        if (!exportDir.empty()) {
            exportResult = Skins::ExportTemplate(exportDir) ? 0 : 1;
            return; // GameEngine::Run ignores Quit() during OnInit; quit on the first update
        }
#if !defined(__ANDROID__) // Android uses the launcher icon from the manifest
        Image icon = LoadImage(AssetPath("icon.png").c_str()); // tools/make_icon.py, from a real avatar
        if (IsImageValid(icon)) {
            ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            ImageResize(&icon, 64, 64);
            SetWindowIcon(icon);
        }
        UnloadImage(icon);
#endif
        if (Progress::Options().fullscreen && !Automation::Active()) SetFullscreen(true);
        if (Automation::Active()) {
            ExportPortraits();
            Automation::Every([this](int frame) { Capture(frame); });
        }
        LoadScene(CreateRef<MenuScene>(MenuPage::Title));
    }

    void OnUnscaledUpdate() override {
        if (!exportDir.empty()) return Quit();
        if (Input::GetKeyDown(KEY_F11) || (Input::GetKeyDown(KEY_ENTER) && (Input::GetKey(KEY_LEFT_ALT) || Input::GetKey(KEY_RIGHT_ALT))))
            ToggleFullscreenMode();
        if (Input::GetKeyDown(KEY_F1)) ToggleDebug();
    }

public:
    int exportResult = 0;

private:
    void ExportPortraits() {
        auto folder = Automation::Directory() / "portraits";
        std::filesystem::create_directories(folder);
        for (int i = 0; i < kHeroCount; ++i) {
            Image image = LoadImageFromTexture(Art::Portrait((HeroId)i));
            ExportImage(image, (folder / (std::string(HeroInfo((HeroId)i).key) + ".png")).u8string().c_str());
            UnloadImage(image);
        }
    }

    // Scripted tour used to verify rendering without manual play.
    void Capture(int frame) {
        int skin = Skins::Current(), skins = (int)Skins::All().size();
        switch (frame) {
        case 2:
            Skins::Select((skin + 1) % skins, false);
            GoToMenu(MenuPage::Title);
            break; // switch round trip
        case 4:
            Skins::Select((skin + skins - 1) % skins, false);
            GoToMenu(MenuPage::Title);
            break;
        case 50:
            Automation::SaveCanvas("01_title");
            GoToMenu(MenuPage::Levels);
            break;
        case 90:
            Automation::SaveCanvas("02_levels");
            GoToMenu(MenuPage::Heroes);
            break;
        case 130:
            Automation::SaveCanvas("03_heroes");
            GoToMenu(MenuPage::Kaiju);
            break;
        case 170:
            Automation::SaveCanvas("04_kaiju");
            GoToMenu(MenuPage::Help);
            break;
        case 200:
            Automation::SaveCanvas("05_help");
            GoToBattle(0);
            break;
        case 240:
            Automation::SaveCanvas("06_battle_start");
            if (auto* b = Debug()) b->DebugAutoplay();
            break;
        case 330:
            Automation::SaveCanvas("07_combat_city");
            GoToBattle(1);
            break;
        case 370:
            if (auto* b = Debug()) b->DebugAutoplay(3);
            break;
        case 490:
            Automation::SaveCanvas("08_combat_mountain");
            GoToBattle(2);
            break;
        case 530:
            if (auto* b = Debug()) b->DebugAutoplay(4);
            break;
        case 720:
            Automation::SaveCanvas("09_boss_nebula");
            if (auto* b = Debug()) b->DebugCast();
            break;
        case 732:
            Automation::SaveCanvas("10_brothers_beam");
            break;
        case 770:
            Automation::SaveCanvas("11_airstrike");
            GoToBattle(0);
            break;
        case 810:
            if (auto* b = Debug()) b->DebugFinish();
            break;
        case 950:
            Automation::SaveCanvas("12_victory");
            GoToBattle(1);
            break;
        case 990:
            if (auto* b = Debug()) b->DebugAutoplay(0);
            break;
        case 1100:
            if (auto* b = Debug()) b->DebugInspect(false);
            break;
        case 1104:
            Automation::SaveCanvas("13_inspect");
            if (auto* b = Debug()) b->DebugInspect(true);
            break;
        case 1110:
            Automation::SaveCanvas("14_pause");
            SetWindowSize(1100, 860);
            break;
        case 1125:
            Automation::SaveWindow("15_margins");
            GoToMenu(MenuPage::Levels);
            break;
        case 1165:
            Automation::SaveCanvas("16_levels");
            PreviewHero((int)HeroId::Tiga);
            GoToMenu(MenuPage::Heroes);
            break;
        case 1210:
            Automation::SaveCanvas("17_codex");
            GoToBattle(3);
            break;
        case 1250:
            if (auto* b = Debug()) b->DebugAutoplay();
            break;
        case 1360:
            Automation::SaveCanvas("18_park");
            GoToBattle(4);
            break;
        case 1400:
            if (auto* b = Debug()) b->DebugAutoplay();
            break;
        case 1510:
            Automation::SaveCanvas("19_plaza");
            GoToBattle(5);
            break;
        case 1550:
            if (auto* b = Debug()) b->DebugAutoplay();
            break;
        case 1680:
            Automation::SaveCanvas("20_studio");
            if (auto* b = Debug()) b->DebugCast();
            break;
        case 1745: // the fusion cut-in runs in real time: heroes slide in, the orb gathers, the pillar lands at 2.9 s
            Automation::SaveCanvas("21_fusion_cutin");
            break;
        case 1825:
            Automation::SaveCanvas("22_fusion_orb");
            break;
        case 1862:
            Automation::SaveCanvas("23_fusion_impact");
            PrepareLevel(6);
            GoToMenu(MenuPage::Prepare);
            break;
        case 1910:
            Automation::SaveCanvas("24_prepare");
            GoToBattle(6);
            break;
        case 1950:
            if (auto* b = Debug()) b->DebugAutoplay(5);
            break;
        case 2130:
            Automation::SaveCanvas("25_volcano_strikes");
            GoToBattle(9);
            break;
        case 2170:
            if (auto* b = Debug()) b->DebugAutoplay(4);
            break;
        case 2350:
            Automation::SaveCanvas("26_moon");
            GoToBattle(11);
            break;
        case 2390:
            if (auto* b = Debug()) b->DebugAutoplay(6);
            break;
        case 2570:
            Automation::SaveCanvas("27_land_of_light");
            GoToBattle(kSurvivalIndex);
            break;
        case 2610:
            if (auto* b = Debug()) b->DebugAutoplay(6);
            break;
        case 2790:
            Automation::SaveCanvas("28_survival");
            if (auto* b = Debug()) b->DebugFinish();
            break;
        case 2930:
            Automation::SaveCanvas("29_survival_result");
            GoToMenu(MenuPage::Prepare);
            break;
        case 2970: // database page: the preview plays inside the panel, a few seconds per hero
            Automation::SaveCanvas("30_prepare_pool");
            PreviewHero((int)HeroId::Taiga);
            GoToMenu(MenuPage::Heroes);
            break;
        default:
            break;
        }
        // The v2.0 heroes on the database page, 160 frames each so the preview fires a few times.
        static const HeroId kTour[] = {HeroId::Taiga, HeroId::X,     HeroId::Hikari, HeroId::Cosmos,
                                       HeroId::Victory, HeroId::Rosso, HeroId::Blu, HeroId::Omega};
        constexpr int kTourStart = 3130, kTourStep = 160, kTourCount = (int)std::size(kTour);
        if (frame >= kTourStart && (frame - kTourStart) % kTourStep == 0 && frame <= kTourStart + (kTourCount - 1) * kTourStep) {
            int i = (frame - kTourStart) / kTourStep;
            Automation::SaveCanvas(TextFormat("%d_preview_%s", 31 + i, HeroInfo(kTour[i]).key));
            if (i + 1 < kTourCount) {
                PreviewHero((int)kTour[i + 1]);
                GoToMenu(MenuPage::Heroes);
            } else {
                Quit();
            }
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
    bool survival = level == kSurvivalIndex && SurvivalLevel();
    if (!survival && (level < 0 || level >= (int)Levels().size())) return GoToMenu(MenuPage::Levels);
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
        if (!std::strcmp(argv[i], "--capture") && i + 1 < argc && !Automation::Start(std::filesystem::u8path(argv[++i]), {true}))
            return 1;
        if (!std::strcmp(argv[i], "--balance") && i + 1 < argc) return RunBalanceReport(argv[++i]);
        if (!std::strcmp(argv[i], "--skin") && i + 1 < argc) skinArg = argv[++i];
        if (!std::strcmp(argv[i], "--export-skin") && i + 1 < argc) exportDir = argv[++i];
    }
    ConfigureLogging();
    Progress::Load();
    int result = 0;
    try {
        UltraGame instance;
        game = &instance;
        instance.SetCanvasScaleMode(CanvasScaleMode::Fit);
        instance.Run();
        if (!exportDir.empty()) result = instance.exportResult;
    } catch (const std::exception& error) {
        Diagnostics::Write(LogLevel::Error, error.what());
        result = 1;
    }
    game = nullptr;
    Diagnostics::Shutdown();
    return result;
}
