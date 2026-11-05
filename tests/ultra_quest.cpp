// Rules of the Ultra Quest fight model (examples/ultra_quest/Fight.*), headless.
#include "../examples/ultra_quest/Fight.h"
#include <cstdio>
#include <functional>

using namespace UQ;

static int passed = 0, failed = 0;
static void Check(bool ok, const char* name) {
    if (ok) {
        ++passed;
    } else {
        ++failed;
        std::printf("FAIL %s\n", name);
    }
}

// Steps until `done` or `limit` seconds; `input` may vary per step.
static void Run(Fight& f, float limit, const std::function<FightInput()>& input, const std::function<bool()>& done = {}) {
    for (int i = 0; i < (int)(limit / kFightStep); ++i) {
        if (done && done()) return;
        f.Step(input ? input() : FightInput{});
        f.events().clear();
    }
}

static void Combo() {
    Fight f;
    f.DebugPlace(600, 800);
    float before = f.kaiju().hp;
    int step = 0;
    Run(f, 1.2f, [&] { FightInput in; in.attack = (step++ % 8) == 0; return in; });
    Check(before - f.kaiju().hp >= 26 + 32 + 52 - 0.01f, "three-hit combo lands every stage");
    Check(f.stats().maxCombo >= 3, "combo counter counts the chain");
    Check(f.hero().energy > 15, "hits build energy");
}

static void Swing(bool guard, bool perfect, float& hpLost, bool& kaijuStaggered, bool& knocked) {
    Fight f;
    f.DebugPlace(600, 840);
    f.DebugForceMove(KaijuMove::Swing);
    knocked = false;
    Run(f, 2.0f, [&] {
        FightInput in;
        float left = f.kaiju().stateLength - f.kaiju().stateTime;
        if (guard) in.guard = perfect ? (f.kaiju().state != KaijuState::Windup || left < 0.08f) : true;
        if (f.hero().state == HeroState::Down) knocked = true;
        return in;
    });
    hpLost = f.hero().maxHp - f.hero().hp;
    kaijuStaggered = f.stats().perfectGuards == 1;
}

static void Guarding() {
    float lost;
    bool staggered, knocked;
    Swing(true, true, lost, staggered, knocked);
    Check(lost == 0 && staggered && !knocked, "perfect guard takes no damage and staggers the kaiju");
    Swing(true, false, lost, staggered, knocked);
    Check(lost > 0 && lost < 30 && !staggered && !knocked, "held guard blocks 80 % of a swing");
    Swing(false, false, lost, staggered, knocked);
    Check(lost >= 99 && knocked, "an unguarded swing knocks Z down");
}

static void DashThroughCharge() {
    Fight f;
    f.DebugPlace(700, 1500);
    f.DebugForceMove(KaijuMove::Charge);
    bool dashed = false;
    Run(f, 3.0f, [&] {
        FightInput in;
        float dist = std::fabs(f.kaiju().x - f.hero().x);
        if (!dashed && f.kaiju().state == KaijuState::Active && dist < 300) {
            in.dash = true;
            in.moveX = 1;
            dashed = true;
        }
        return in;
    });
    Check(dashed && f.hero().hp == f.hero().maxHp, "dashing through a charge avoids it");
    Check(f.hero().x > f.kaiju().x || f.kaiju().state == KaijuState::Stunned || f.kaiju().state == KaijuState::Idle,
          "the dash ends on the far side or the kaiju is past");
}

static void Stomp(bool jump, bool guard, float& lost) {
    Fight f;
    f.DebugPlace(700, 1200);
    f.DebugForceMove(KaijuMove::Stomp);
    bool jumped = false;
    Run(f, 3.5f, [&] {
        FightInput in;
        in.guard = guard;
        for (const auto& w : f.waves())
            if (jump && !jumped && w.speed < 0 && w.x - f.hero().x < 150 && w.x > f.hero().x) {
                in.jump = true;
                jumped = true;
            }
        return in;
    });
    lost = f.hero().maxHp - f.hero().hp;
}

static void Shockwaves() {
    float lost;
    Stomp(false, true, lost);
    Check(lost >= 79, "guarding does not stop a stomp shockwave");
    Stomp(true, false, lost);
    Check(lost == 0, "jumping clears the shockwave");
}

static void Beam() {
    Fight f;
    f.DebugPlace(600, 1300);
    f.DebugEnergy(100);
    f.DebugForceMove(KaijuMove::Swing);
    float before = f.kaiju().hp;
    bool pressed = false, held = true;
    Run(f, 2.4f, [&] {
        if (f.hero().state == HeroState::Beam && f.hero().stateTime > 0.15f && f.kaiju().state != KaijuState::Stagger)
            held = false;
        FightInput in;
        in.beam = !pressed;
        pressed = true;
        return in;
    });
    Check(held, "the beam holds the kaiju in a stagger while it fires");
    float dealt = before - f.kaiju().hp;
    Check(dealt > 250 && dealt < 360, "the beam deals its full ticks once");
    Check(f.hero().energy < 100 && f.stats().beams == 1, "the beam spends the gauge");
    Fight g;
    g.DebugPlace(600, 1300);
    g.DebugEnergy(60);
    Run(g, 1.0f, [] { FightInput in; in.beam = true; return in; });
    Check(g.hero().state != HeroState::Beam && g.stats().beams == 0, "no beam without a full gauge");
}

static void Enrage() {
    Fight f;
    f.DebugPlace(600, 800);
    f.DebugSetHp(1000, f.kaiju().maxHp * 0.5f + 10);
    int step = 0;
    bool roared = false;
    Run(f, 1.0f, [&] {
        if (f.kaiju().state == KaijuState::Roar) roared = true;
        FightInput in;
        in.attack = (step++ % 8) == 0;
        return in;
    });
    Check(roared && f.kaiju().enraged, "half health makes Red King roar and enrage");
}

static void Timer() {
    Fight f;
    f.DebugPlace(200, 2200);
    Run(f, kTimeLimit + 2, {}, [&] { return f.result() != FightResult::Running; });
    Check(f.result() == FightResult::LostTime, "the colour timer running out loses the fight");
    Check(f.timerWarning(), "the timer warns near the end");
}

static void Determinism() {
    auto play = [](std::uint32_t seed) {
        Fight f(seed);
        FightBot bot;
        bot.rng = Rng(seed * 3 + 1);
        Run(f, kTimeLimit + 2, [&] { return bot.Think(f); }, [&] { return f.result() != FightResult::Running; });
        return std::make_pair((int)f.result(), f.elapsed() * 1000 + f.hero().hp + f.kaiju().hp);
    };
    Check(play(5) == play(5), "a seed and a bot replay the same fight");
    int wins = 0;
    for (std::uint32_t s = 1; s <= 20; ++s) wins += play(s).first == (int)FightResult::Won;
    Check(wins >= 10, "the expert bot wins most fights");
}

static void Ranking() {
    Fight f;
    f.DebugPlace(600, 800);
    f.DebugSetHp(1000, 30);
    int step = 0;
    Run(f, 1.0f, [&] { FightInput in; in.attack = (step++ % 8) == 0; return in; },
        [&] { return f.result() != FightResult::Running; });
    Check(f.result() == FightResult::Won, "the last hit wins");
    Check(f.Rank() >= 3, "a fast, untouched win ranks S or better");
    Fight g;
    Check(g.Rank() == -1, "no rank before the end");
}

int main() {
    Combo();
    Guarding();
    DashThroughCharge();
    Shockwaves();
    Beam();
    Enrage();
    Timer();
    Determinism();
    Ranking();
    std::printf("Ultra Quest tests: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
