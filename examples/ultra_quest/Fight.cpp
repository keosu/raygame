#include "Fight.h"

namespace UQ {

namespace {
constexpr float kGravity = 3000.0f;
constexpr float kRockGravity = 1800.0f;
constexpr float kWalk = 310.0f;
constexpr float kJump = 1340.0f;
constexpr float kDashSpeed = 980.0f, kDashTime = 0.22f, kDashCooldown = 0.5f;
constexpr float kHeroHalf = 70.0f, kKaijuHalf = 105.0f;
constexpr float kHeroHeight = 400.0f, kKaijuHeight = 440.0f;
constexpr float kPerfectWindow = 0.16f;
constexpr float kBeamCharge = 0.55f, kBeamTime = 1.3f, kBeamTickEvery = 0.1f, kBeamTick = 24.0f;
constexpr float kRockRadius = 46.0f;

struct Swing {
    float startup, active, recovery, reach, damage, poise, knock, lunge, stop;
    const char* sound;
};
// Jab, straight, then the spinning kick that finishes the chain.
constexpr Swing kCombo[3] = {
    {0.07f, 0.07f, 0.15f, 150, 26, 20, 100, 160, 0.06f, "punch"},
    {0.08f, 0.07f, 0.18f, 170, 32, 26, 140, 180, 0.075f, "punch"},
    {0.12f, 0.09f, 0.30f, 210, 52, 62, 380, 240, 0.12f, "kick"},
};

int Sign(float v) { return v < 0 ? -1 : 1; }
} // namespace

Fight::Fight(std::uint32_t seed) : rng(seed) {}

void Fight::Emit(EventKind kind, Vector2 at, float amount, std::string text, bool onKaiju) {
    pending.push_back({kind, at, amount, std::move(text), onKaiju});
}

void Fight::SetHero(HeroState s) {
    h.state = s;
    h.stateTime = 0;
    h.hitDone = false;
}

void Fight::SetKaiju(KaijuState s, float length) {
    k.state = s;
    k.stateTime = 0;
    k.stateLength = length;
    k.hitDone = false;
}

void Fight::Step(const FightInput& input) {
    const float dt = kFightStep;
    if (freeze > 0) {
        freeze -= dt;
        return;
    }
    if (outcome == FightResult::Running) {
        time += dt;
        if (time >= kTimeLimit) {
            outcome = FightResult::LostTime;
            SetHero(HeroState::Lose);
            SetKaiju(KaijuState::Win);
            Emit(EventKind::Sound, {h.x, 300}, 0, "defeat");
        }
    }
    chainTime -= dt;
    if (chainTime <= 0) chain = 0;
    StepHero(outcome == FightResult::Running || h.state == HeroState::Beam ? input : FightInput{}, dt);
    StepKaiju(dt);
    StepProjectiles(dt);
    ResolveBodies();
}

// ---------------------------------------------------------------- hero

void Fight::StepHero(const FightInput& in, float dt) {
    h.stateTime += dt;
    h.dashCooldown -= dt;
    h.invulnerable -= dt;
    h.flash -= dt;
    h.guardPressed += dt;
    if (in.guard && !h.wasGuard) h.guardPressed = 0;
    h.wasGuard = in.guard;

    bool grounded = h.y <= 0;
    bool free = h.state == HeroState::Idle || h.state == HeroState::Walk || h.state == HeroState::Guard;
    if (free && k.state != KaijuState::Dying) h.facing = Sign(k.x - h.x);

    auto startDash = [&] {
        int dir = in.moveX > 0.3f ? 1 : in.moveX < -0.3f ? -1 : h.facing;
        SetHero(HeroState::Dash);
        h.vx = dir * kDashSpeed;
        h.invulnerable = std::max(h.invulnerable, 0.2f);
        h.dashCooldown = kDashCooldown;
        Emit(EventKind::Sound, {h.x, 100}, 0, "dash");
        Emit(EventKind::Dust, {h.x, 0}, 6);
    };
    auto startJump = [&] {
        SetHero(HeroState::Air);
        h.vy = kJump;
        h.vx = in.moveX * kWalk;
        h.combo = 0;
        Emit(EventKind::Sound, {h.x, 0}, 0, "jump");
        Emit(EventKind::Dust, {h.x, 0}, 5);
    };
    auto startAttack = [&](int stage) {
        SetHero(HeroState::Attack);
        h.combo = stage;
        h.queued = false;
        Emit(EventKind::Sound, {h.x, 300}, 0, "swing");
    };

    switch (h.state) {
    case HeroState::Idle:
    case HeroState::Walk:
    case HeroState::Guard:
        if (in.beam && h.energy >= h.maxEnergy) {
            SetHero(HeroState::BeamCharge);
            h.energy = 0;
            h.vx = 0;
            stat.beams++;
            Emit(EventKind::Sound, {h.x, 300}, 0, "charge");
            Emit(EventKind::Text, {h.x, 430}, 0, "泽斯提姆光线！");
            break;
        }
        if (in.dash && h.dashCooldown <= 0) { startDash(); break; }
        if (in.jump) { startJump(); break; }
        if (in.attack) { startAttack(0); break; }
        if (in.guard) {
            if (h.state != HeroState::Guard) SetHero(HeroState::Guard);
            h.vx = Approach(h.vx, 0, 4000 * dt);
            break;
        }
        h.vx = in.moveX * kWalk;
        if (std::fabs(in.moveX) > 0.1f) {
            if (h.state != HeroState::Walk) SetHero(HeroState::Walk);
        } else if (h.state != HeroState::Idle) {
            SetHero(HeroState::Idle);
        }
        break;
    case HeroState::Air:
        h.vx = Approach(h.vx, in.moveX * kWalk, 2600 * dt);
        if (in.attack && h.combo == 0) {
            SetHero(HeroState::AirKick);
            h.vx = h.facing * 900;
            h.vy = -1500;
            h.combo = 1; // one kick per jump
            Emit(EventKind::Sound, {h.x, h.y}, 0, "swing");
        }
        break;
    case HeroState::AirKick:
        h.vy = -1500;
        break;
    case HeroState::Dash:
        if (h.stateTime >= kDashTime) {
            h.vx *= 0.25f;
            SetHero(HeroState::Idle);
        }
        break;
    case HeroState::Attack: {
        const Swing& s = kCombo[h.combo];
        float t = h.stateTime;
        h.vx = t < s.startup + s.active ? h.facing * s.lunge : Approach(h.vx, 0, 3000 * dt);
        if (in.attack && t > s.startup * 0.5f && h.combo < 2) h.queued = true;
        float end = s.startup + s.active + s.recovery;
        if (h.queued && t >= s.startup + s.active + s.recovery * 0.35f) {
            startAttack(h.combo + 1);
        } else if (t >= end) {
            h.combo = 0;
            SetHero(HeroState::Idle);
        } else if (t >= s.startup + s.active && in.dash && h.dashCooldown <= 0) {
            h.combo = 0;
            startDash();
        } else if (t >= s.startup + s.active && in.guard) {
            h.combo = 0;
            SetHero(HeroState::Guard);
        }
        break;
    }
    case HeroState::Hurt:
        h.vx = Approach(h.vx, 0, 1800 * dt);
        if (h.stateTime >= h.hitstun && grounded) SetHero(HeroState::Idle);
        break;
    case HeroState::Down:
        h.vx = Approach(h.vx, 0, 1500 * dt);
        if (h.stateTime >= 0.9f && grounded) SetHero(HeroState::GetUp);
        break;
    case HeroState::GetUp:
        if (h.stateTime >= 0.35f) SetHero(HeroState::Idle);
        break;
    case HeroState::BeamCharge:
        if (h.stateTime >= kBeamCharge) {
            SetHero(HeroState::Beam);
            h.beamTick = 0;
            Emit(EventKind::Sound, {h.x, 300}, 0, "beam");
            Emit(EventKind::Beam, {h.x, 300});
        }
        break;
    case HeroState::Beam:
        h.beamTick -= dt;
        if (h.beamTick <= 0) {
            h.beamTick += kBeamTickEvery;
            Vector2 from, to;
            if (BeamSegment(from, to)) {
                if ((k.x - h.x) * h.facing > 0 && k.state != KaijuState::Dying && k.state != KaijuState::Roar)
                    HurtKaiju(kBeamTick, 16, 70, {k.x - h.facing * 60, 300}, nullptr);
                for (auto& r : rockList)
                    if (r.alive && (r.x - h.x) * h.facing > 0 && std::fabs(r.y - 300) < 120 &&
                        std::fabs(r.x - h.x) < std::fabs(to.x - h.x)) {
                        r.alive = false;
                        stat.rocksBroken++;
                        Emit(EventKind::RockBreak, {r.x, r.y}, 1, "rock");
                    }
            }
        }
        if (h.stateTime >= kBeamTime) SetHero(outcome == FightResult::Won ? HeroState::Win : HeroState::Idle);
        break;
    case HeroState::Win:
    case HeroState::Lose:
        h.vx = Approach(h.vx, 0, 2000 * dt);
        break;
    }

    // Integrate.
    if (h.state != HeroState::AirKick && (h.y > 0 || h.vy > 0)) h.vy -= kGravity * dt;
    h.x += h.vx * dt;
    h.y += h.vy * dt;
    if (h.y <= 0) {
        bool landed = h.y < 0 || h.vy < 0;
        h.y = 0;
        h.vy = 0;
        if (landed && (h.state == HeroState::Air || h.state == HeroState::AirKick)) {
            Emit(EventKind::Sound, {h.x, 0}, 0, "land");
            Emit(EventKind::Dust, {h.x, 0}, h.state == HeroState::AirKick ? 10 : 6);
            h.combo = 0;
            h.vx *= 0.3f;
            SetHero(HeroState::Idle);
        }
    }
    h.x = Clamp(h.x, kArenaMargin, kArenaW - kArenaMargin);
    if (outcome == FightResult::Won && h.state != HeroState::Beam && h.state != HeroState::Win && h.y <= 0)
        SetHero(HeroState::Win);
    HeroHits();
}

void Fight::HeroHits() {
    if (h.state == HeroState::Attack) {
        const Swing& s = kCombo[h.combo];
        float t = h.stateTime;
        if (t < s.startup || t >= s.startup + s.active || h.hitDone) return;
        float lo = std::min(h.x, h.x + h.facing * s.reach), hi = std::max(h.x, h.x + h.facing * s.reach);
        for (auto& r : rockList) {
            if (r.alive && r.y < 440 && r.x + kRockRadius > lo && r.x - kRockRadius < hi) {
                r.alive = false;
                h.hitDone = true;
                h.energy = std::min(h.maxEnergy, h.energy + 8);
                stat.rocksBroken++;
                Emit(EventKind::RockBreak, {r.x, r.y}, 1, "rock");
                freeze = 0.05f;
                return;
            }
        }
        if (k.state == KaijuState::Dying) return;
        if (k.x + kKaijuHalf > lo && k.x - kKaijuHalf < hi && h.y < kKaijuHeight) {
            h.hitDone = true;
            HurtKaiju(s.damage, s.poise, s.knock * h.facing, {k.x - h.facing * kKaijuHalf * 0.6f, 230 + h.combo * 30.0f},
                      s.sound);
            freeze = s.stop;
        }
    } else if (h.state == HeroState::AirKick && !h.hitDone && k.state != KaijuState::Dying) {
        float footX = h.x + h.facing * 80;
        if (std::fabs(footX - k.x) < kKaijuHalf + 40 && h.y < kKaijuHeight) {
            h.hitDone = true;
            HurtKaiju(44, 45, 240 * h.facing, {k.x - h.facing * kKaijuHalf * 0.5f, h.y + 40}, "kick");
            freeze = 0.1f;
            SetHero(HeroState::Air);
            h.vy = 900;
            h.vx = -h.facing * 360;
            h.combo = 1;
        }
    }
}

bool Fight::BeamSegment(Vector2& from, Vector2& to) const {
    if (h.state != HeroState::Beam) return false;
    from = {h.x + h.facing * 70, h.y + 300};
    float end = h.facing > 0 ? kArenaW + 200 : -200;
    if ((k.x - h.x) * h.facing > 0 && k.state != KaijuState::Dying) end = k.x - h.facing * 40;
    to = {end, h.y + 300};
    return true;
}

bool Fight::HurtHero(KaijuMove source, float damage, float knock, float fromX, bool knockdown, bool unblockable) {
    lastHurt = Hurt::Dodged;
    if (h.invulnerable > 0 || h.state == HeroState::Down || h.state == HeroState::GetUp ||
        h.state == HeroState::Win || h.state == HeroState::Lose || outcome != FightResult::Running)
        return false;
    int away = Sign(h.x - fromX);
    bool front = (fromX - h.x) * h.facing >= 0;
    Vector2 at = {h.x - away * kHeroHalf * 0.5f, 240};
    if (h.state == HeroState::Guard && front && !unblockable) {
        if (h.guardPressed <= kPerfectWindow) {
            lastHurt = Hurt::Parried;
            h.energy = std::min(h.maxEnergy, h.energy + 20);
            stat.perfectGuards++;
            Emit(EventKind::Parry, at, 0, "parry");
            Emit(EventKind::Text, {h.x, 420}, 0, "完美防御！");
            freeze = 0.12f;
            return true;
        }
        lastHurt = Hurt::Blocked;
        h.hp -= damage * 0.2f;
        stat.damageTaken += damage * 0.2f;
        stat.damageBy[(int)source - 1] += damage * 0.2f;
        h.vx = away * knock * 0.6f;
        h.energy = std::min(h.maxEnergy, h.energy + 4);
        Emit(EventKind::Guard, at, damage * 0.2f, "guard");
        freeze = 0.04f;
    } else {
        lastHurt = Hurt::Hit;
        h.hp -= damage;
        stat.damageTaken += damage;
        stat.damageBy[(int)source - 1] += damage;
        h.energy = std::min(h.maxEnergy, h.energy + damage * 0.06f);
        h.flash = 0.14f;
        chain = 0;
        Emit(EventKind::Hit, at, damage, "hurt");
        Emit(EventKind::Shake, at, knockdown ? 16 : 8);
        bool armored = h.state == HeroState::BeamCharge || h.state == HeroState::Beam;
        if (!armored) {
            h.combo = 0;
            if (knockdown) {
                SetHero(HeroState::Down);
                h.vx = away * knock;
                h.vy = 520;
                h.invulnerable = 1.4f;
                freeze = 0.1f;
            } else {
                SetHero(HeroState::Hurt);
                h.hitstun = 0.32f;
                h.vx = away * knock;
                freeze = 0.06f;
            }
        }
    }
    if (h.hp <= 0) {
        h.hp = 0;
        outcome = FightResult::LostHp;
        SetHero(HeroState::Lose);
        h.invulnerable = 99;
        SetKaiju(KaijuState::Win);
        Emit(EventKind::Sound, {h.x, 300}, 0, "defeat");
    }
    return true;
}

// ---------------------------------------------------------------- kaiju

void Fight::HurtKaiju(float damage, float poise, float knock, Vector2 at, const char* sound) {
    if (k.state == KaijuState::Dying || k.state == KaijuState::Roar || outcome != FightResult::Running) return;
    k.hp -= damage;
    k.flash = sound ? 0.1f : 0.035f; // beam ticks only tint, or the kaiju washes out white
    stat.hits++;
    chain++;
    chainTime = 1.3f;
    stat.maxCombo = std::max(stat.maxCombo, chain);
    if (sound) h.energy = std::min(h.maxEnergy, h.energy + damage * 0.18f);
    Emit(EventKind::Hit, at, damage, sound ? sound : "", true);
    k.pushback += knock * 0.5f;
    if (k.hp <= 0) {
        k.hp = 0;
        outcome = FightResult::Won;
        SetKaiju(KaijuState::Dying, 2.4f);
        rockList.clear();
        waveList.clear();
        Emit(EventKind::Shake, at, 18);
        Emit(EventKind::Sound, at, 0, "roar");
        return;
    }
    if (!k.enraged && k.hp <= k.maxHp * 0.5f) {
        k.enraged = true;
        k.move = KaijuMove::None;
        SetKaiju(KaijuState::Roar, 1.6f);
        Emit(EventKind::Enrage, {k.x, 400}, 0, "roar");
        Emit(EventKind::Text, {k.x, 440}, 0, "雷德王狂暴化！");
        return;
    }
    bool armored = (k.move == KaijuMove::Charge && (k.state == KaijuState::Active || k.state == KaijuState::Windup)) ||
                   k.state == KaijuState::Stunned;
    if (!sound && !armored) {
        // The beam holds the kaiju in place for as long as it pours on.
        if (k.state != KaijuState::Stagger) {
            k.move = KaijuMove::None;
            SetKaiju(KaijuState::Stagger, 0.35f);
        } else {
            k.stateLength = std::max(k.stateLength, k.stateTime + 0.35f);
        }
        return;
    }
    k.poise += poise;
    float threshold = k.enraged ? 150.0f : 115.0f;
    if (!armored && k.poise >= threshold && k.staggerImmune <= 0) {
        k.poise = 0;
        k.staggerImmune = 2.4f;
        k.move = KaijuMove::None;
        SetKaiju(KaijuState::Stagger, 0.6f);
        Emit(EventKind::Text, {k.x, 430}, 0, "破防！");
    }
}

void Fight::DebugSetHp(float hero, float kaiju) {
    h.hp = hero;
    k.hp = kaiju;
    if (kaiju <= k.maxHp * 0.5f) k.enraged = true;
}

void Fight::DebugPlace(float heroX, float kaijuX, float kaijuIdle) {
    h.x = heroX;
    h.y = h.vx = h.vy = 0;
    k.x = kaijuX;
    k.y = k.vy = k.pushback = 0;
    SetHero(HeroState::Idle);
    SetKaiju(KaijuState::Idle);
    k.move = KaijuMove::None;
    k.idleFor = kaijuIdle;
    h.facing = h.x < k.x ? 1 : -1;
    k.facing = -h.facing;
}

void Fight::DebugForceMove(KaijuMove move) {
    if (k.state == KaijuState::Idle || k.state == KaijuState::Walk) StartMove(move);
}

float Fight::WindupProgress() const {
    return k.state == KaijuState::Windup && k.stateLength > 0 ? Clamp(k.stateTime / k.stateLength, 0.0f, 1.0f) : -1;
}

void Fight::StartMove(KaijuMove move) {
    static constexpr float kWindup[] = {0, 0.72f, 0.85f, 0.9f, 0.9f};
    k.move = move;
    stat.movesUsed[(int)move - 1]++;
    k.repeat = (int)move == k.lastMove ? k.repeat + 1 : 0;
    k.lastMove = (int)move;
    k.chargeDistance = 0;
    k.wavesLeft = move == KaijuMove::Stomp && k.enraged ? 1 : 0;
    SetKaiju(KaijuState::Windup, kWindup[(int)move] * (k.enraged ? 0.8f : 1.0f));
    if (move == KaijuMove::Charge) Emit(EventKind::Sound, {k.x, 400}, 0, "roar");
}

void Fight::Decide() {
    float dist = std::fabs(h.x - k.x);
    if (h.state == HeroState::Down || h.state == HeroState::GetUp) {
        k.idleFor = 0.4f;
        return;
    }
    // Weights for swing, charge, boulder, stomp by range.
    float w[5] = {0, 0, 0, 0, 0};
    if (dist < 340) {
        w[1] = 5; w[2] = 1; w[3] = 1; w[4] = 3;
    } else if (dist < 760) {
        w[1] = 0; w[2] = 4; w[3] = 3; w[4] = 3;
    } else {
        w[2] = 4; w[3] = 5; w[4] = 1;
    }
    if (k.lastMove > 0 && k.repeat >= 1) w[k.lastMove] = 0;
    // Close in for a swing now and then instead of always firing from range.
    if (dist >= 340 && dist < 760 && rng.Chance(0.25f)) {
        SetKaiju(KaijuState::Walk, 0.9f);
        return;
    }
    float total = w[1] + w[2] + w[3] + w[4];
    float pick = rng.Float() * total;
    int move = 1;
    for (int i = 1; i <= 4; ++i) {
        if (pick < w[i]) { move = i; break; }
        pick -= w[i];
    }
    StartMove((KaijuMove)move);
}

void Fight::StepKaiju(float dt) {
    k.stateTime += dt;
    k.flash -= dt;
    k.staggerImmune -= dt;
    k.poise = std::max(0.0f, k.poise - 30 * dt);
    float speed = Speed();
    int toHero = Sign(h.x - k.x);
    float dist = std::fabs(h.x - k.x);
    bool turning = k.state == KaijuState::Idle || k.state == KaijuState::Walk ||
                   (k.state == KaijuState::Windup && k.move != KaijuMove::Charge);
    if (turning) k.facing = toHero;

    float vx = 0;
    switch (k.state) {
    case KaijuState::Idle:
        if (outcome != FightResult::Running) break;
        k.idleFor -= dt;
        if (dist > 460) vx = toHero * 130 * speed;
        else if (dist < 200) vx = -toHero * 90;
        if (k.idleFor <= 0) Decide();
        break;
    case KaijuState::Walk:
        vx = toHero * 170 * speed;
        if (dist < 300 || k.stateTime >= k.stateLength) {
            if (dist < 340) {
                StartMove(KaijuMove::Swing);
            } else {
                SetKaiju(KaijuState::Idle);
                k.idleFor = 0.2f;
            }
        }
        break;
    case KaijuState::Windup:
        if (k.move == KaijuMove::Stomp && k.stateTime >= k.stateLength - 0.25f && k.y <= 0 && !k.hitDone) {
            k.vy = 760; // hop before the landing that sends the shockwaves
            k.hitDone = true;
        }
        if (k.stateTime >= k.stateLength) {
            float active = k.move == KaijuMove::Swing ? 0.16f : k.move == KaijuMove::Charge ? 1.6f : 0.35f;
            SetKaiju(KaijuState::Active, active);
            if (k.move == KaijuMove::Swing) Emit(EventKind::Sound, {k.x, 300}, 0, "swing");
            if (k.move == KaijuMove::Boulder) {
                int count = k.enraged ? 3 : 1;
                for (int i = 0; i < count; ++i) {
                    float target = h.x + (count > 1 ? (i - 1) * 230.0f : 0.0f);
                    target = Clamp(target, kArenaMargin, kArenaW - kArenaMargin);
                    float flight = 0.95f + i * 0.12f;
                    Rock r; // leaves the hands of the throwing pose
                    r.x = k.x + k.facing * 150;
                    r.y = 280;
                    r.vx = (target - r.x) / flight;
                    r.vy = (0.5f * kRockGravity * flight * flight - r.y) / flight;
                    r.spin = rng.Range(-6, 6);
                    rockList.push_back(r);
                }
                Emit(EventKind::Sound, {k.x, 400}, 0, "rock");
            }
        }
        break;
    case KaijuState::Active:
        if (k.move == KaijuMove::Swing) {
            if (!k.hitDone && (h.x - k.x) * k.facing > -40 && dist <= 320 && h.y < 380) {
                k.hitDone = true;
                if (HurtHero(KaijuMove::Swing, 100, 560, k.x, true) && lastHurt == Hurt::Parried) {
                    k.move = KaijuMove::None;
                    SetKaiju(KaijuState::Stagger, 0.9f);
                    break;
                }
            }
            if (k.stateTime >= k.stateLength) SetKaiju(KaijuState::Recover, 0.65f / speed);
        } else if (k.move == KaijuMove::Charge) {
            vx = k.facing * 860 * speed;
            k.chargeDistance += std::fabs(vx) * dt;
            // Bodies are kept exactly this far apart, so contact is "within reach of touching".
            // A well-timed jump clears it: the apex is about 300.
            if (!k.hitDone && std::fabs(h.x - k.x) < kHeroHalf + kKaijuHalf + 12 && h.y < 220) {
                k.hitDone = true;
                if (HurtHero(KaijuMove::Charge, 120, 720, k.x - k.facing * 50, true) && lastHurt == Hurt::Parried) {
                    k.move = KaijuMove::None;
                    SetKaiju(KaijuState::Stunned, 1.3f);
                    Emit(EventKind::Shake, {k.x, 200}, 10);
                    break;
                }
            }
            bool wall = (k.facing > 0 && k.x >= kArenaW - kArenaMargin - 20) || (k.facing < 0 && k.x <= kArenaMargin + 20);
            if (wall) {
                if (k.chargeDistance > 500) {
                    SetKaiju(KaijuState::Stunned, 1.5f);
                    Emit(EventKind::Debris, {k.x + k.facing * 120, 200}, 24, "crash");
                    Emit(EventKind::Shake, {k.x, 200}, 20);
                    Emit(EventKind::Text, {k.x, 440}, 0, "撞墙眩晕！");
                } else {
                    SetKaiju(KaijuState::Recover, 0.5f);
                }
            } else if (k.stateTime >= k.stateLength) {
                SetKaiju(KaijuState::Recover, 0.55f);
            }
        } else if (k.move == KaijuMove::Boulder) {
            if (k.stateTime >= k.stateLength) SetKaiju(KaijuState::Recover, 0.55f / speed);
        } else if (k.move == KaijuMove::Stomp) {
            if (k.y <= 0 && k.vy <= 0 && !k.hitDone) {
                k.hitDone = true;
                for (int dir : {-1, 1}) waveList.push_back({k.x + dir * 90.0f, dir * 720 * speed, 2.0f});
                Emit(EventKind::Shake, {k.x, 0}, 22);
                Emit(EventKind::Dust, {k.x, 0}, 22);
            }
            if (k.hitDone && k.stateTime >= k.stateLength) {
                if (k.wavesLeft > 0) {
                    k.wavesLeft--;
                    k.vy = 620;
                    SetKaiju(KaijuState::Active, 0.35f);
                } else {
                    SetKaiju(KaijuState::Recover, 0.6f / speed);
                }
            }
        }
        break;
    case KaijuState::Recover:
        if (k.stateTime >= k.stateLength) {
            k.move = KaijuMove::None;
            SetKaiju(KaijuState::Idle);
            k.idleFor = k.enraged ? rng.Range(0.3f, 0.7f) : rng.Range(0.5f, 1.1f);
        }
        break;
    case KaijuState::Stagger:
    case KaijuState::Stunned:
        if (k.stateTime >= k.stateLength) {
            k.move = KaijuMove::None;
            SetKaiju(KaijuState::Idle);
            k.idleFor = 0.3f;
        }
        break;
    case KaijuState::Roar:
        if (k.stateTime >= 0.2f && !k.hitDone) {
            k.hitDone = true;
            Emit(EventKind::Shake, {k.x, 400}, 24);
            rockList.clear();
            if (dist < 520 && h.state != HeroState::Down && h.state != HeroState::Lose) {
                SetHero(HeroState::Hurt);
                h.hitstun = 0.45f;
                h.vx = toHero * 760;
                h.combo = 0;
            }
        }
        if (k.stateTime >= k.stateLength) {
            SetKaiju(KaijuState::Idle);
            k.idleFor = 0.4f;
        }
        break;
    case KaijuState::Dying:
        if (k.stateTime >= k.stateLength && !k.hitDone) {
            k.hitDone = true;
            Emit(EventKind::Explode, {k.x, 220}, 1, "explode");
            Emit(EventKind::Shake, {k.x, 220}, 30);
        }
        break;
    case KaijuState::Win:
        break;
    }

    if (k.y > 0 || k.vy > 0) k.vy -= kGravity * dt;
    k.y += k.vy * dt;
    if (k.y <= 0) {
        if (k.vy < -200) Emit(EventKind::Sound, {k.x, 0}, 0, "stomp");
        k.y = 0;
        k.vy = 0;
    }
    k.x += (vx + k.pushback) * dt;
    k.pushback = Approach(k.pushback, 0, 1600 * dt);
    k.x = Clamp(k.x, kArenaMargin, kArenaW - kArenaMargin);
}

void Fight::StepProjectiles(float dt) {
    for (auto& r : rockList) {
        if (!r.alive) continue;
        r.vy -= kRockGravity * dt;
        r.x += r.vx * dt;
        r.y += r.vy * dt;
        r.spin += dt;
        bool inside = r.x + kRockRadius > h.x - kHeroHalf && r.x - kRockRadius < h.x + kHeroHalf &&
                      r.y - kRockRadius < h.y + kHeroHeight && r.y + kRockRadius > h.y;
        if (inside && HurtHero(KaijuMove::Boulder, 65, 320, r.x, false)) {
            r.alive = false;
            Emit(EventKind::RockBreak, {r.x, r.y}, 1, "rock");
            continue;
        }
        if (r.y <= kRockRadius * 0.4f) {
            r.alive = false;
            Emit(EventKind::RockBreak, {r.x, 10}, 1, "rock");
            Emit(EventKind::Shake, {r.x, 0}, 5);
        }
    }
    rockList.erase(std::remove_if(rockList.begin(), rockList.end(), [](const Rock& r) { return !r.alive; }),
                   rockList.end());
    for (auto& w : waveList) {
        w.x += w.speed * dt;
        w.life -= dt;
        if (!w.hit && std::fabs(w.x - h.x) < 64 && h.y < 70) {
            w.hit = true;
            HurtHero(KaijuMove::Stomp, 80, 380, w.x - Sign(w.speed) * 30, false, true);
        }
    }
    waveList.erase(std::remove_if(waveList.begin(), waveList.end(),
                                  [](const Shockwave& w) { return w.life <= 0 || w.x < -100 || w.x > kArenaW + 100; }),
                   waveList.end());
}

void Fight::ResolveBodies() {
    if (k.state == KaijuState::Dying || h.state == HeroState::Dash || h.state == HeroState::Down ||
        h.state == HeroState::GetUp || h.y > 300)
        return;
    float minGap = kHeroHalf + kKaijuHalf;
    float dx = h.x - k.x;
    if (std::fabs(dx) >= minGap) return;
    int side = dx == 0 ? -k.facing : Sign(dx);
    float target = k.x + side * minGap;
    if (target < kArenaMargin || target > kArenaW - kArenaMargin) {
        // Pinned against the edge: the kaiju gives way instead.
        h.x = Clamp(target, kArenaMargin, kArenaW - kArenaMargin);
        k.x = h.x - side * minGap;
    } else {
        h.x = target;
    }
}

int Fight::Rank() const {
    if (outcome != FightResult::Won) return -1;
    float score = h.hp / h.maxHp * 55.0f;
    score += Clamp((kTimeLimit - time) / 70.0f, 0.0f, 1.0f) * 35.0f;
    score += std::min(stat.perfectGuards, 3) * 2.0f;
    score += std::min(stat.maxCombo, 20) / 20.0f * 4.0f;
    return score >= 90 ? 4 : score >= 75 ? 3 : score >= 58 ? 2 : score >= 40 ? 1 : 0;
}

// ---------------------------------------------------------------- bot

FightInput FightBot::Think(const Fight& fight) {
    FightInput in;
    if (fight.result() != FightResult::Running) return in;
    const Hero& h = fight.hero();
    const Kaiju& k = fight.kaiju();
    float now = fight.elapsed();
    float dx = k.x - h.x;
    int toward = dx > 0 ? 1 : -1;
    float dist = std::fabs(dx);

    if (k.state != seenState || k.move != seenMove) {
        seenState = k.state;
        seenMove = k.move;
        reactAt = now + (0.34f - 0.22f * skill) + rng.Range(0.0f, 0.08f);
        decided = false;
    }
    bool seen = now >= reactAt;
    if (seen && !decided) {
        decided = true;
        plan = rng.Chance(0.3f + 0.65f * skill);
        parry = rng.Chance(skill > 0.7f ? 0.6f : skill * 0.3f);
        if (k.state == KaijuState::Windup && k.move == KaijuMove::Charge) dodgeCharge = plan;
    }
    if (k.move != KaijuMove::Charge) dodgeCharge = false;
    attackGap -= kFightStep;
    holdGuard -= kFightStep;

    // Shockwaves and rocks first: they are the only things that come from anywhere.
    for (const auto& w : fight.waves()) {
        float rel = (h.x - w.x) * (w.speed > 0 ? 1 : -1);
        if (!w.hit && rel > 40 && rel < 150 && h.y <= 0 && rng.Chance(0.35f + 0.6f * skill)) {
            in.jump = true;
            return in;
        }
    }
    for (const auto& r : fight.rocks()) {
        float fall = (r.vy + std::sqrt(std::max(0.0f, r.vy * r.vy + 2 * 1800 * r.y))) / 1800;
        float landing = r.x + r.vx * std::max(0.0f, fall);
        if (std::fabs(landing - h.x) < 190 && rng.Chance(0.4f + 0.6f * skill)) {
            bool front = (r.x - h.x) * h.facing > 0;
            if (skill > 0.6f && front && std::fabs(r.x - h.x) < 200 && r.y < 430 && attackGap <= 0) {
                in.attack = true;
                attackGap = 0.25f;
                return in;
            }
            int away = std::fabs(landing - h.x) < 20 ? -toward : (landing > h.x ? -1 : 1);
            in.moveX = (float)away;
            if (skill > 0.7f && fall < 0.45f && h.dashCooldown <= 0) in.dash = true;
            return in;
        }
    }

    bool ready = h.state == HeroState::Idle || h.state == HeroState::Walk || h.state == HeroState::Guard;
    if (k.state == KaijuState::Windup && seen && plan) {
        float left = k.stateLength - k.stateTime;
        if (k.move == KaijuMove::Swing && dist < 400) {
            if (parry) {
                if (left < 0.1f) {
                    in.guard = true;
                    holdGuard = 0.3f;
                }
                return in;
            }
            in.moveX = -toward;
            if (left < 0.3f && h.dashCooldown <= 0) in.dash = true;
            return in;
        }
        if (k.move == KaijuMove::Stomp) {
            in.moveX = dist < 300 ? -toward : 0;
            return in;
        }
        if (k.move == KaijuMove::Charge) return in; // wait for the run, then dash through it
    }
    if (k.state == KaijuState::Active && k.move == KaijuMove::Charge && (h.x - k.x) * k.facing > 0) {
        // Dash through when it is close, or jump it when the dash is not ready.
        if (dodgeCharge) {
            if (dist < 330 && h.dashCooldown <= 0) {
                in.dash = true;
                in.moveX = (float)toward;
            } else if (dist < 330 && h.y <= 0) {
                in.jump = true;
            }
        }
        return in;
    }
    if (holdGuard > 0) {
        in.guard = true;
        return in;
    }
    if (!ready && h.state != HeroState::Attack) return in;

    bool open = k.state == KaijuState::Recover || k.state == KaijuState::Stagger || k.state == KaijuState::Stunned ||
                k.state == KaijuState::Idle || k.state == KaijuState::Walk ||
                (k.state == KaijuState::Windup && (!plan || k.move == KaijuMove::Boulder));
    if (h.energy >= h.maxEnergy && ready && k.state != KaijuState::Roar &&
        (k.state == KaijuState::Recover || k.state == KaijuState::Stunned || k.state == KaijuState::Stagger ||
         k.state == KaijuState::Idle)) {
        in.beam = true;
        return in;
    }
    if (open && k.state != KaijuState::Roar) {
        if (dist > 230) {
            in.moveX = (float)toward;
        } else if (attackGap <= 0) {
            in.attack = true;
            attackGap = 0.11f + (1 - skill) * 0.12f;
        }
    } else if (dist < 260 && ready) {
        in.moveX = -toward * 0.6f;
    }
    return in;
}

} // namespace UQ
