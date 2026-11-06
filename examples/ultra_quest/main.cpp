#include "Assets.h"
#include "Draw.h"
#include "ExploreScene.h"
#include "FightScene.h"
#include "Game.h"
#include "Story.h"
#include "TransformScene.h"
#include <cstring>
#include <fstream>
#include <functional>

namespace UQ {

namespace {
class QuestGame;
QuestGame* game = nullptr;

class QuestGame : public GameEngine {
public:
    QuestGame() : GameEngine("奥特曼 · 泽塔传说", kCanvasW, kCanvasH) {
        SetClearColor({6, 8, 20, 255});
        SetDefaultShortcutsEnabled(false);
    }

protected:
    void OnInit() override {
        Display::SetMarginStyle(Display::MarginStyle::BlurredCanvas, 0.45f);
        Draw::Init();
        Fonts::Load();
        Sfx::Load();
        Poses::Load();
        Story::Load();
#if !defined(__ANDROID__) // Android uses the launcher icon from the manifest
        Image icon = LoadImage(AssetPath("icon.png").c_str());
        if (IsImageValid(icon)) {
            ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            ImageResize(&icon, 64, 64);
            SetWindowIcon(icon);
        }
        UnloadImage(icon);
#endif
        if (Progress::Options().fullscreen && !Automation::Active()) SetFullscreen(true);
        if (Automation::Active()) Automation::Every([this](int frame) { Capture(frame); });
        LoadScene(CreateRef<TitleScene>());
    }

    void OnUnscaledUpdate() override {
        if (Input::GetKeyDown(KEY_F11) || (Input::GetKeyDown(KEY_ENTER) && (Input::GetKey(KEY_LEFT_ALT) || Input::GetKey(KEY_RIGHT_ALT))))
            ToggleFullscreenMode();
    }

private:
    // Scripted tour for visual checks: each step waits for its condition, then acts.
    struct Step {
        std::function<bool()> ready;
        std::function<void()> act;
    };
    std::vector<Step> tour;
    size_t stepIndex = 0;
    int stepFrame = 0;

    template <class T> T* Current() { return dynamic_cast<T*>(GetCurrentScene().get()); }

    void BuildTour() {
        auto after = [this](int frames) { return [this, frames] { return stepFrame >= frames; }; };
        auto shot = [](const char* name) { return [name] { Automation::SaveCanvas(name); }; };
        auto fight = [this] { return Current<FightScene>(); };
        tour = {
            {after(40), shot("01_title")},
            {after(1), [this] { if (auto* t = Current<TitleScene>()) t->SetPage(TitleScene::Page::Help); }},
            {after(30), shot("02_help")},
            {after(1), [this] { if (auto* t = Current<TitleScene>()) t->SetPage(TitleScene::Page::Settings); }},
            {after(30), shot("03_settings")},
            {after(1), [] { GoToExplore(); }},
            {after(100), shot("04_explore_intro")},
            {after(1), [this] { if (auto* e = Current<ExploreScene>()) { e->DebugSkipDialogue(); e->DebugWalkTo(900); } }},
            {after(70), shot("05_street_crack")},
            {after(1), [this] { if (auto* e = Current<ExploreScene>()) e->DebugInteract("owner"); }},
            {after(110), shot("06_owner_dialogue")},
            {after(1), [this] { if (auto* e = Current<ExploreScene>()) { e->DebugSkipDialogue(); e->DebugFindAll(); e->DebugWalkTo(2240); } }},
            {after(70), shot("07_park_clues")},
            {after(1), [this] { if (auto* e = Current<ExploreScene>()) e->DebugReveal(); }},
            {after(70), shot("08_reveal_rise")},
            {after(110), shot("09_reveal_dialogue")},
            {after(1), [] { GoToTransform(); }},
            {after(62), shot("10_transform_medals")},
            {after(118), shot("11_transform_burst")},
            {after(1), [] { GoToFight(true); }},
            {after(56), shot("12_fight_landing")},
            {after(100), shot("13_fight_banner")},
            {after(1), [this, fight] { if (auto* f = fight()) { f->DebugSkipIntro(); f->DebugAutoplay(true, 0.45f); collecting = true; } }},
            {after(200), shot("14_fight_melee")},
            // Let the bot fight while the kaiju cycles its moves, until every action shot is taken.
            {[this] { return actionShots.size() >= kActions.size() || stepFrame > 1100; },
             [this, fight] { collecting = false; if (auto* f = fight()) { f->DebugAutoplay(false); f->Model().DebugSetHp(1000, 2500); } }},
            // Scripted moments: knocked down by a swing, an air kick, a wall crash.
            {after(20), [fight] { if (auto* f = fight()) { f->Model().DebugPlace(900, 1150, 0.1f); f->Model().DebugForceMove(KaijuMove::Swing); } }},
            {[fight] { auto* f = fight(); return f && f->Model().hero().state == HeroState::Down && f->Model().hero().stateTime > 0.3f; },
             shot("a7_hero_down")},
            {after(90), [fight] { if (auto* f = fight()) { f->Model().DebugPlace(900, 1250); FightInput in; in.jump = true; f->DebugInput(in); } }},
            {after(14), [fight] { if (auto* f = fight()) { FightInput in; in.attack = true; f->DebugInput(in); } }},
            {[fight] { auto* f = fight(); return f && f->Model().hero().state == HeroState::AirKick && f->Model().hero().stateTime > 0.05f; },
             shot("a9_air_kick")},
            {after(60), [fight] { if (auto* f = fight()) { f->Model().DebugPlace(700, 1500); f->Model().DebugForceMove(KaijuMove::Charge); f->DebugAutoplay(true, 1.0f); } }},
            {[fight] { auto* f = fight(); return f && f->Model().kaiju().state == KaijuState::Stunned && f->Model().kaiju().stateTime > 0.2f; },
             shot("a8_kaiju_stunned")},
            {after(1), [fight] { if (auto* f = fight()) f->Model().DebugForceMove(KaijuMove::Charge); }},
            {[fight] { auto* f = fight(); return f && f->Model().WindupProgress() > 0.6f; }, shot("15_warning")},
            {after(90), [fight] { if (auto* f = fight()) { f->DebugAutoplay(false); f->Model().DebugEnergy(100); } }},
            {[fight] { auto* f = fight(); return f && f->Model().hero().state == HeroState::Idle; },
             [fight] { if (auto* f = fight()) { FightInput in; in.beam = true; f->DebugInput(in); } }},
            {[fight] { auto* f = fight(); return f && f->Model().hero().state == HeroState::Beam && f->Model().hero().stateTime > 0.35f; },
             shot("16_beam")},
            {after(80), [fight] {
                 if (auto* f = fight()) {
                     f->DebugAutoplay(true);
                     f->Model().DebugSetHp(800, f->Model().kaiju().maxHp * 0.5f + 10); // the next hit enrages
                 }
             }},
            {[fight] { auto* f = fight(); return f && f->Model().kaiju().state == KaijuState::Roar && f->Model().kaiju().stateTime > 0.5f; },
             shot("17_enrage")},
            {after(120), [] { Progress::Options().touchControls = true; }},
            {after(20), shot("18_touch_controls")},
            {after(1), [fight] { Progress::Options().touchControls = false; if (auto* f = fight()) f->DebugPause(true); }},
            {after(10), shot("19_pause")},
            {after(1), [fight] { if (auto* f = fight()) { f->DebugPause(false); f->Model().DebugSetHp(800, 40); } }},
            {[fight] { auto* f = fight(); return f && f->Model().kaiju().state == KaijuState::Dying && f->Model().kaiju().stateTime > 1.2f; },
             shot("20_kaiju_falls")},
            {[fight] { auto* f = fight(); return f && f->DebugExploded(); }, [] {}},
            {after(70), shot("21_victory")},
            {[fight] { auto* f = fight(); return f && f->DebugPhase() == 3 && f->DebugPhaseTime() > 1.4f; }, shot("22_epilogue")},
            {after(1), [fight] { if (auto* f = fight()) f->DebugSkipDialogue(); }},
            {after(80), shot("23_result")},
            {after(1), [] { GoToFight(false); }},
            {after(5), [fight] { if (auto* f = fight()) { f->DebugSkipIntro(); f->Model().DebugSetHp(1, 2000); f->DebugAutoplay(false); } }},
            {after(2), [fight] { if (auto* f = fight()) f->Model().DebugForceMove(KaijuMove::Stomp); }},
            {[fight] { auto* f = fight(); return f && f->DebugPhase() == 5 && f->DebugPhaseTime() > 0.8f; }, shot("24_defeat")},
            {after(1), [this] { SetWindowSize(1000, 900); }},
            {after(20), [] { Automation::SaveWindow("25_margins"); }},
            {after(1), [this] { Quit(); }},
        };
    }

    // Fight moments captured whenever they first happen during the bot's fight.
    struct ActionShot {
        const char* name;
        std::function<bool(FightScene&)> when;
    };
    const std::vector<ActionShot> kActions = {
        {"a1_punch_hit", [](FightScene& f) { const auto& h = f.Model().hero(); return h.state == HeroState::Attack && h.combo < 2 && f.DebugRecoil() > 0.12f; }},
        {"a2_kick_hit", [](FightScene& f) { const auto& h = f.Model().hero(); return h.state == HeroState::Attack && h.combo == 2 && f.DebugRecoil() > 0.12f; }},
        {"a3_kaiju_swing", [](FightScene& f) { const auto& k = f.Model().kaiju(); return k.state == KaijuState::Active && k.move == KaijuMove::Swing && k.stateTime > 0.04f; }},
        {"a4_kaiju_charge", [](FightScene& f) { const auto& k = f.Model().kaiju(); return k.state == KaijuState::Active && k.move == KaijuMove::Charge && k.stateTime > 0.3f; }},
        {"a5_kaiju_throw", [](FightScene& f) { const auto& k = f.Model().kaiju(); return k.state == KaijuState::Active && k.move == KaijuMove::Boulder && k.stateTime > 0.08f; }},
        {"a6_boulder_lift", [](FightScene& f) { const auto& k = f.Model().kaiju(); return k.state == KaijuState::Windup && k.move == KaijuMove::Boulder && k.stateTime > 0.4f; }},
    };
    std::vector<std::string> actionShots;
    int forced = 0;
    bool collecting = false;

    void CaptureActions() {
        auto* f = Current<FightScene>();
        if (!collecting || !f || f->DebugPhase() != 1) return;
        for (const auto& a : kActions) {
            if (std::find(actionShots.begin(), actionShots.end(), a.name) != actionShots.end() || !a.when(*f)) continue;
            Automation::SaveCanvas(a.name);
            actionShots.push_back(a.name);
        }
        // Keep the kaiju cycling through its moves so each one shows up, and nobody dies yet.
        const auto& k = f->Model().kaiju();
        if (actionShots.size() < kActions.size() && (f->Model().hero().hp < 300 || k.hp < k.maxHp * 0.55f))
            f->Model().DebugSetHp(1000, k.maxHp * 0.9f);
        if (k.state == KaijuState::Idle && k.idleFor < 0.3f && actionShots.size() < kActions.size()) {
            static const KaijuMove order[] = {KaijuMove::Swing, KaijuMove::Charge, KaijuMove::Boulder, KaijuMove::Stomp};
            f->Model().DebugForceMove(order[forced++ % 4]);
        }
    }

    void Capture(int frame) {
        if (frame == 1) BuildTour();
        CaptureActions();
        ++stepFrame;
        // A tour that stalls (a condition never met) still ends instead of hanging the run.
        if (stepIndex < tour.size() && stepFrame > 1200) {
            Automation::SaveCanvas(TextFormat("stalled_step_%02d", (int)stepIndex));
            Quit();
            return;
        }
        if (stepIndex < tour.size() && tour[stepIndex].ready()) {
            tour[stepIndex].act();
            ++stepIndex;
            stepFrame = 0;
        }
    }
};

void ConfigureLogging() {
#ifdef RAYGAME_QUIET_EXAMPLE
    constexpr bool console = false;
#else
    constexpr bool console = true;
#endif
    try {
        Diagnostics::Initialize(FileSystem::UserDataDirectory("raygame.ultra_quest") / "logs/game.log", console);
        Diagnostics::InstallRaylibCallback();
    } catch (...) {
        if (!console) SetTraceLogLevel(LOG_NONE);
    }
}

// Headless fights between the bot and Red King at several skill levels.
int RunBalanceReport(const std::string& path) {
    std::ofstream out(std::filesystem::u8path(path));
    if (!out) return 1;
    const float skills[] = {1.0f, 0.75f, 0.5f, 0.3f};
    const int kRuns = 200;
    out << "Ultra Quest balance: bot vs Red King, " << kRuns << " seeded fights per skill\n";
    out << "skill  win%  lostHp lostTime  winTime  hpLeft  kaijuLeft(loss)  parries  beams  ranks C/B/A/S/SS\n";
    for (float skill : skills) {
        int wins = 0, lostHp = 0, lostTime = 0;
        double winTime = 0, hpLeft = 0, kaijuLeft = 0, parries = 0, beams = 0;
        int ranks[5] = {};
        double used[4] = {}, damage[4] = {};
        for (int run = 1; run <= kRuns; ++run) {
            Fight fight((std::uint32_t)run);
            FightBot bot;
            bot.skill = skill;
            bot.rng = Rng((std::uint32_t)(run * 31 + 7));
            int steps = 0;
            while (fight.result() == FightResult::Running && steps < (int)((kTimeLimit + 5) / kFightStep)) {
                fight.Step(bot.Think(fight));
                fight.events().clear();
                ++steps;
            }
            parries += fight.stats().perfectGuards;
            beams += fight.stats().beams;
            for (int m = 0; m < 4; ++m) {
                used[m] += fight.stats().movesUsed[m];
                damage[m] += fight.stats().damageBy[m];
            }
            if (fight.result() == FightResult::Won) {
                ++wins;
                winTime += fight.elapsed();
                hpLeft += fight.hero().hp / fight.hero().maxHp;
                ranks[std::max(0, fight.Rank())]++;
            } else {
                kaijuLeft += fight.kaiju().hp / fight.kaiju().maxHp;
                if (fight.result() == FightResult::LostTime) ++lostTime;
                else ++lostHp;
            }
        }
        int losses = kRuns - wins;
        out << TextFormat("%.2f  %5.1f  %6d %8d  %7.1f  %5.0f%%  %14.0f%%  %7.1f  %5.1f  %d/%d/%d/%d/%d\n", skill,
                          100.0 * wins / kRuns, lostHp, lostTime, wins ? winTime / wins : 0.0, wins ? 100 * hpLeft / wins : 0.0,
                          losses ? 100 * kaijuLeft / losses : 0.0, parries / kRuns, beams / kRuns, ranks[0], ranks[1],
                          ranks[2], ranks[3], ranks[4]);
        out << "       per fight  swing/charge/boulder/stomp  used "
            << TextFormat("%.1f/%.1f/%.1f/%.1f", used[0] / kRuns, used[1] / kRuns, used[2] / kRuns, used[3] / kRuns)
            << "  damage to Z "
            << TextFormat("%.0f/%.0f/%.0f/%.0f\n", damage[0] / kRuns, damage[1] / kRuns, damage[2] / kRuns, damage[3] / kRuns);
    }
    return 0;
}
} // namespace

void GoToTitle() {
    if (game) game->LoadScene(CreateRef<TitleScene>());
}
void GoToExplore() {
    if (game) game->LoadScene(CreateRef<ExploreScene>());
}
void GoToTransform() {
    if (game) game->LoadScene(CreateRef<TransformScene>());
}
void GoToFight(bool story) {
    if (game) game->LoadScene(CreateRef<FightScene>(story));
}
void QuitGame() {
    if (game) game->Quit();
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

} // namespace UQ

int main(int argc, char** argv) {
    using namespace UQ;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--capture") && i + 1 < argc && !Automation::Start(std::filesystem::u8path(argv[++i]), {true}))
            return 1;
        if (!std::strcmp(argv[i], "--balance") && i + 1 < argc) return RunBalanceReport(argv[++i]);
    }
    ConfigureLogging();
    Progress::Load();
    int result = 0;
    try {
        QuestGame instance;
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
