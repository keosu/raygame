#pragma once

#include "Common.h"

namespace UQ {

// The giant fight as a pure model: no drawing, no raylib globals, deterministic for a given
// seed and input stream, so the balance report can replay thousands of fights headless.
// World units are canvas pixels at zoom 1; x grows right, y is height above the ground.

constexpr float kArenaW = 2400.0f;
constexpr float kArenaMargin = 140.0f;
constexpr float kFightStep = 1.0f / 60.0f;
constexpr float kTimeLimit = 120.0f; // the colour timer: Z cannot stay on Earth for long
constexpr float kTimerWarning = 40.0f;

struct FightInput {
    float moveX = 0;     // -1..1
    bool jump = false;   // pressed this step
    bool attack = false; // pressed this step
    bool dash = false;   // pressed this step
    bool beam = false;   // pressed this step
    bool guard = false;  // held
};

enum class HeroState { Idle, Walk, Air, Dash, Attack, AirKick, Guard, Hurt, Down, GetUp, BeamCharge, Beam, Win, Lose };
enum class KaijuState { Idle, Walk, Windup, Active, Recover, Stagger, Stunned, Roar, Dying, Win };
enum class KaijuMove { None, Swing, Charge, Boulder, Stomp };
enum class FightResult { Running, Won, LostHp, LostTime };

struct Hero {
    float x = 600, y = 0, vx = 0, vy = 0;
    int facing = 1;
    float hp = 1000, maxHp = 1000;
    float energy = 0, maxEnergy = 100;
    HeroState state = HeroState::Idle;
    float stateTime = 0;
    int combo = 0;          // attack stage 0..2
    bool queued = false;    // next combo stage buffered
    bool hitDone = false;   // the current swing already connected
    float dashCooldown = 0;
    float invulnerable = 0;
    float guardPressed = 99; // seconds since guard went down, for perfect guards
    float flash = 0;         // white hit flash for drawing
    float hitstun = 0;
    bool wasGuard = false;
    float beamTick = 0;
};

struct Kaiju {
    float x = 1800, y = 0, vy = 0;
    int facing = -1;
    float hp = 2800, maxHp = 2800;
    KaijuState state = KaijuState::Idle;
    KaijuMove move = KaijuMove::None;
    float stateTime = 0;
    float stateLength = 0;
    float idleFor = 1.4f;    // seconds before the next decision
    float poise = 0;         // damage taken recently; enough of it staggers
    float staggerImmune = 0;
    bool enraged = false;
    bool hitDone = false;
    float flash = 0;
    float chargeDistance = 0;
    int lastMove = -1, repeat = 0;
    int wavesLeft = 0;       // stomp shockwaves still to come
    float pushback = 0;      // horizontal slide from heavy hits
};

struct Rock {
    float x, y, vx, vy;
    float spin;
    bool alive = true;
};

struct Shockwave {
    float x, speed;
    float life;
    bool hit = false;
};

enum class EventKind { Hit, Guard, Parry, Sound, Shake, Text, Dust, Debris, RockBreak, Enrage, Explode, Beam };

struct FightEvent {
    EventKind kind;
    Vector2 at{};        // world position (y as height)
    float amount = 0;    // damage, shake strength, particle count
    std::string text;    // sound name or caption
    bool onKaiju = false;
};

struct FightStats {
    float damageTaken = 0;
    int perfectGuards = 0;
    int maxCombo = 0;
    int hits = 0;
    int beams = 0;
    int rocksBroken = 0;
    // Per kaiju attack (swing, charge, boulder, stomp): times used and damage dealt to Z.
    int movesUsed[4] = {};
    float damageBy[4] = {};
};

class Fight {
public:
    explicit Fight(std::uint32_t seed = 1);

    void Step(const FightInput& input);

    const Hero& hero() const { return h; }
    const Kaiju& kaiju() const { return k; }
    const std::vector<Rock>& rocks() const { return rockList; }
    const std::vector<Shockwave>& waves() const { return waveList; }
    FightResult result() const { return outcome; }
    float elapsed() const { return time; }
    float remaining() const { return std::max(0.0f, kTimeLimit - time); }
    bool timerWarning() const { return remaining() <= kTimerWarning || h.hp <= h.maxHp * 0.3f; }
    float hitstop() const { return freeze; }
    const FightStats& stats() const { return stat; }
    int comboCount() const { return chain; }
    // 0..4 for C, B, A, S, SS once won.
    int Rank() const;
    // The beam as a segment while it fires (from, to in world units), for drawing and hits.
    bool BeamSegment(Vector2& from, Vector2& to) const;
    // Seconds into the current windup and its total length; the HUD shows a warning.
    float WindupProgress() const;

    std::vector<FightEvent>& events() { return pending; }

    // Debug hooks for the capture tour and tests.
    void DebugSetHp(float hero, float kaiju);
    void DebugEnergy(float value) { h.energy = value; }
    void DebugForceMove(KaijuMove move);
    // Puts both fighters on the ground; the kaiju then waits `kaijuIdle` seconds before deciding.
    void DebugPlace(float heroX, float kaijuX, float kaijuIdle = 999);

private:
    Hero h;
    Kaiju k;
    std::vector<Rock> rockList;
    std::vector<Shockwave> waveList;
    std::vector<FightEvent> pending;
    FightResult outcome = FightResult::Running;
    FightStats stat;
    Rng rng;
    float time = 0;
    float freeze = 0;
    int chain = 0;        // running hit chain for the combo counter
    float chainTime = 0;
    enum class Hurt { Dodged, Parried, Blocked, Hit };
    Hurt lastHurt = Hurt::Dodged; // how the last HurtHero call ended

    void StepHero(const FightInput& input, float dt);
    void StepKaiju(float dt);
    void StepProjectiles(float dt);
    void ResolveBodies();
    void SetHero(HeroState s);
    void SetKaiju(KaijuState s, float length = 0);
    void Decide();
    void StartMove(KaijuMove move);
    void HeroHits();
    void KaijuHits();
    // Damage to the hero from `fromX`; returns false if dodged. `unblockable` ignores guarding.
    bool HurtHero(KaijuMove source, float damage, float knock, float fromX, bool knockdown, bool unblockable = false);
    void HurtKaiju(float damage, float poise, float knock, Vector2 at, const char* sound);
    void Emit(EventKind kind, Vector2 at, float amount = 0, std::string text = {}, bool onKaiju = false);
    float Speed() const { return k.enraged ? 1.25f : 1.0f; }
};

// A scripted opponent for the balance report and the capture tour. `skill` 0..1 sets the
// reaction time and how often it reads a telegraph correctly.
struct FightBot {
    float skill = 1.0f;
    Rng rng{7};
    float reactAt = -1; // fight time when the current kaiju action becomes "seen"
    KaijuState seenState = KaijuState::Idle;
    KaijuMove seenMove = KaijuMove::None;
    bool decided = false;
    bool plan = false;   // read this telegraph correctly
    bool parry = false;  // answer a swing with a perfect guard instead of backing off
    bool dodgeCharge = false;
    float holdGuard = 0;
    float attackGap = 0;
    FightInput Think(const Fight& fight);
};

} // namespace UQ
