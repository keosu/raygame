#include "Battle.h"
#include "Art.h"
#include "Assets.h"
#include "Draw.h"
#include "Game.h"
#include "Gui.h"
#include <rlgl.h>

namespace UG {

namespace {
constexpr float kStep = 1.0f / 60.0f;
constexpr float kSpeeds[] = {1.0f, 2.0f, 3.0f};
constexpr float kBrothersMax = 70;
constexpr float kStrikeMax = 30;
constexpr float kFusionMax = 75;
constexpr float kFusionCharge = 25; // first charge after the roster completes
constexpr float kShieldTaken = 0.42f; // damage share that gets through Zetton's barrier
constexpr float kFusionImpactAt = 2.9f, kFusionEnd = 4.6f; // cut-in timeline, real seconds
constexpr float kWaveGap = 16;
constexpr float kExhaustTime = 8; // a drained hero stays down this long, then stands at 60% energy
constexpr unsigned kKaijuMask = 1u << kKaijuLayer;
constexpr const char* kModeNames[] = {"最前", "最强", "最近"};

Rectangle MapRect() { return {kMapX, kMapY, kMapW, kMapH}; }
Rectangle CardRect(int i) { return {1086 + (i % 2) * 98.0f, 90 + (i / 2) * 104.0f, 92, 98}; }
Rectangle SkillRect(int i) { return {1086 + i * 98.0f, 404, 92, 52}; }
Rectangle FusionRect() { return {1086, 460, 190, 58}; }
Rectangle InfoRect() { return {1086, 524, 190, 188}; }

Color SpinHue(Color c, int steps) {
    for (int i = 0; i < steps % 3; ++i) {
        unsigned char r = c.r;
        c.r = c.g;
        c.g = c.b;
        c.b = r;
    }
    return c;
}

float EaseOut(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return 1 - (1 - t) * (1 - t) * (1 - t);
}

MonsterActor* KaijuOf(Collider* collider) {
    auto* object = collider ? collider->GetGameObject() : nullptr;
    auto actor = object ? object->GetComponent<MonsterActor>() : nullptr;
    return actor && !actor->dead ? actor.get() : nullptr;
}
} // namespace

// ================================================================ actors

float MonsterActor::Height() const { return Art::KaijuHeight(type) * Scale(); }

void MonsterActor::FixedUpdate() {
    if (dead || !battle->Running()) return;
    BattleScene& b = *battle;
    const EnemyDef& def = Def();
    float dt = GameTime::DeltaTime();
    anim += dt;
    appear = std::min(1.0f, appear + dt * 2.5f);
    flash = std::max(0.0f, flash - dt * 6);
    bool phased = phaseTime > 0;
    for (float* timer : {&slowTime, &burnTime, &breakTime, &vulnTime, &stunTime, &shieldTime, &phaseTime, &buffTime})
        *timer = std::max(0.0f, *timer - dt);
    if (burnTime > 0) {
        b.DamageEnemy(*this, burnDps * dt, -1, true);
        if (GetRandomValue(0, 5) == 0) b.Emit(BattleScene::Fx::Ember, Center(), {255, 140, 40, 255}, 1, 40, 5, 0.5f);
    } else {
        burnDps = 0;
    }
    if (dead) return;

    const PathLine& line = b.map.paths[path];
    if (type == EnemyId::Zetton) {
        abilityA -= dt;
        if (abilityA <= 0) {
            shieldTime = 1.6f;
            abilityA = 11.0f;
            Sfx::Play("shield", 0.5f);
            b.AddRing(Center(), 10, 46, 0.4f, 4, {255, 220, 100, 255});
        }
        abilityB -= dt;
        if (abilityB <= 0) {
            abilityB = 13;
            if (dist < line.length - 200) {
                b.Burst(Center(), {255, 220, 90, 255}, 18, 140, 5, 0.5f);
                dist += 60;
                gameObject->transform->position = line.Sample(dist, &dir);
                b.Burst(Center(), {255, 220, 90, 255}, 18, 140, 5, 0.5f);
                b.Text(Add(Center(), {0, -40}), "瞬间移动", {255, 220, 100, 255}, 14);
                Sfx::Play("teleport", 0.6f);
            }
        }
    } else if (type == EnemyId::Belial) {
        abilityA -= dt;
        if (abilityA <= 0) {
            abilityA = 11.0f;
            for (int i = 0; i < 2; ++i) b.SpawnEnemy(EnemyId::BaltanClone, path, std::max(0.0f, dist - 30 - i * 22));
            b.Burst(Center(), {200, 40, 90, 255}, 26, 160, 6, 0.6f);
            Sfx::Play("split", 0.6f);
        }
        if (!enraged && hp < maxHp * 0.5f) {
            enraged = true;
            speedMul = 1.45f;
            b.shake = std::max(b.shake, 0.8f);
            b.Banner("贝利亚暴怒了！", "移动速度大幅提升，集中火力！", Palette::red, true);
            Sfx::Play("boss", 0.8f, 0);
        }
    } else if (type == EnemyId::Metron) {
        abilityA -= dt;
        if (abilityA <= 0) {
            abilityA = 6.0f;
            if (HeroActor* h = b.NearestHero(Center(), kTile * 2.4f)) {
                b.DisableHero(*h, HeroActor::Disable::Confused, 2.2f, Center());
                b.Text(Add(h->Pos(), {0, -46}), "幻觉！", {255, 120, 110, 255}, 14);
            }
        }
    } else if (type == EnemyId::Tregear) {
        const Color violet{140, 100, 255, 255};
        abilityA -= dt;
        if (abilityA <= 0 && !phased) {
            abilityA = 10.0f;
            phaseTime = 1.4f;
            b.AddRing(Center(), 46, 6, 0.4f, 4, violet);
            b.Burst(Center(), violet, 20, 150, 5, 0.45f);
            b.Text(Add(Center(), {0, -50}), "异次元之门", {190, 170, 255, 255}, 14);
            Sfx::Play("teleport", 0.6f);
        }
        if (phased && phaseTime <= 0) {
            if (dist < line.length - 200) dist += 30;
            gameObject->transform->position = line.Sample(dist, &dir);
            b.AddRing(Center(), 6, 50, 0.45f, 4, violet);
            b.Burst(Center(), violet, 20, 160, 5, 0.45f);
        }
        abilityB -= dt;
        if (abilityB <= 0 && !phased) {
            abilityB = 12.0f;
            for (MonsterActor* e : b.KaijuInCircle(Center(), kTile * 2.6f)) {
                if (e == this || e->Def().boss) continue;
                e->buffTime = 4.5f;
                b.AddBeam(Center(), e->Center(), 2.5f, 0.3f, violet, WHITE);
            }
            b.AddRing(Center(), 10, kTile * 2.6f, 0.5f, 4, violet);
            b.Text(Add(Center(), {0, -50}), "狂暴光线", {190, 170, 255, 255}, 14);
            Sfx::Play("split", 0.5f);
        }
    } else if (type == EnemyId::Gatanothor) {
        if (!enraged && hp < maxHp * 0.5f) {
            enraged = true; // shell cracked
            armor *= 0.25f;
            b.shake = std::max(b.shake, 1.0f);
            b.Banner("外壳碎裂！", "加坦杰厄的护甲大幅下降，全力进攻！", Palette::gold, true);
            Sfx::Play("explode", 0.8f, 0);
        }
        abilityA -= dt;
        if (abilityA <= 0) {
            abilityA = 13.0f;
            b.fogTime = 4.0f;
            b.Text(Add(Center(), {0, -70}), "黑雾", {200, 180, 255, 255}, 18);
            Sfx::Play("boss", 0.6f, 0);
        }
        abilityB -= dt;
        if (abilityB <= 0) {
            abilityB = 9.0f;
            if (HeroActor* h = b.NearestHero(Center(), kTile * 4.2f)) {
                b.DisableHero(*h, HeroActor::Disable::Petrified, 3.0f, Center());
                b.Text(Add(h->Pos(), {0, -46}), "石化！", {210, 210, 200, 255}, 14);
            }
        }
    }

    strikePause = std::max(0.0f, strikePause - dt);
    if (def.strikeDamage > 0 && b.Assault() > 0 && phaseTime <= 0 && stunTime <= 0 && appear >= 1) {
        strikeTimer -= dt;
        if (strikeTimer <= 0) {
            HeroActor* target = nullptr;
            float best = def.strikeRange * kTile;
            for (auto& h : b.heroes) {
                float d = Len(Sub(h->Pos(), Center()));
                if (h->disabled != HeroActor::Disable::Exhausted && d <= best) {
                    best = d;
                    target = h.get();
                }
            }
            if (target) {
                b.KaijuAttack(*this, *target);
                strikeTimer = def.strikeCooldown * Rand(0.9f, 1.1f);
                if (!def.boss) strikePause = 0.45f;
            } else {
                strikeTimer = 0.25f;
            }
        }
    }

    float slow = slowTime > 0 ? slowAmount * (1 - def.slowResist) : 0;
    float v = stunTime > 0 || strikePause > 0 || sieging ? 0 : def.speed * kTile * speedMul * (1 - slow);
    if (buffTime > 0) v *= 1.4f;
    if (phaseTime > 0) v *= 1.3f;
    dist = std::min(dist + v * dt, line.length);
    Vector2 at = line.Sample(dist, &dir);
    if (dir.x > 0.2f) face = 1;
    else if (dir.x < -0.2f) face = -1;
    if (!b.Siege()) {
        gameObject->transform->position = at;
        if (dist >= line.length) b.Leak(*this);
        return;
    }
    if (!sieging && (Len(Sub(at, b.map.basePos)) < siegeReach || dist >= line.length)) {
        sieging = true;
        siegeTimer = Rand(0.2f, 0.6f);
    }
    if (sieging) {
        at = Add(at, Mul(Vector2{-dir.y, dir.x}, siegeSide));
        face = b.map.basePos.x >= at.x ? 1.0f : -1.0f;
        if (stunTime <= 0 && phaseTime <= 0 && appear >= 1) {
            siegeTimer -= dt;
            if (siegeTimer <= 0) {
                b.SiegeHit(*this);
                siegeTimer = def.boss ? 1.5f : 1.2f;
            }
        }
    }
    gameObject->transform->position = at;
}

void MonsterActor::Render() {
    if (dead || battle->embedded) return; // the database preview shows only the skill's effects
    Art::KaijuPose pose;
    pose.feet = Add(Feet(), {0, (1 - appear) * 6});
    pose.scale = Scale() * (0.6f + 0.4f * appear);
    pose.time = anim;
    pose.face = face;
    pose.flash = flash;
    pose.alpha = appear * (phaseTime > 0 ? 0.28f : 1.0f);
    pose.chill = slowTime > 0 ? std::min(1.0f, slowTime) : 0;
    pose.burn = burnTime > 0 ? 1.0f : 0.0f;
    pose.shielded = shieldTime > 0;
    pose.enraged = enraged || buffTime > 0;
    if (phaseTime > 0 || buffTime > 0) {
        Draw::BlendAdd();
        Draw::Glow(Center(), 26 * Scale(), WithAlpha({140, 90, 255, 255}, phaseTime > 0 ? 0.5f : 0.3f));
        Draw::BlendNormal();
    }
    Art::DrawKaiju(type, pose);
    Vector2 top = Add(Feet(), {0, -Height() - 4});
    if (stunTime > 0)
        for (int i = 0; i < 3; ++i) Draw::Star(Add(top, Mul(Dir(battle->simTime * 5 + i * 2.1f), 9)), 3.5f, 1.5f, 0, Palette::gold);
    if (slowTime > 0) {
        Draw::BlendAdd();
        Draw::Glow(Center(), 16 * Scale(), WithAlpha({120, 200, 255, 255}, 0.25f));
        Draw::BlendNormal();
    }
}

void HeroActor::FixedUpdate() {
    if (!battle->Running()) return;
    float dt = GameTime::DeltaTime();
    build = std::min(1.0f, build + dt * 3);
    recoil = std::max(0.0f, recoil - dt * 5);
    lunge = std::max(0.0f, lunge - dt * 3.5f);
    flash = std::max(0.0f, flash - dt * 4);
    hurt = std::max(0.0f, hurt - dt * 4);
    calm += dt;
    if (calm > 4 && disabled != Disable::Exhausted) hp = std::min(maxHp, hp + maxHp * 0.05f * dt);
    if (disabledTime > 0) {
        disabledTime = std::max(0.0f, disabledTime - dt);
        if (disabledTime <= 0) {
            if (disabled == Disable::Exhausted) {
                hp = maxHp * 0.6f;
                battle->AddRing(Pos(), 6, 40, 0.4f, 3, HeroInfo(hero).beam);
                battle->Text(Add(Pos(), {0, -50}), "重新站起！", HeroInfo(hero).beam, 14);
            }
            disabled = Disable::None;
        }
        return;
    }
    cooldown -= battle->fogTime > 0 ? dt * 0.5f : dt;
    if (cooldown > 0) return;
    MonsterActor* best = nullptr;
    float bestScore = -1e30f;
    for (MonsterActor* e : battle->KaijuInCircle(Pos(), HeroRange(hero, level))) {
        if (e->appear < 0.3f || e->phaseTime > 0) continue;
        float score = mode == 0 ? e->dist - battle->map.paths[e->path].length : mode == 1 ? e->hp : -Len(Sub(e->Feet(), Pos()));
        if (score > bestScore) {
            bestScore = score;
            best = e;
        }
    }
    if (!best) {
        cooldown = 0.05f;
        return;
    }
    battle->Fire(*this, *best);
    cooldown = HeroCooldown(hero, level) * nextCooldownScale * (battle->embedded ? 0.5f : 1.0f); // the preview is a showcase
    nextCooldownScale = 1;
}

void HeroActor::Render() {
    if (battle->embedded) return; // the database page draws the big portrait instead
    const HeroDef& def = HeroInfo(hero);
    Vector2 p = Pos();
    float time = battle->simTime;
    // Hexagonal pedestal with a hero-colored rim.
    Vector2 base = Add(p, {0, 12});
    Draw::Fan(Add(base, {0, 5}), Draw::EllipsePoints(Add(base, {0, 5}), 25, 12, 0, 6), {28, 30, 42, 255});
    Draw::Fan(base, Draw::EllipsePoints(base, 25, 12, 0, 6), Darken(def.primary, 0.35f));
    Draw::Fan(Add(base, {0, -2}), Draw::EllipsePoints(Add(base, {0, -2}), 22, 10, 0, 6), {120, 132, 156, 255});
    Draw::Fan(Add(base, {-2, -3}), Draw::EllipsePoints(Add(base, {-2, -3}), 14, 6, 0, 6), {160, 172, 194, 255});
    for (int i = 0; i <= level; ++i) Draw::Diamond({base.x - (level * 6.0f) + i * 12.0f, base.y + 9}, 3.6f, 4.6f, Palette::gold);
    if (level >= 2) {
        Draw::BlendAdd();
        Draw::GlowStretched(base, 34, 16, 0, WithAlpha(def.beam, 0.35f + 0.15f * std::sin(time * 3 + id)));
        Draw::BlendNormal();
    }
    float bob = std::sin(time * 2.2f + id * 1.7f) * 1.3f;
    float size = (58 + level * 3.0f) * EaseOut(build) * (1 + 0.07f * recoil);
    bool stone = disabled == Disable::Petrified && disabledTime > 0;
    bool confused = disabled == Disable::Confused && disabledTime > 0;
    bool exhausted = disabled == Disable::Exhausted && disabledTime > 0;
    if (stone || exhausted) bob = 0;
    if (exhausted) size *= 0.9f;
    Vector2 kick{0, 0};
    if (lunge > 0) {
        // Leap out along an arc and drop back onto the pedestal.
        float k = std::sin(lunge * PI);
        kick = Mul(Normalize(Sub(lungeTo, p)), 26 * k);
        kick.y -= 16 * k;
    }
    Rectangle dest{p.x - size / 2 + kick.x, p.y + 10 - size * 0.92f + bob + (exhausted ? 6 : 0) + kick.y, size, size};
    if (focus > 0 && def.kind == AttackKind::Focus) {
        float k = focus / (level >= 2 ? 6.0f : 4.0f);
        Draw::BlendAdd();
        Draw::Glow(Add(p, {0, -26}), 30 + 26 * k, WithAlpha({255, 214, 90, 255}, 0.18f + 0.3f * k));
        for (int i = 0; i < focus; ++i) {
            float a = time * 2.4f + i * 2 * PI / focus;
            Draw::Glow(Add(p, {std::cos(a) * 24, -20 + std::sin(a) * 9}), 5, WithAlpha({255, 236, 150, 255}, 0.9f));
        }
        Draw::BlendNormal();
    }
    Color tint = stone ? Color{120, 116, 106, 255} : exhausted ? Color{86, 88, 110, 255} : WHITE;
    if (hurt > 0) tint = Mix(tint, {255, 90, 90, 255}, hurt * 0.6f);
    Art::DrawPortrait(hero, dest, tint);
    if (battle->Assault() > 0 || hp < maxHp) {
        // Colour timer on the pedestal: blue while healthy, blinking red when low or exhausted.
        float ratio = Clamp(hp / maxHp, 0.0f, 1.0f);
        bool low = ratio < 0.35f || exhausted;
        bool blinkOn = !low || std::fmod(time, low && exhausted ? 0.3f : 0.5f) < (exhausted ? 0.15f : 0.3f);
        Vector2 lamp{p.x, p.y + 22};
        Color tone = low ? Color{255, 50, 50, 255} : Color{70, 170, 255, 255};
        DrawCircleV(lamp, 5.5f, {30, 34, 46, 255});
        DrawCircleV(lamp, 4, blinkOn ? tone : Darken(tone, 0.6f));
        if (blinkOn) {
            Draw::BlendAdd();
            Draw::Glow(lamp, low ? 16.0f : 10.0f, WithAlpha(tone, low ? 0.7f : 0.35f));
            Draw::BlendNormal();
        }
        if (ratio < 1 || exhausted) {
            Rectangle bar{p.x - 20, p.y + 29, 40, 4};
            DrawRectangleRec({bar.x - 1, bar.y - 1, bar.width + 2, bar.height + 2}, {10, 10, 18, 220});
            float shown = exhausted ? 1 - disabledTime / kExhaustTime : ratio;
            Color fill = exhausted ? Color{120, 120, 140, 255} : ratio > 0.35f ? Color{80, 190, 255, 255} : Color{255, 70, 70, 255};
            DrawRectangleRec({bar.x, bar.y, bar.width * Clamp(shown, 0.0f, 1.0f), bar.height}, fill);
        }
    }
    if (exhausted) Gui::TextOutline("能量耗尽", p.x, p.y - size * 0.95f, 12, {255, 120, 120, 255}, BLACK, 1.2f);
    if (stone) {
        Draw::BlendAdd();
        Draw::Glow(Add(p, {0, -24}), 26, WithAlpha({200, 196, 180, 255}, 0.25f));
        Draw::BlendNormal();
    }
    if (confused) {
        for (int i = 0; i < 3; ++i) {
            Vector2 c = Add(Add(p, {0, -size * 0.95f}), Mul(Dir(time * 4 + i * 2.1f), 12));
            Draw::Star(c, 4, 1.6f, time * 3, {255, 90, 80, 255});
        }
    }
    if (flash > 0) {
        Draw::BlendAdd();
        for (int side : {-1, 1}) {
            Vector2 eye = Art::EyePoint(hero, side);
            Draw::Glow({dest.x + eye.x / 256 * size, dest.y + eye.y / 256 * size}, 9, WithAlpha(def.beam, flash));
        }
        Draw::Glow(Add(p, {0, -24}), 18 * flash, WithAlpha(def.beam, flash * 0.6f));
        Draw::BlendNormal();
    }
}

void CorpseActor::Render() {
    Art::KaijuPose pose;
    pose.feet = Add(gameObject->transform->position, {0, t * 10});
    pose.scale = scale * (1 - t * 0.25f);
    pose.face = face;
    pose.flash = Clamp(1 - t * 1.5f, 0.0f, 1.0f);
    pose.alpha = Clamp(1 - t / 1.1f, 0.0f, 1.0f);
    Art::DrawKaiju(type, pose);
}

// ================================================================ lifecycle

BattleScene::BattleScene(int index) : Scene("Battle"), levelIndex(index) {}

BattleScene::BattleScene(HeroId hero) : Scene("Battle"), levelIndex(0), demo(true), demoHero(hero) {}

void BattleScene::OnLoad() {
    if (demo) {
        endless = DemoLevel(demoHero);
        level = &endless;
    } else if (levelIndex == kSurvivalIndex) {
        endless = *SurvivalLevel();
        level = &endless;
        EnsureWaves(2);
    } else {
        level = &Levels()[levelIndex];
    }
    roster = headless || demo ? level->heroes : Progress::Roster(*level);
    surviveTime = 0;
    newRecord = false;
    map.Build(*level);
    if (!mapTexture.id && !headless) mapTexture = RenderMap(map, 2.0f, false, false);
    for (auto& row : towerGrid) row.fill(-1);
    monsters.clear();
    heroes.clear();
    strikes.clear();
    spawning.clear();
    zones.clear();
    CreateEmitters();
    gold = level->gold;
    lives = level->lives;
    citadel = citadelMax = level->siege;
    citadelShot = 2;
    citadelPulse = 0;
    kills = botPurchases = 0;
    waveIndex = -1;
    pathToggle = 0;
    nextWaveTimer = -1;
    simTime = 0;
    paused = false;
    outcome = Outcome::None;
    outcomeTime = 0;
    placing = selected = hoverCard = -1;
    targetingStrike = false;
    brothersCooldown = 35;
    strikeCooldown = 12;
    fusionCooldown = 0;
    fusionAnim = -1;
    fogTime = 0;
    flashScreen = shake = baseFlash = goldPulse = livesPulse = 0;
    ApplySpeed();
    Banner(level->name, level->subtitle, Palette::gold);
    if (!embedded) {
        EventBus::Subscribe("ug.kill", [this](const EventData&) { goldPulse = 1; });
        Sfx::PlayMusic("battle");
    }
    if (demo) {
        speedIndex = 0;
        ApplySpeed();
        waveIndex = 0; // skills and wave scaling behave as in the first wave
        demoSpawn = 0.6f;
        demoCount = 0;
        if (BuyTower(demoHero, 9, 5)) SetDemoStar(demoStar);
    }
}

void BattleScene::OnUnload() {
    if (mapTexture.id) UnloadRenderTexture(mapTexture);
    mapTexture = {};
    SetGameSpeed(1);
}

void BattleScene::Restart() {
    Clear();
    Load();
}

void BattleScene::CreateEmitters() {
    struct Setup {
        Fx kind;
        ParticleBlend blend;
        float drag;
        Vector2 gravity;
        float endScale;
        int order;
    };
    const Setup setups[] = {
        {Fx::Spark, ParticleBlend::Additive, 2.5f, {0, 200}, 0.3f, 11},
        {Fx::Glow, ParticleBlend::Additive, 3.2f, {0, 0}, 0.25f, 11},
        {Fx::Smoke, ParticleBlend::Alpha, 1.2f, {0, -25}, 2.6f, 10},
        {Fx::Ember, ParticleBlend::Additive, 1.0f, {0, -60}, 0.2f, 11},
    };
    for (const auto& s : setups) {
        auto emitter = CreateGameObject("fx")->AddComponent<ParticleEmitter>();
        emitter->emitting = false;
        // Skills burst hundreds of sparks at once; leave headroom over a busy wave.
        emitter->maxParticles = s.kind == Fx::Spark || s.kind == Fx::Glow ? 1800 : 900;
        emitter->texture = Draw::GlowHandle();
        emitter->blend = s.blend;
        emitter->drag = s.drag;
        emitter->acceleration = s.gravity;
        emitter->endSizeScale = s.endScale;
        emitter->radialVelocity = true;
        emitter->rotationSpeedMin = emitter->rotationSpeedMax = 0;
        emitter->renderOrder = s.order;
        fx[(int)s.kind] = emitter;
    }
    fx[(int)Fx::Spark]->alignToVelocity = true;
    fx[(int)Fx::Spark]->stretch = 0.04f;
    effects = CreateGameObject("effects")->AddComponent<EffectLayer>();
    effects->renderOrder = 13;
}

// ================================================================ frame update

void BattleScene::Update() {
    Scene::Update();
    if (embedded) return; // the preview has no input, HUD or result screen
    if (restartRequested) {
        restartRequested = false;
        Restart();
        return;
    }
    float real = GameTime::UnscaledDeltaTime();
    HandleInput();
    UpdateFusion(real);
    bannerTime += real;
    goldPulse = std::max(0.0f, goldPulse - real * 3);
    livesPulse = std::max(0.0f, livesPulse - real * 2);
    if (outcome != Outcome::None) outcomeTime += real;
}

void BattleScene::SetPaused(bool value) {
    paused = value;
    ApplySpeed();
}

void BattleScene::CycleSpeed() {
    speedIndex = (speedIndex + 1) % 3;
    ApplySpeed();
}

void BattleScene::ApplySpeed() const {
    bool frozen = fusionAnim >= 0 && fusionAnim < kFusionImpactAt;
    if (!headless && !embedded) SetGameSpeed(paused || frozen ? 0.0f : kSpeeds[speedIndex]);
}

void BattleScene::EnsureWaves(int count) {
    if (!Endless()) return;
    while ((int)endless.waves.size() < count) endless.waves.push_back(SurvivalWave((int)endless.waves.size()));
}

float BattleScene::Assault() const {
    int wave = std::max(0, waveIndex);
    return Endless() ? SurvivalAssault(*level, wave) : level->assault * (1 + 0.03f * wave);
}

bool BattleScene::CanStartWave() const {
    return Running() && waveIndex + 1 < (int)level->waves.size() && (waveIndex < 0 || spawning.empty());
}

void BattleScene::HandleInput() {
    if (!Running()) return;
    if (!Input::GetFrame().focused && !paused) SetPaused(true); // e.g. app sent to background
    if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK)) {
        if (placing >= 0 || targetingStrike) {
            placing = -1;
            targetingStrike = false;
        } else if (selected >= 0) {
            selected = -1;
        } else {
            SetPaused(!paused);
        }
    }
    if (Input::GetKeyDown(KEY_P)) SetPaused(!paused);
    if (paused || Cinematic()) return;

    for (int i = 0; i < (int)roster.size(); ++i) {
        if (Input::GetKeyDown(KEY_ONE + i)) {
            int hero = (int)roster[i];
            placing = placing == hero ? -1 : hero;
            selected = -1;
            targetingStrike = false;
        }
    }
    if (Input::GetKeyDown(KEY_SPACE) && CanStartWave()) StartNextWave(waveIndex >= 0);
    if (Input::GetKeyDown(KEY_F) || Input::GetKeyDown(KEY_TAB)) CycleSpeed();
    if (Input::GetKeyDown(KEY_Q)) CastBrothers();
    if (Input::GetKeyDown(KEY_E)) CastFusion();
    if (Input::GetKeyDown(KEY_W) && strikeCooldown <= 0 && waveIndex >= 0) {
        targetingStrike = !targetingStrike;
        placing = -1;
    }
    if (HeroActor* t = FindTower(selected)) {
        if (Input::GetKeyDown(KEY_U)) UpgradeTower(*t);
        if (Input::GetKeyDown(KEY_T)) t->mode = (t->mode + 1) % 3;
        if (Input::GetKeyDown(KEY_DELETE) || Input::GetKeyDown(KEY_BACKSPACE)) SellTower(t->id);
    }
    if (Input::GetMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        placing = -1;
        targetingStrike = false;
        selected = -1;
    }

    Vector2 m = Input::GetMousePosition();
    if (!Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) || UI::IsMouseOverUI()) return;
    if (!Input::IsMouseInCanvas() || !CheckCollisionPointRec(m, MapRect())) return;
    int col = (int)std::floor((m.x - kMapX) / kTile), row = (int)std::floor((m.y - kMapY) / kTile);
    if (targetingStrike) {
        CastStrike(m);
        targetingStrike = false;
    } else if (placing >= 0) {
        PlaceTower(col, row);
    } else if (map.InGrid(col, row) && towerGrid[row][col] >= 0) {
        selected = towerGrid[row][col];
        Sfx::Play("click", 0.5f);
    } else {
        selected = -1;
    }
}

// ================================================================ simulation

void BattleScene::FixedUpdate() {
    float dt = GameTime::DeltaTime();
    simTime += dt;
    if (Running()) {
        for (auto& s : spawning) {
            s.timer -= dt;
            while (s.timer <= 0 && s.spawned < s.group.count) {
                int pathCount = (int)map.paths.size();
                int path = s.group.path < 0 ? (pathToggle++ % pathCount) : std::min(s.group.path, pathCount - 1);
                SpawnEnemy(s.group.enemy, path, 0);
                s.spawned++;
                s.timer += s.group.interval;
            }
        }
        spawning.erase(std::remove_if(spawning.begin(), spawning.end(), [](const ActiveSpawn& s) { return s.spawned >= s.group.count; }),
                       spawning.end());
        if (spawning.empty() && waveIndex >= 0 && waveIndex + 1 < (int)level->waves.size()) {
            if (nextWaveTimer < 0) nextWaveTimer = kWaveGap;
            nextWaveTimer -= dt;
            if (nextWaveTimer <= 0) StartNextWave(false);
        }
        if (waveIndex >= 0) {
            brothersCooldown = std::max(0.0f, brothersCooldown - dt);
            strikeCooldown = std::max(0.0f, strikeCooldown - dt);
            fusionCooldown = std::max(0.0f, fusionCooldown - dt);
        }
        fogTime = std::max(0.0f, fogTime - dt);
        for (auto& s : strikes) {
            s.time += dt;
            float jetX = Lerp(kMapX - 140, kMapX + kMapW + 140, s.time / 1.8f);
            while (s.dropped < 5 && jetX >= s.target.x - 100 + s.dropped * 50) {
                auto bomb = Launch(ProjectileMotion::Lob, {jetX, s.target.y - 80});
                bomb->target = {Clamp(s.target.x - 100 + s.dropped * 50, kMapX + 10, kMapX + kMapW - 10),
                                Clamp(s.target.y + Rand(-18, 18), kMapY + 10, kMapY + kMapH - 10)};
                bomb->flightTime = 0.45f;
                float damage = 110 + 30.0f * std::max(0, waveIndex);
                bomb->onArrive = [this, damage](Projectile&, Vector2 at) {
                    if (Running()) Explode(at, 66, damage, -1, false, true, {255, 150, 48, 255});
                };
                bomb->onDraw = [](const Projectile& p) {
                    float t = Clamp(p.Age() / 0.45f, 0.0f, 1.0f);
                    Vector2 at = p.RenderPosition();
                    Draw::Shadow(p.target, 6 + t * 8, 3 + t * 3, 0.2f + t * 0.3f);
                    Draw::Capsule(Add(at, {0, -5}), Add(at, {0, 5}), 3.5f, {50, 54, 64, 255});
                    Draw::Tri(Add(at, {-4, -7}), Add(at, {4, -7}), Add(at, {0, -2}), {236, 120, 40, 255});
                };
                s.dropped++;
            }
        }
        strikes.erase(std::remove_if(strikes.begin(), strikes.end(), [](const Airstrike& s) { return s.time > 1.8f; }), strikes.end());
        if (demo) StepDemo(dt);
        StepZones(dt);
        if (Siege()) StepSiege(dt);
    }

    Scene::FixedUpdate(); // kaiju walk and use abilities, heroes attack, projectiles fly
    monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](const Ref<MonsterActor>& m) { return m->dead; }), monsters.end());

    if (Running()) {
        if (Siege()) lives = citadel > 0 ? (int)std::ceil(citadel) : 0;
        if (lives <= 0) {
            lives = 0;
            SetOutcome(Outcome::Defeat);
        } else if (!Endless() && !demo && waveIndex == (int)level->waves.size() - 1 && spawning.empty() && monsters.empty()) {
            SetOutcome(Outcome::Victory);
        }
    }
    if (Running() && waveIndex >= 0) surviveTime += dt;
    StepEffects(dt);
}

void BattleScene::StartNextWave(bool early) {
    if (!CanStartWave()) return;
    if (early && nextWaveTimer > 0) {
        int bonus = (int)std::ceil(nextWaveTimer * 1.5f);
        gold += bonus;
        goldPulse = 1;
        Text({kMapX + kMapW / 2, kMapY + 40}, TextFormat("提前召唤 +%d", bonus), Palette::gold, 18);
    }
    if (waveIndex >= 0) gold += 20 + 5 * std::min(waveIndex, 20);
    if (Siege() && waveIndex >= 0 && citadel < citadelMax) {
        float repair = std::min(citadelMax - citadel, citadelMax * 0.05f);
        citadel += repair;
        Text(Add(map.basePos, {0, -150}), TextFormat("灯塔修复 +%.0f", repair), Palette::cyan, 16);
        AddRing(map.basePos, 20, kTile * 2, 0.6f, 4, {150, 220, 255, 255});
    }
    waveIndex++;
    EnsureWaves(waveIndex + 2); // the HUD previews the wave after this one
    nextWaveTimer = -1;
    const WaveDef& w = level->waves[waveIndex];
    for (const auto& g : w.groups) spawning.push_back({g, 0, g.delay});
    if (w.boss) {
        const char* bossName = "强敌";
        for (const auto& g : w.groups)
            if (EnemyInfo(g.enemy).boss) bossName = EnemyInfo(g.enemy).name;
        Banner(TextFormat("第 %d 波 · 警告", waveIndex + 1), TextFormat("%s 正在逼近！", bossName), Palette::red, true);
        Sfx::Play("boss", 0.8f, 0);
    } else {
        bool last = waveIndex == (int)level->waves.size() - 1;
        Banner(TextFormat("第 %d 波", waveIndex + 1), last ? "最终波！守住最后的防线" : "怪兽来袭，准备迎战", Palette::gold);
        Sfx::Play("wave", 0.7f, 0);
    }
}

void BattleScene::SpawnEnemy(EnemyId type, int path, float dist) {
    const EnemyDef& def = EnemyInfo(type);
    auto object = CreateGameObject("kaiju");
    auto e = object->AddComponent<MonsterActor>();
    e->battle = this;
    e->object = object;
    e->type = type;
    e->path = path;
    e->dist = dist;
    int wave = std::max(0, waveIndex);
    e->maxHp = e->hp = def.hp * (def.boss ? BossHpScale(*level, wave) : WaveHpScale(*level, wave));
    if (embedded) e->maxHp = e->hp = 1e7f; // invisible preview targets never die: no corpses or reward text
    e->armor = def.armor * (1 + 0.05f * wave);
    e->speedMul = def.boss ? 1.0f : Rand(0.94f, 1.06f);
    e->abilityA = type == EnemyId::Belial ? 6.0f : type == EnemyId::Metron ? Rand(2, 5) : type == EnemyId::Gatanothor ? 8.0f : 4.0f;
    e->abilityB = type == EnemyId::Tregear ? 8.0f : type == EnemyId::Gatanothor ? 5.0f : 6.5f;
    e->strikeTimer = Rand(0.8f, std::max(1.0f, def.strikeCooldown));
    if (Siege()) {
        e->siegeReach = kTile * Rand(1.65f, 2.15f) * (def.boss ? 1.2f : 1.0f);
        e->siegeSide = Rand(-16, 16);
    }
    e->anim = Rand(0, 10);
    e->ySort = true;
    object->transform->position = map.paths[path].Sample(dist, &e->dir);
    auto body = object->AddComponent<CircleCollider>(12 * e->Scale());
    body->offset = {0, -e->Height() * 0.45f};
    body->isTrigger = true;
    body->layer = kKaijuLayer;
    body->mask = 0; // query target only: no contacts with other kaiju
    monsters.push_back(e);
}

std::vector<MonsterActor*> BattleScene::KaijuInCircle(Vector2 center, float radius) const {
    std::vector<MonsterActor*> out;
    for (Collider* c : GetPhysicsWorld().OverlapCircle(center, radius, kKaijuMask))
        if (MonsterActor* e = KaijuOf(c)) out.push_back(e);
    return out;
}

std::vector<Ref<MonsterActor>> BattleScene::Alive() const {
    std::vector<Ref<MonsterActor>> out;
    for (const auto& m : monsters)
        if (!m->dead) out.push_back(m);
    return out;
}

void BattleScene::SiegeHit(MonsterActor& e) {
    const EnemyDef& def = e.Def();
    int wave = std::max(0, waveIndex);
    float damage = def.leak * (1 + 0.04f * wave) * (e.buffTime > 0 || e.enraged ? 1.3f : 1.0f);
    citadel = std::max(0.0f, citadel - damage);
    citadelPulse = 1;
    livesPulse = 1;
    baseFlash = std::max(baseFlash, 0.6f);
    if (leakWave < 0) leakWave = waveIndex + 1;
    leakCount[(int)e.type]++;
    Vector2 hit = Add(map.basePos, {Rand(-16, 16), Rand(-80, 20)});
    Color tone = def.strike == KaijuStrike::Beam ? Color{255, 96, 70, 255} : def.strike == KaijuStrike::Bolt ? Color{255, 236, 90, 255}
                                                                                                             : Color{255, 160, 90, 255};
    if (def.strike == KaijuStrike::Beam || def.strike == KaijuStrike::Bolt || def.boss) {
        AddBeam(e.Center(), hit, def.boss ? 6.0f : 3.0f, 0.25f, tone, WHITE);
    } else {
        Vector2 claw = Lerp(e.Center(), hit, 0.75f);
        AddRing(claw, 4, 20, 0.25f, 3, tone);
    }
    Sparks(hit, tone, 5, 150);
    if (def.boss) {
        shake = std::max(shake, 0.45f);
        Text(Add(hit, {0, -20}), TextFormat("-%.0f", damage), Palette::red, 18);
    }
    Sfx::Play("hit", 0.2f);
}

void BattleScene::StepSiege(float dt) {
    citadelPulse = std::max(0.0f, citadelPulse - dt * 2);
    citadelShot -= dt;
    if (citadelShot > 0 || waveIndex < 0) return;
    // Lighthouse beam: the lamp picks off the nearest kaiju at its wall.
    MonsterActor* best = nullptr;
    float bestDist = kTile * 3.4f;
    for (const auto& e : monsters) {
        if (e->dead || e->appear < 0.5f || e->phaseTime > 0) continue;
        float d = Len(Sub(e->Feet(), map.basePos));
        if (d < bestDist) {
            bestDist = d;
            best = e.get();
        }
    }
    if (!best) {
        citadelShot = 0.2f;
        return;
    }
    citadelShot = 1.0f;
    Vector2 sun = Add(map.basePos, kLanternOffset), at = best->Center();
    AddBeam(sun, at, 10, 0.35f, {120, 200, 255, 255}, WHITE);
    AddBeam(sun, at, 4, 0.35f, {255, 220, 150, 255}, WHITE);
    AddRing(sun, 10, 42, 0.3f, 3, {200, 236, 255, 255});
    Burst(at, {150, 210, 255, 255}, 12, 160, 5, 0.4f);
    DamageEnemy(*best, 80 * (1 + 0.12f * std::max(0, waveIndex)), -1, true);
    Sfx::Play("beam", 0.3f);
}

void BattleScene::StepZones(float dt) {
    for (auto& z : zones) {
        z.time += dt;
        for (MonsterActor* e : KaijuInCircle(z.at, z.radius)) {
            DamageEnemy(*e, z.dps * dt, z.tower, true);
            if (e->dead) continue;
            e->slowAmount = e->slowTime > 0 ? std::max(e->slowAmount, z.slow) : z.slow;
            e->slowTime = std::max(e->slowTime, 0.3f);
            if (z.vuln) e->vulnTime = std::max(e->vulnTime, 0.3f);
        }
        if (GetRandomValue(0, 3) == 0) {
            Vector2 p = Add(z.at, Mul(Dir(Rand(0, 2 * PI)), z.radius * std::sqrt(Rand(0, 1))));
            Emit(Fx::Ember, p, {255, 200, 90, 255}, 1, 40, 5, 0.7f);
        }
    }
    zones.erase(std::remove_if(zones.begin(), zones.end(), [](const Zone& z) { return z.time >= z.life; }), zones.end());
}

void BattleScene::StepDemo(float dt) {
    demoClock += dt;
    if (embedded && !heroes.empty()) {
        // The database preview frames about four tiles either side of the portrait; targets that
        // walk past it leave quietly instead of crossing the text column.
        float retireX = heroes.front()->Pos().x + kTile * 3.2f;
        for (const auto& e : monsters)
            if (!e->dead && e->Feet().x > retireX) {
                e->dead = true;
                Destroy(e->GetGameObject());
            }
    }
    demoSpawn -= dt;
    if (demoSpawn > 0) return;
    static const EnemyId kParade[] = {EnemyId::Gomora, EnemyId::Baltan, EnemyId::Eleking, EnemyId::Gomora, EnemyId::RedKing, EnemyId::KingJoe};
    EnemyId next = kParade[demoCount++ % (int)std::size(kParade)];
    float start = embedded ? kTile * 8.5f : 0; // two tiles left of the hero, already in range
    SpawnEnemy(next, 0, start);
    if (demoCount % 3 == 0) SpawnEnemy(EnemyId::Gomora, 0, start); // a pair now and then, for chains and splash
    demoSpawn = (EnemyInfo(next).hp > 300 ? 2.2f : 1.2f) * (embedded ? 0.6f : 1.0f);
}

void BattleScene::SetDemoStar(int star) {
    demoStar = Clamp(star, 0, 2);
    if (heroes.empty()) return;
    HeroActor& t = *heroes.front();
    t.level = demoStar;
    t.maxHp = t.hp = HeroMaxHp(t.hero, t.level);
    t.shots = 0;
    t.focus = 0;
    t.dealt = 0;
    t.recoil = 1;
    demoClock = 0;
    AddRing(t.Pos(), 8, 52, 0.5f, 4, Palette::gold);
    Burst(Add(t.Pos(), {0, -14}), Palette::gold, 20, 150, 5, 0.6f);
}

void BattleScene::Knock(MonsterActor& e, float tiles) {
    if (e.dead || e.Def().boss) return;
    e.dist = std::max(0.0f, e.dist - tiles * kTile);
    e.sieging = false;
    e.GetGameObject()->transform->position = map.paths[e.path].Sample(e.dist, &e.dir);
}

MonsterActor* BattleScene::NearestKaiju(Vector2 at, float radius, const std::vector<const MonsterActor*>& skip) const {
    MonsterActor* best = nullptr;
    float bestDist = radius;
    for (MonsterActor* e : KaijuInCircle(at, radius)) {
        if (e->phaseTime > 0 || std::find(skip.begin(), skip.end(), e) != skip.end()) continue;
        float d = Len(Sub(e->Center(), at));
        if (d <= bestDist) {
            bestDist = d;
            best = e;
        }
    }
    return best;
}

void BattleScene::Leak(MonsterActor& e) {
    const EnemyDef& def = e.Def();
    e.dead = true;
    Destroy(e.GetGameObject());
    if (demo) return;
    lives -= def.leak;
    leakCount[(int)e.type]++;
    if (leakWave < 0) leakWave = waveIndex + 1;
    livesPulse = 1;
    baseFlash = 1;
    shake = std::max(shake, def.boss ? 1.2f : 0.4f);
    Text(Add(map.basePos, {0, -60}), TextFormat("-%d", def.leak), Palette::red, 22);
    Burst(map.basePos, {255, 80, 80, 255}, 20, 180, 6, 0.6f);
    Sfx::Play("leak", 0.8f, 0);
}

void BattleScene::Fire(HeroActor& t, MonsterActor& target) {
    const HeroDef& def = HeroInfo(t.hero);
    t.recoil = 1;
    t.flash = 1;
    t.shots++;
    float dmg = HeroDamage(t.hero, t.level);
    float range = HeroRange(t.hero, t.level);
    Vector2 origin = Add(t.Pos(), {0, -24});
    Vector2 aim = target.Center();
    switch (def.kind) {
    case AttackKind::PierceBeam:
        PierceBeam(t, origin, aim, dmg, range * 1.15f, def.beam, t.level >= 2);
        Sfx::Play("beam", 0.4f);
        break;
    case AttackKind::Boomerang: {
        auto p = Launch(ProjectileMotion::Boomerang, origin);
        Vector2 d = Normalize(Sub(aim, origin));
        p->target = Add(origin, Mul(d, std::min(range * 1.1f, Len(Sub(aim, origin)) + 50)));
        p->speed = 480;
        p->hitRadius = 8;
        p->pierce = true;
        int tower = t.id;
        bool breaker = t.level >= 2;
        p->onHit = [this, dmg, tower, breaker](Projectile& self, Collider* c) {
            MonsterActor* e = KaijuOf(c);
            if (!e || !Running()) return;
            DamageEnemy(*e, dmg, tower);
            if (breaker && !e->dead) e->breakTime = 3;
            Sparks(self.Position(), WHITE, 5, 160);
            Sfx::Play("hit", 0.25f);
        };
        p->onStep = [this](Projectile& self) { Emit(Fx::Glow, self.Position(), {255, 210, 90, 255}, 1, 0, 14, 0.22f); };
        p->onDraw = [](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            float spin = self.Age() * 16;
            Draw::BlendAdd();
            Draw::Glow(at, 20, WithAlpha({255, 200, 70, 255}, 0.55f));
            Draw::Glow(at, 8, WithAlpha({255, 60, 70, 255}, 0.35f));
            Draw::BlendNormal();
            Draw::Ellipse(at, 14, 4.2f, {255, 214, 90, 255}, spin);
            Draw::Ellipse(at, 8, 2.4f, {255, 255, 255, 255}, spin);
            DrawRectangleRec({at.x - 2, at.y - 5, 4, 8}, {70, 74, 86, 255});
        };
        Sfx::Play("slugger", 0.35f);
        break;
    }
    case AttackKind::Snipe: {
        bool crit = t.level >= 2 && Rand(0, 1) < 0.25f;
        AddBeam(origin, aim, crit ? 15.0f : 10.0f, 0.42f, def.beam, WHITE);
        AddBeam(origin, aim, 5, 0.42f, {190, 120, 255, 255}, WHITE);
        DamageEnemy(target, crit ? dmg * 2.5f : dmg, t.id);
        Burst(aim, def.beam, 16, 180, 5, 0.5f);
        Burst(aim, WHITE, 8, 90, 6, 0.28f);
        Burst(aim, {190, 120, 255, 255}, 8, 140, 4, 0.4f);
        AddRing(aim, 4, 40, 0.4f, 3, def.beam);
        AddRing(aim, 8, 28, 0.32f, 2, WHITE);
        Sparks(aim, {255, 180, 255, 255}, 6, 160);
        if (crit) Text(Add(aim, {0, -26}), "临界！", {255, 180, 80, 255}, 18);
        Sfx::Play("snipe", 0.45f);
        break;
    }
    case AttackKind::Pulse: {
        // Wrecking Burst: charging throws off a red-black force field, then a blue-white beam fires.
        float slow = t.level >= 2 ? 0.6f : def.slow;
        Color field{230, 40, 52, 255}, dark{96, 16, 42, 255};
        for (MonsterActor* e : KaijuInCircle(t.Pos(), range)) {
            DamageEnemy(*e, dmg, t.id);
            if (e->dead) continue;
            e->slowAmount = e->slowTime > 0 ? std::max(e->slowAmount, slow) : slow;
            e->slowTime = 1.8f;
            if (t.level >= 2) e->vulnTime = 2.0f;
            Sparks(e->Center(), field, 3, 90);
        }
        Vector2 core = Add(t.Pos(), {0, 4});
        for (int i = 0; i < 5; ++i) Bolt(core, Add(core, Mul(Dir(Rand(0, 2 * PI)), range * Rand(0.5f, 0.95f))), i % 2 ? dark : field);
        Burst(core, field, 16, 190, 5, 0.4f);
        Burst(core, dark, 8, 110, 6, 0.3f);
        AddRing(core, 8, range, 0.5f, 5, field);
        AddRing(core, 6, range * 0.7f, 0.4f, 3, dark);
        AddBeam(origin, aim, 7, 0.3f, def.beam, WHITE);
        Bolt(origin, aim, field);
        Burst(aim, def.beam, 10, 150, 5, 0.35f);
        Sfx::Play("pulse", 0.3f);
        break;
    }
    case AttackKind::Splash: {
        auto p = Launch(ProjectileMotion::Homing, origin);
        p->follow = Follow(target);
        p->followOffset = {0, -target.Height() * 0.45f};
        p->target = aim;
        p->speed = 360;
        p->maxLife = 3;
        float splash = def.splash * kTile * (1 + 0.12f * t.level);
        int tower = t.id;
        bool burn = t.level >= 2;
        static const Color kArcHues[] = {{255, 170, 50, 255}, {80, 220, 255, 255}, {255, 80, 170, 255}, {150, 255, 120, 255}};
        Color hue = kArcHues[t.shots % 4];
        p->onArrive = [this, dmg, splash, tower, burn, hue](Projectile&, Vector2 at) {
            if (Running()) Explode(at, splash, dmg, tower, burn, false, hue);
        };
        p->onStep = [this, hue](Projectile& self) {
            Emit(Fx::Glow, self.Position(), hue, 1, 20, 16, 0.3f);
            if (GetRandomValue(0, 2) == 0) Emit(Fx::Spark, self.Position(), SpinHue(hue, 1), 1, 40, 3, 0.25f);
        };
        p->onDraw = [](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            float spin = self.Age() * 5;
            static const Color hues[] = {{255, 170, 50, 255}, {80, 220, 255, 255}, {255, 80, 170, 255}, {170, 255, 140, 255}};
            Color c = hues[(int)(self.Age() * 8) % 4];
            Draw::BlendAdd();
            Draw::Glow(at, 20, WithAlpha(c, 0.75f));
            Draw::Glow(at, 8, WithAlpha(WHITE, 0.9f));
            Draw::BlendNormal();
            Draw::Star(at, 9, 4, spin, c);
        };
        Sfx::Play("fire", 0.35f);
        break;
    }
    case AttackKind::Twin: {
        for (float side : {-1.0f, 1.0f}) {
            auto p = Launch(ProjectileMotion::Homing, Add(origin, {side * 10, -6}));
            p->follow = Follow(target);
            p->followOffset = {0, -target.Height() * 0.45f};
            p->target = aim;
            p->speed = 640;
            p->maxLife = 3;
            std::weak_ptr<MonsterActor> victim = target.GetGameObject()->GetComponent<MonsterActor>();
            int tower = t.id;
            p->onArrive = [this, victim, dmg, tower](Projectile&, Vector2 at) {
                if (!Running()) return;
                if (auto e = victim.lock(); e && !e->dead) DamageEnemy(*e, dmg, tower);
                Sparks(at, HeroInfo(HeroId::Zero).beam, 6, 180);
                Sparks(at, {255, 70, 80, 255}, 4, 140);
                Sfx::Play("hit", 0.2f);
            };
            p->onStep = [this](Projectile& self) { Emit(Fx::Glow, self.Position(), {120, 255, 220, 255}, 1, 0, 10, 0.16f); };
            p->onDraw = [side](const Projectile& self) {
                Vector2 at = self.RenderPosition();
                Draw::BlendAdd();
                Draw::Glow(at, 12, WithAlpha({90, 255, 180, 255}, 0.55f));
                Draw::Glow(at, 5, WithAlpha({255, 60, 70, 255}, 0.4f));
                Draw::BlendNormal();
                Draw::Ellipse(at, 9, 2.6f, {230, 248, 255, 255}, self.Age() * 24 * side);
                Draw::Ellipse(at, 5, 1.4f, {255, 70, 80, 255}, self.Age() * 24 * side);
            };
        }
        if (t.level >= 2 && t.shots % 5 == 0) {
            PierceBeam(t, origin, aim, dmg * 3, range * 1.2f, {90, 255, 160, 255}, false);
            Text(Add(origin, {0, -30}), "艾梅利姆切割", {90, 255, 160, 255}, 13);
        }
        Sfx::Play("zero", 0.3f);
        break;
    }
    case AttackKind::Multi: {
        // Three shots per type: Multi (piercing Zeperion Ray), Power (Delacium Light Flux), Sky (Runboldt bolts).
        int sky = t.level >= 2 ? 5 : 3;
        int step = (t.shots - 1) % (6 + sky);
        int form = step < 3 ? 0 : step < 6 ? 1 : 2;
        static const char* kForms[] = {"复合型", "强力型", "空中型"};
        static const Color kTones[] = {{200, 150, 255, 255}, {255, 96, 40, 255}, {80, 170, 255, 255}};
        Color tone = kTones[form];
        if (step == 0 || step == 3 || step == 6) Text(Add(origin, {0, -30}), kForms[form], tone, 13);
        if (form == 0) {
            PierceBeam(t, origin, aim, dmg, range * 1.1f, tone, false);
            Sfx::Play("beam", 0.4f);
        } else if (form == 1) {
            AddBeam(origin, aim, 13, 0.36f, tone, {255, 230, 170, 255});
            DamageEnemy(target, dmg * 2.2f, t.id);
            if (!target.dead) target.breakTime = std::max(target.breakTime, 2.5f);
            Burst(aim, tone, 18, 190, 6, 0.45f);
            Burst(aim, {255, 220, 120, 255}, 8, 100, 7, 0.3f);
            AddRing(aim, 6, 38, 0.38f, 4, tone);
            Sfx::Play("snipe", 0.4f);
        } else {
            AddBeam(origin, aim, 4, 0.18f, tone, WHITE);
            DamageEnemy(target, dmg * 0.75f, t.id);
            Sparks(aim, tone, 6, 170);
            t.nextCooldownScale = 0.45f;
            Sfx::Play("hit", 0.3f);
        }
        break;
    }
    case AttackKind::Vortex: {
        auto p = Launch(ProjectileMotion::Homing, origin);
        p->follow = Follow(target);
        p->followOffset = {0, -target.Height() * 0.45f};
        p->target = aim;
        p->speed = 560;
        p->maxLife = 3;
        float radius = def.splash * kTile * (t.level >= 2 ? 1.3f : 1.0f);
        float pull = kTile * (t.level >= 2 ? 0.82f : 0.55f);
        int tower = t.id;
        p->onArrive = [this, dmg, radius, pull, tower](Projectile&, Vector2 at) {
            if (!Running()) return;
            Color blue{110, 170, 255, 255}, violet{150, 90, 255, 255};
            for (MonsterActor* e : KaijuInCircle(at, radius)) {
                Vector2 from = e->Center();
                DamageEnemy(*e, dmg, tower);
                if (e->dead) continue;
                e->dist = std::max(0.0f, e->dist - pull * (e->Def().boss ? 0.3f : 1.0f));
                e->GetGameObject()->transform->position = map.paths[e->path].Sample(e->dist);
                e->stunTime = std::max(e->stunTime, 0.25f);
                AddBeam(from, at, 2.5f, 0.25f, violet, WHITE);
            }
            AddRing(at, radius, 6, 0.4f, 5, violet);
            AddRing(at, radius * 0.7f, 4, 0.3f, 3, blue);
            Burst(at, blue, 16, 150, 5, 0.4f);
            Burst(at, WHITE, 6, 60, 7, 0.25f);
            Sfx::Play("teleport", 0.35f);
        };
        p->onStep = [this](Projectile& self) { Emit(Fx::Glow, self.Position(), {110, 170, 255, 255}, 1, 0, 14, 0.2f); };
        p->onDraw = [](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            Draw::BlendAdd();
            Draw::Glow(at, 22, WithAlpha({90, 150, 255, 255}, 0.7f));
            Draw::Glow(at, 9, WithAlpha(WHITE, 0.9f));
            Draw::BlendNormal();
        };
        Sfx::Play("pulse", 0.25f);
        break;
    }
    case AttackKind::Spear: {
        Vector2 d = Normalize(Sub(aim, origin));
        constexpr float kSpeed = 820;
        auto p = Launch(ProjectileMotion::Straight, origin);
        p->velocity = Mul(d, kSpeed);
        p->hitRadius = 12;
        p->pierce = true;
        p->maxLife = range * 1.3f / kSpeed;
        int tower = t.id;
        float splash = def.splash * kTile;
        auto burst = std::make_shared<bool>(t.level >= 2);
        p->onHit = [this, dmg, tower, splash, burst](Projectile& self, Collider* c) {
            MonsterActor* e = KaijuOf(c);
            if (!e || !Running()) return;
            DamageEnemy(*e, dmg, tower);
            Sparks(self.Position(), {255, 80, 80, 255}, 4, 160);
            Sparks(self.Position(), {90, 170, 255, 255}, 4, 160);
            if (*burst) {
                *burst = false;
                Explode(self.Position(), splash, dmg * 0.6f, tower, false, false, {200, 80, 255, 255});
            }
            Sfx::Play("hit", 0.25f);
        };
        p->onStep = [this](Projectile& self) {
            bool red = std::fmod(self.Age() * 20, 1.0f) < 0.5f;
            Emit(Fx::Glow, self.Position(), red ? Color{255, 80, 80, 255} : Color{80, 160, 255, 255}, 1, 0, 10, 0.2f);
        };
        p->onDraw = [d](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            Vector2 n{-d.y, d.x};
            Vector2 tail = Sub(at, Mul(d, 34)), tip = Add(at, Mul(d, 10));
            Draw::BlendAdd();
            Draw::Glow(at, 18, WithAlpha({200, 200, 255, 255}, 0.4f));
            Draw::BlendNormal();
            for (int strand = 0; strand < 2; ++strand) {
                Color c = strand ? Color{90, 170, 255, 255} : Color{255, 70, 70, 255};
                Vector2 prev{};
                for (int k = 0; k <= 8; ++k) {
                    float u = k / 8.0f;
                    float wave = std::sin(u * 3 * PI + self.Age() * 30 + strand * PI) * 4;
                    Vector2 q = Add(Lerp(tail, tip, u), Mul(n, wave));
                    if (k) Draw::Line(prev, q, 2.2f, c);
                    prev = q;
                }
            }
            Draw::Line(tail, tip, 1.6f, WHITE);
            Draw::Tri(Add(tip, Mul(d, 8)), Add(tip, Mul(n, 5)), Sub(tip, Mul(n, 5)), WHITE);
        };
        Sfx::Play("slugger", 0.35f);
        break;
    }
    default:
        FireSignature(t, target, dmg, range, origin, aim);
        break;
    }
}

void BattleScene::FireSignature(HeroActor& t, MonsterActor& target, float dmg, float range, Vector2 origin, Vector2 aim) {
    const HeroDef& def = HeroInfo(t.hero);
    int tower = t.id;
    switch (def.kind) {
    case AttackKind::Slash: {
        if (t.level >= 2 && t.shots % 4 == 0) {
            // Specium Ray: arms crossed, a wide white-blue stream.
            PierceBeam(t, origin, aim, dmg * 3, range * 1.2f, {150, 220, 255, 255}, false);
            AddBeam(origin, Add(origin, Mul(Normalize(Sub(aim, origin)), range * 1.2f)), 18, 0.45f, WithAlpha({150, 220, 255, 255}, 0.6f), WHITE);
            Text(Add(origin, {0, -34}), "斯派修姆光线", {170, 230, 255, 255}, 14);
            Sfx::Play("beam", 0.5f);
            break;
        }
        // Ultra Slash: a spinning saw disc that flies straight through every kaiju in line.
        Vector2 d = Normalize(Sub(aim, origin));
        constexpr float kSpeed = 600;
        auto p = Launch(ProjectileMotion::Straight, Add(origin, {0, -16}));
        p->velocity = Mul(d, kSpeed);
        p->hitRadius = 14;
        p->pierce = true;
        p->maxLife = range * 1.3f / kSpeed;
        p->onHit = [this, dmg, tower, d](Projectile& self, Collider* c) {
            MonsterActor* e = KaijuOf(c);
            if (!e || !Running()) return;
            Vector2 at = e->Center(), n{-d.y, d.x};
            DamageEnemy(*e, dmg, tower);
            AddBeam(Sub(at, Mul(n, 18)), Add(at, Mul(n, 18)), 3, 0.22f, {150, 220, 255, 255}, WHITE);
            Sparks(self.Position(), WHITE, 6, 200);
            Sparks(self.Position(), {150, 220, 255, 255}, 4, 160);
            Sfx::Play("hit", 0.25f);
        };
        p->onStep = [this](Projectile& self) { Emit(Fx::Glow, self.Position(), {150, 220, 255, 255}, 1, 0, 12, 0.18f); };
        p->onDraw = [](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            float spin = self.Age() * 26;
            auto flat = [at](Vector2 v) { return Vector2{at.x + v.x, at.y + v.y * 0.62f}; };
            Draw::BlendAdd();
            Draw::Glow(at, 24, WithAlpha({140, 210, 255, 255}, 0.55f));
            Draw::BlendNormal();
            for (int i = 0; i < 12; ++i) {
                float a = spin + i * PI / 6;
                Draw::Tri(flat(Mul(Dir(a), 9)), flat(Mul(Dir(a + 0.42f), 9)), flat(Mul(Dir(a + 0.12f), 16)), {210, 240, 255, 255});
            }
            Draw::Ellipse(at, 10, 6.2f, {190, 232, 255, 240}, 0);
            Draw::Ellipse(at, 5, 3.1f, WHITE, 0);
        };
        Sfx::Play("slugger", 0.35f);
        break;
    }
    case AttackKind::Infinity: {
        Color gold{255, 190, 80, 255}, red{255, 70, 50, 255};
        // The Mebius Brace traces an infinity sign before every shot.
        Vector2 sign = Add(origin, {0, -18});
        for (int k = 0; k < 20; ++k) {
            float u = k * 2 * PI / 20, s = std::sin(u), q = 1 + s * s;
            Vector2 p = Add(sign, {16 * std::cos(u) / q, 16 * s * std::cos(u) / q});
            Emit(Fx::Glow, p, k % 2 ? gold : red, 1, 6, 6, 0.35f);
        }
        int period = t.level >= 2 ? 3 : 4;
        if (t.shots % period == 0) {
            // Mebium Burst: Burning Brave hurls a sun-like fireball.
            auto p = Launch(ProjectileMotion::Homing, Add(origin, {0, -6}));
            p->follow = Follow(target);
            p->followOffset = {0, -target.Height() * 0.45f};
            p->target = aim;
            p->speed = 380;
            p->maxLife = 3;
            float splash = def.splash * kTile * 1.25f;
            bool burn = t.level >= 2;
            p->onArrive = [this, dmg, splash, tower, burn](Projectile&, Vector2 at) {
                if (Running()) Explode(at, splash, dmg * 2.2f, tower, burn, false, {255, 110, 40, 255});
            };
            p->onStep = [this](Projectile& self) {
                Emit(Fx::Ember, self.Position(), {255, 140, 40, 255}, 1, 50, 8, 0.4f);
                Emit(Fx::Glow, self.Position(), {255, 90, 30, 255}, 1, 10, 18, 0.25f);
            };
            p->onDraw = [](const Projectile& self) {
                Vector2 at = self.RenderPosition();
                float pulse = 1 + 0.12f * std::sin(self.Age() * 30);
                Draw::BlendAdd();
                Draw::Glow(at, 34 * pulse, WithAlpha({255, 90, 30, 255}, 0.8f));
                Draw::Glow(at, 16, WithAlpha({255, 230, 160, 255}, 0.95f));
                for (int i = 0; i < 6; ++i) {
                    float a = self.Age() * 9 + i * PI / 3;
                    Draw::Glow(Add(at, Mul(Dir(a), 14)), 6, WithAlpha({255, 160, 60, 255}, 0.8f));
                }
                Draw::BlendNormal();
            };
            Text(Add(origin, {0, -36}), "梦比姆爆裂", {255, 150, 60, 255}, 14);
            Sfx::Play("fire", 0.45f);
            break;
        }
        AddBeam(origin, aim, 9, 0.3f, gold, {255, 250, 220, 255});
        AddBeam(origin, aim, 3, 0.3f, red, WHITE);
        DamageEnemy(target, dmg, tower);
        Burst(aim, gold, 12, 170, 5, 0.4f);
        AddRing(aim, 4, 30, 0.3f, 3, gold);
        Sfx::Play("beam", 0.35f);
        break;
    }
    case AttackKind::Kick: {
        Color flame{255, 130, 40, 255}, hot{255, 230, 160, 255};
        auto kick = [&](MonsterActor& e, float scale) {
            Vector2 at = e.Center();
            Vector2 from = Add(t.Pos(), {0, -40});
            AddBeam(from, at, 10, 0.25f, flame, hot);
            for (int i = 0; i < 6; ++i) Emit(Fx::Ember, Lerp(from, at, Rand(0, 1)), flame, 1, 50, 6, 0.4f);
            DamageEnemy(e, dmg * scale, tower);
            for (MonsterActor* other : KaijuInCircle(at, def.splash * kTile))
                if (other != &e) DamageEnemy(*other, dmg * scale * 0.4f, tower);
            if (!e.dead) {
                e.stunTime = std::max(e.stunTime, t.level >= 2 ? 1.0f : 0.5f);
                Knock(e, 0.35f);
            }
            Burst(at, flame, 18, 220, 6, 0.4f);
            Burst(at, hot, 8, 100, 7, 0.25f);
            AddRing(at, 6, 40, 0.3f, 4, flame);
            Smoke(at, 3, 10);
        };
        t.lunge = 1;
        t.lungeTo = target.Feet();
        kick(target, 1);
        if (t.level >= 2)
            if (MonsterActor* second = NearestKaiju(aim, range, {&target})) {
                kick(*second, 0.7f);
                Text(Add(origin, {0, -36}), "双重飞踢", flame, 13);
            }
        shake = std::max(shake, 0.15f);
        Sfx::Play("slugger", 0.4f);
        break;
    }
    case AttackKind::Chain: {
        // Ginga Thunderbolt: the arms wind a lightning vortex, then it leaps from kaiju to kaiju.
        Color yellow{255, 236, 90, 255}, cyan{120, 220, 255, 255};
        AddRing(origin, 4, 26, 0.3f, 3, yellow);
        for (int i = 0; i < 6; ++i) Emit(Fx::Glow, Add(origin, Mul(Dir(i * PI / 3 + simTime * 6), 14)), cyan, 1, 30, 6, 0.3f);
        int hops = t.level >= 2 ? 6 : 4;
        std::vector<const MonsterActor*> struck;
        MonsterActor* current = &target;
        Vector2 from = origin;
        float amount = dmg;
        for (int i = 0; i < hops && current; ++i) {
            Vector2 at = current->Center();
            Bolt(from, at, yellow);
            Bolt(from, at, cyan);
            Sparks(at, yellow, 6, 180);
            struck.push_back(current);
            DamageEnemy(*current, amount, tower);
            if (!current->dead && t.level >= 2) current->stunTime = std::max(current->stunTime, 0.3f);
            from = at;
            amount *= 0.75f;
            current = NearestKaiju(at, kTile * 1.8f, struck);
        }
        Sfx::Play("pulse", 0.3f);
        break;
    }
    case AttackKind::Edge: {
        // Photon Edge: a whip of light flung from the crest sweeps a cone.
        Color red{255, 90, 60, 255}, gold{255, 210, 120, 255};
        Vector2 d = Normalize(Sub(aim, origin));
        float base = std::atan2(d.y, d.x);
        constexpr float kHalf = 35 * DEG2RAD;
        std::vector<MonsterActor*> hit;
        for (MonsterActor* e : KaijuInCircle(t.Pos(), range)) {
            Vector2 to = Sub(e->Center(), origin);
            float diff = std::atan2(to.y, to.x) - base;
            diff = std::atan2(std::sin(diff), std::cos(diff));
            if (e == &target || std::fabs(diff) <= kHalf) hit.push_back(e);
        }
        auto ribbon = [&](Vector2 end, float width) {
            Vector2 n{-(end.y - origin.y), end.x - origin.x};
            n = Normalize(n);
            Vector2 prev = origin;
            for (int k = 1; k <= 10; ++k) {
                float u = k / 10.0f;
                Vector2 q = Add(Lerp(origin, end, u), Mul(n, std::sin(u * 3 * PI + simTime * 10) * 10 * (1 - u * 0.5f)));
                AddBeam(prev, q, width, 0.28f, k % 2 ? red : gold, {255, 245, 220, 255});
                prev = q;
            }
        };
        ribbon(Add(aim, Mul(d, 20)), 7);
        Vector2 prev{};
        for (int k = -5; k <= 5; ++k) {
            Vector2 q = Add(origin, Mul(Dir(base + k * kHalf / 5), range * 0.8f));
            if (k > -5) AddBeam(prev, q, 3, 0.25f, WithAlpha(red, 0.7f), gold);
            prev = q;
        }
        for (MonsterActor* e : hit) {
            if (e != &target) ribbon(e->Center(), 3.5f);
            Vector2 at = e->Center();
            DamageEnemy(*e, dmg, tower);
            Sparks(at, gold, 5, 170);
            if (t.level >= 2) Knock(*e, 0.25f);
        }
        Sfx::Play("snipe", 0.35f);
        break;
    }
    case AttackKind::Field: {
        // Meta Field: a golden dimension that pins kaiju down.
        Vector2 at = target.Feet();
        zones.push_back({at, def.splash * kTile * (1 + 0.1f * t.level), dmg, def.slow, 0, 3.5f, tower, t.level >= 2});
        AddBeam(origin, Add(at, {0, -10}), 5, 0.35f, {255, 200, 100, 255}, WHITE);
        AddRing(at, 6, def.splash * kTile, 0.45f, 4, {255, 210, 110, 255});
        Burst(at, {255, 210, 110, 255}, 16, 150, 5, 0.5f);
        Sfx::Play("shield", 0.35f);
        break;
    }
    case AttackKind::Dual: {
        int period = t.level >= 2 ? 2 : 3;
        if (t.shots % period == 0) {
            // Strong Type: the Garnate Bomber, a red fireball that stuns where it lands.
            Color red{255, 80, 40, 255};
            auto p = Launch(ProjectileMotion::Lob, origin);
            p->target = target.Feet();
            p->flightTime = 0.5f;
            float splash = def.splash * kTile;
            p->onArrive = [this, dmg, splash, tower, red](Projectile&, Vector2 at) {
                if (!Running()) return;
                Explode(at, splash, dmg * 2.2f, tower, false, false, red);
                for (MonsterActor* e : KaijuInCircle(at, splash)) e->stunTime = std::max(e->stunTime, 0.6f);
            };
            p->onStep = [this, red](Projectile& self) { Emit(Fx::Ember, self.Position(), red, 1, 40, 7, 0.35f); };
            p->onDraw = [](const Projectile& self) {
                Vector2 at = self.RenderPosition();
                Draw::Shadow(self.target, 8, 4, 0.35f);
                Draw::BlendAdd();
                Draw::Glow(at, 26, WithAlpha({255, 70, 30, 255}, 0.85f));
                Draw::Glow(at, 10, WithAlpha({255, 230, 170, 255}, 0.95f));
                Draw::BlendNormal();
            };
            AddRing(Add(t.Pos(), {0, -20}), 6, 34, 0.3f, 4, red);
            Text(Add(origin, {0, -34}), "强壮型", red, 13);
            Sfx::Play("fire", 0.4f);
            break;
        }
        // Flash Type: the Solgent Ray from crossed arms.
        Color blue{130, 200, 255, 255};
        AddBeam(origin, aim, 9, 0.28f, blue, WHITE);
        AddBeam(origin, aim, 3, 0.28f, {255, 90, 90, 255}, WHITE);
        DamageEnemy(target, dmg, tower);
        Burst(aim, blue, 10, 160, 5, 0.35f);
        Sfx::Play("beam", 0.3f);
        break;
    }
    case AttackKind::Focus: {
        int cap = t.level >= 2 ? 6 : 4;
        if (t.focusTarget == &target) t.focus = std::min(t.focus + 1, cap);
        else t.focus /= 2; // switching targets keeps half the charge
        t.focusTarget = &target;
        float mult = 1 + 0.3f * t.focus;
        Color tone = Mix({90, 160, 255, 255}, {255, 214, 90, 255}, t.focus / (float)cap);
        if (t.level >= 2 && t.focus == cap) {
            PierceBeam(t, origin, aim, dmg * mult, range * 1.2f, tone, false);
            if (t.shots % 3 == 0) Text(Add(origin, {0, -34}), "全力光线", tone, 13);
        } else {
            AddBeam(origin, aim, 6 + t.focus * 1.6f, 0.3f, tone, WHITE);
            DamageEnemy(target, dmg * mult, tower);
            Burst(aim, tone, 8 + t.focus * 3, 160, 5, 0.35f);
        }
        if (t.focus > 0) AddRing(aim, 4, 18 + t.focus * 5, 0.25f, 2, tone);
        Sfx::Play("beam", 0.3f + 0.03f * t.focus);
        break;
    }
    default:
        break;
    }
}

void BattleScene::PierceBeam(HeroActor& t, Vector2 origin, Vector2 toward, float damage, float range, Color glow, bool burn) {
    Vector2 d = Normalize(Sub(toward, origin));
    Vector2 end = Add(origin, Mul(d, range));
    for (const SegmentHit& hit : GetPhysicsWorld().SegmentCast(origin, end, 8, kKaijuMask)) {
        MonsterActor* e = KaijuOf(hit.collider);
        if (!e) continue;
        DamageEnemy(*e, damage, t.id);
        if (burn && !e->dead) {
            e->burnTime = 2;
            e->burnDps = std::max(e->burnDps, damage * 0.3f);
        }
        Sparks(hit.point, glow, 4, 150);
    }
    AddBeam(origin, end, 11, 0.36f, glow, WHITE);
    AddBeam(origin, end, 4, 0.36f, SpinHue(glow, 1), WHITE);
    Burst(end, glow, 12, 160, 5, 0.35f);
    Burst(end, WHITE, 6, 80, 6, 0.22f);
    AddRing(end, 4, 28, 0.3f, 2, glow);
    for (int i = 0; i < 6; ++i) Emit(Fx::Glow, Lerp(origin, end, Rand(0, 1)), glow, 1, 60, 6, 0.35f);
}

void BattleScene::Explode(Vector2 at, float radius, float damage, int towerId, bool burn, bool stun, Color hue) {
    for (MonsterActor* e : KaijuInCircle(at, radius)) {
        DamageEnemy(*e, damage, towerId);
        if (e->dead) continue;
        if (burn) {
            e->burnTime = 2.5f;
            e->burnDps = std::max(e->burnDps, damage * 0.2f);
        }
        if (stun) e->stunTime = std::max(e->stunTime, 0.9f);
    }
    Color hot = Lighten(hue, 0.45f);
    Color alt = SpinHue(hue, 1);
    Burst(at, hue, 26, 240, 7, 0.55f);
    Burst(at, hot, 12, 110, 9, 0.32f);
    Burst(at, alt, 16, 180, 5, 0.48f);
    Sparks(at, WHITE, 8, 200);
    Smoke(at, 8, 16);
    AddRing(at, 8, radius, 0.42f, 5, hue);
    AddRing(at, 4, radius * 0.7f, 0.55f, 3, hot);
    AddRing(at, 12, radius * 1.15f, 0.7f, 4, alt);
    Sfx::Play("explode", 0.5f);
    if (stun) shake = std::max(shake, 0.35f); // airstrikes only; Arc fires too often to shake every hit
}

void BattleScene::DamageEnemy(MonsterActor& e, float amount, int towerId, bool ignoreArmor) {
    if (e.dead || amount <= 0 || e.phaseTime > 0) return;
    float armor = (e.breakTime > 0 ? e.armor * 0.4f : e.armor) + (e.buffTime > 0 ? 4.0f : 0.0f);
    float dmg = ignoreArmor ? amount : std::max(amount * 0.25f, amount - armor);
    if (e.vulnTime > 0) dmg *= 1.2f;
    if (e.shieldTime > 0) dmg *= kShieldTaken;
    e.hp -= dmg;
    if (amount > 2) e.flash = 0.7f;
    if (HeroActor* t = FindTower(towerId)) t->dealt += dmg;
    if (e.hp <= 0) KillEnemy(e, towerId);
}

void BattleScene::KillEnemy(MonsterActor& e, int towerId) {
    const EnemyDef& def = e.Def();
    e.dead = true;
    Destroy(e.GetGameObject());
    gold += def.reward;
    kills++;
    if (HeroActor* t = FindTower(towerId)) t->kills++;
    Vector2 c = e.Center();
    Text(Add(c, {0, -14}), TextFormat("+%d", def.reward), Palette::gold, def.boss ? 24.0f : 15.0f);
    Color alt = SpinHue(def.color, 1);
    Burst(c, Lighten(def.color, 0.35f), def.boss ? 48 : 14, def.boss ? 280.0f : 160.0f, def.boss ? 8.0f : 5.0f, 0.55f);
    Burst(c, alt, def.boss ? 28 : 8, 200, 6, 0.45f);
    Burst(c, {255, 220, 140, 255}, def.boss ? 20 : 6, 120, 7, 0.3f);
    Smoke(e.Feet(), def.boss ? 14 : 5, def.boss ? 20.0f : 10.0f);
    AddRing(c, 4, 36 * e.Scale(), 0.4f, 3, Lighten(def.color, 0.4f));
    AddRing(c, 8, 58 * e.Scale(), 0.55f, 3, alt);
    if (!headless) {
        auto object = CreateGameObject("corpse");
        object->transform->position = e.Feet();
        auto corpse = object->AddComponent<CorpseActor>();
        corpse->type = e.type;
        corpse->face = e.face;
        corpse->scale = e.Scale();
        corpse->ySort = true;
        object->AddComponent<DestroyAfter>(1.1f);
    }
    Sfx::Play("explode", def.boss ? 0.9f : 0.4f);
    if (e.type == EnemyId::Baltan) {
        SpawnEnemy(EnemyId::BaltanClone, e.path, std::max(0.0f, e.dist - 12));
        SpawnEnemy(EnemyId::BaltanClone, e.path, e.dist + 12);
        Sfx::Play("split", 0.4f);
    }
    if (def.boss) {
        shake = 1.3f;
        flashScreen = 0.6f;
        Banner(TextFormat("击败 %s！", def.name), TextFormat("获得 %d 金币", def.reward), Palette::gold);
        for (int i = 0; i < 4; ++i) AddRing(c, 10.0f + i * 8, 120.0f + i * 40, 0.6f + i * 0.15f, 5, {255, 230, 160, 255});
    }
    EventBus::Emit("ug.kill", EventData{def.reward});
}

void BattleScene::StepEffects(float dt) {
    flashScreen = std::max(0.0f, flashScreen - dt * 1.6f);
    shake = std::max(0.0f, shake - dt * 1.8f);
    baseFlash = std::max(0.0f, baseFlash - dt * 2);
}

// ================================================================ skills

void BattleScene::CastBrothers() {
    if (brothersCooldown > 0 || !Running() || waveIndex < 0) return;
    brothersCooldown = kBrothersMax;
    float dmg = 260 + 55.0f * std::max(0, waveIndex);
    int i = 0;
    for (const auto& e : Alive()) {
        Color c = HeroInfo(roster[i++ % roster.size()]).beam;
        Vector2 cc = e->Center();
        AddBeam({cc.x + Rand(-30, 30), kMapY - 20}, cc, 13, 0.75f, c, WHITE);
        Burst(cc, c, 10, 160, 5, 0.5f);
        DamageEnemy(*e, e->Def().boss ? dmg * 0.5f : dmg, -1, true);
    }
    flashScreen = 1;
    shake = std::max(shake, 0.8f);
    Banner("奥特兄弟 · 合体光线", "全场怪兽受到巨大伤害！", Palette::cyan);
    Sfx::Play("skill", 0.9f, 0);
}

void BattleScene::CastStrike(Vector2 target) {
    if (strikeCooldown > 0 || !Running()) return;
    strikeCooldown = kStrikeMax;
    strikes.push_back({target, 0, 0});
    Sfx::Play("jet", 0.8f, 0);
}

int BattleScene::RosterCount() const {
    int count = 0;
    for (HeroId id : roster)
        if (std::any_of(heroes.begin(), heroes.end(), [id](const Ref<HeroActor>& h) { return h->hero == id; })) ++count;
    return count;
}

bool BattleScene::RosterComplete() const { return RosterCount() >= (int)roster.size(); }

void BattleScene::CastFusion(bool force) {
    if (fusionCooldown > 0 || !Running() || waveIndex < 0 || fusionAnim >= 0) return;
    if (!force && !RosterComplete()) return;
    fusionCooldown = fusionSpan = kFusionMax;
    Sfx::Play("fusion", 1.0f, 0);
    if (headless) return FusionImpact();
    // The battle freezes while the cut-in plays; FusionImpact lands at kFusionImpactAt.
    fusionAnim = 0;
    placing = -1;
    targetingStrike = false;
    ApplySpeed();
}

void BattleScene::UpdateFusion(float real) {
    if (fusionAnim < 0 || paused) return;
    float before = fusionAnim;
    fusionAnim += real;
    if (before < kFusionImpactAt && fusionAnim >= kFusionImpactAt) {
        FusionImpact();
        ApplySpeed();
    }
    if (fusionAnim >= kFusionEnd) fusionAnim = -1;
}

void BattleScene::FusionImpact() {
    Vector2 center{kMapX + kMapW * 0.5f, kMapY + kMapH * 0.45f};
    static const Color hues[] = {{255, 70, 70, 255},  {255, 196, 60, 255},  {80, 255, 170, 255},
                                 {70, 170, 255, 255}, {190, 110, 255, 255}, {255, 120, 210, 255}};
    int beam = 0;
    for (const auto& hero : heroes) {
        Color c = HeroInfo(hero->hero).beam;
        AddBeam(Add(hero->Pos(), {0, -28}), center, 8, 0.9f, c, WHITE);
        if (++beam >= 10) break;
    }
    AddBeam({center.x, kMapY - 30}, {center.x, kMapY + kMapH}, 40, 1.3f, {255, 244, 210, 255}, WHITE);
    auto alive = Alive();
    int shown = 0;
    for (const auto& enemy : alive) {
        Color c = hues[shown % 6];
        Vector2 at = enemy->Center();
        if (shown < 16) AddBeam(center, at, 12, 0.5f, c, WHITE);
        float amount = std::max(2600.0f, enemy->maxHp * 0.68f);
        DamageEnemy(*enemy, amount, -1, true);
        if (!enemy->dead) enemy->stunTime = std::max(enemy->stunTime, 1.5f);
        Burst(at, c, 14, 200, 6, 0.4f);
        ++shown;
    }
    for (int i = 0; i < 6; ++i) {
        AddRing(center, 10, 90.0f + i * 78, 0.7f + i * 0.12f, 7, hues[i]);
        AddRing(center, 20, 140.0f + i * 90, 1.0f + i * 0.12f, 3, WHITE);
        Burst(center, hues[i], 22, 300, 8, 0.7f);
    }
    Burst(center, WHITE, 30, 160, 12, 0.5f);
    Smoke(center, 14, 26);
    flashScreen = 1.6f;
    shake = 2.0f;
}

// ================================================================ towers

void BattleScene::PlaceTower(int col, int row) {
    if (!BuyTower((HeroId)placing, col, row)) return;
    if (!Input::GetKey(KEY_LEFT_SHIFT) && !Input::GetKey(KEY_RIGHT_SHIFT)) placing = -1;
    selected = -1;
}

bool BattleScene::BuyTower(HeroId hero, int col, int row) {
    const HeroDef& def = HeroInfo(hero);
    Vector2 at = TileCenter(col, row);
    if (!map.Buildable(col, row) || towerGrid[row][col] >= 0) {
        Sfx::Play("error", 0.5f);
        return false;
    }
    if (gold < def.cost) {
        Sfx::Play("error", 0.5f);
        Text(at, "金币不足", Palette::red, 15);
        return false;
    }
    gold -= def.cost;
    auto object = CreateGameObject("hero");
    object->transform->position = at;
    auto t = object->AddComponent<HeroActor>();
    t->battle = this;
    t->id = nextTowerId++;
    t->hero = hero;
    t->col = col;
    t->row = row;
    t->invested = def.cost;
    t->hp = t->maxHp = HeroMaxHp(hero, 0);
    t->mode = def.kind == AttackKind::Snipe ? 1 : 0;
    t->ySort = true;
    int before = RosterCount();
    heroes.push_back(t);
    towerGrid[row][col] = t->id;
    if (before < (int)roster.size() && RosterComplete()) {
        fusionCooldown = std::max(fusionCooldown, kFusionCharge);
        fusionSpan = std::max(fusionCooldown, 1.0f);
        Banner("光之巨人全员集结！", "究极合体开始蓄能，完成后按 E 发动", Palette::gold);
    }
    Burst(Add(at, {0, -10}), def.beam, 26, 150, 5, 0.6f);
    AddRing(at, 6, 46, 0.45f, 4, def.beam);
    AddBeam({at.x, kMapY - 30}, Add(at, {0, -6}), 14, 0.4f, def.beam, WHITE);
    Sfx::Play("build", 0.6f);
    return true;
}

void BattleScene::UpgradeTower(HeroActor& t) {
    int cost = UpgradeCost(t.hero, t.level);
    if (cost <= 0 || gold < cost) {
        Sfx::Play("error", 0.5f);
        if (cost > 0) Text(Add(t.Pos(), {0, -40}), "金币不足", Palette::red, 15);
        return;
    }
    gold -= cost;
    t.invested += cost;
    t.level++;
    t.maxHp = HeroMaxHp(t.hero, t.level);
    if (t.disabled != HeroActor::Disable::Exhausted) t.hp = t.maxHp; // an upgrade refills the colour timer
    t.recoil = 1;
    Burst(Add(t.Pos(), {0, -14}), Palette::gold, 30, 170, 5, 0.7f);
    AddRing(t.Pos(), 8, 52, 0.5f, 4, Palette::gold);
    Text(Add(t.Pos(), {0, -46}), t.level == 2 ? "满级！" : "升级！", Palette::gold, 17);
    Sfx::Play("upgrade", 0.6f, 0);
}

int BattleScene::SellValue(const HeroActor& t) const { return (int)(t.invested * 0.7f); }

void BattleScene::SellTower(int id) {
    HeroActor* t = FindTower(id);
    if (!t) return;
    int refund = SellValue(*t);
    gold += refund;
    Text(Add(t->Pos(), {0, -30}), TextFormat("+%d", refund), Palette::gold, 16);
    Smoke(t->Pos(), 6, 12);
    Burst(t->Pos(), HeroInfo(t->hero).beam, 14, 120, 4, 0.5f);
    towerGrid[t->row][t->col] = -1;
    Destroy(t->GetGameObject());
    heroes.erase(std::remove_if(heroes.begin(), heroes.end(), [id](const Ref<HeroActor>& h) { return h->id == id; }), heroes.end());
    selected = -1;
    Sfx::Play("sell", 0.6f);
}

HeroActor* BattleScene::FindTower(int id) {
    for (auto& t : heroes)
        if (t->id == id) return t.get();
    return nullptr;
}

HeroActor* BattleScene::NearestHero(Vector2 at, float radius, bool skipDisabled) {
    HeroActor* best = nullptr;
    float bestDist = radius;
    for (auto& t : heroes) {
        if (skipDisabled && t->disabledTime > 0) continue;
        float d = Len(Sub(t->Pos(), at));
        if (d <= bestDist) {
            bestDist = d;
            best = t.get();
        }
    }
    return best;
}

void BattleScene::DisableHero(HeroActor& hero, HeroActor::Disable kind, float seconds, Vector2 from) {
    hero.disabledTime = std::max(hero.disabledTime, seconds);
    hero.disabled = kind;
    bool stone = kind == HeroActor::Disable::Petrified;
    Color tone = stone ? Color{200, 196, 180, 255} : Color{255, 70, 60, 255};
    AddBeam(from, Add(hero.Pos(), {0, -20}), stone ? 6.0f : 3.0f, 0.4f, tone, WHITE);
    Burst(Add(hero.Pos(), {0, -20}), tone, 14, 120, 5, 0.4f);
    Sfx::Play(stone ? "shield" : "teleport", 0.45f);
}

void BattleScene::KaijuAttack(MonsterActor& e, HeroActor& target) {
    const EnemyDef& def = e.Def();
    float damage = def.strikeDamage * Assault() * (e.buffTime > 0 || e.enraged ? 1.3f : 1.0f);
    Vector2 from = e.Center(), to = Add(target.Pos(), {0, -22});
    int heroId = target.id;
    if (def.boss || def.strike == KaijuStrike::Rock)
        Text(Add(from, {0, -e.Height() * 0.6f}), def.strikeName, {255, 160, 120, 255}, def.boss ? 15.0f : 12.0f);
    switch (def.strike) {
    case KaijuStrike::None:
        return;
    case KaijuStrike::Shock: {
        Color tone{255, 170, 90, 255};
        AddRing(e.Feet(), 8, def.strikeRange * kTile, 0.35f, 4, tone);
        Sparks(to, tone, 6, 140);
        DamageHero(target, damage);
        Sfx::Play("pulse", 0.3f);
        break;
    }
    case KaijuStrike::Beam: {
        Color glow = e.type == EnemyId::Baltan    ? Color{120, 200, 255, 255}
                     : e.type == EnemyId::Belial  ? Color{220, 30, 60, 255}
                     : e.type == EnemyId::Tregear ? Color{140, 100, 255, 255}
                                                  : Color{255, 96, 70, 255};
        AddBeam(from, to, def.boss ? 7.0f : 4.0f, 0.3f, glow, WHITE);
        Sparks(to, glow, 6, 150);
        DamageHero(target, damage);
        Sfx::Play("beam", 0.25f);
        break;
    }
    case KaijuStrike::Bolt:
        Bolt(from, to, {255, 236, 90, 255});
        Sparks(to, {255, 236, 90, 255}, 5, 160);
        DamageHero(target, damage);
        Sfx::Play("hit", 0.25f);
        break;
    case KaijuStrike::Rock:
    case KaijuStrike::Fireball: {
        bool fire = def.strike == KaijuStrike::Fireball;
        auto p = Launch(ProjectileMotion::Lob, from);
        p->layerMask = 0; // lands on a hero, never on kaiju
        p->target = to;
        p->flightTime = fire ? 0.8f : 0.7f;
        p->onArrive = [this, heroId, damage, fire](Projectile&, Vector2 at) {
            if (!Running()) return;
            if (HeroActor* h = FindTower(heroId)) DamageHero(*h, damage);
            if (fire) {
                for (auto& h : heroes)
                    if (h->id != heroId && Len(Sub(h->Pos(), at)) < kTile * 1.2f) DamageHero(*h, damage * 0.5f);
                AddRing(at, 6, kTile * 1.2f, 0.35f, 4, {255, 150, 50, 255});
            }
            Burst(at, fire ? Color{255, 140, 40, 255} : Color{170, 140, 100, 255}, 14, 160, 6, 0.4f);
            Smoke(at, fire ? 4 : 6, 12);
            Sfx::Play("explode", 0.35f);
        };
        p->onDraw = [fire](const Projectile& self) {
            Vector2 at = self.RenderPosition();
            Draw::Shadow(self.target, 8, 4, 0.35f);
            if (fire) {
                Draw::BlendAdd();
                Draw::Glow(at, 24, WithAlpha({255, 110, 30, 255}, 0.8f));
                Draw::Glow(at, 9, WithAlpha({255, 240, 180, 255}, 0.95f));
                Draw::BlendNormal();
            } else {
                Draw::Shaded(at, 8, 7, {132, 112, 88, 255}, self.Age() * 6, 1.5f);
            }
        };
        Sfx::Play(fire ? "fire" : "slugger", 0.35f);
        break;
    }
    }
}

void BattleScene::DamageHero(HeroActor& hero, float amount) {
    if (!Running() || amount <= 0 || hero.disabled == HeroActor::Disable::Exhausted) return;
    hero.hp -= amount;
    hero.hurt = 1;
    hero.calm = 0;
    if (hero.hp > 0) return;
    hero.hp = 0;
    hero.disabled = HeroActor::Disable::Exhausted;
    hero.disabledTime = kExhaustTime;
    ++exhaustions;
    Text(Add(hero.Pos(), {0, -52}), "能量耗尽！", {255, 90, 90, 255}, 15);
    Burst(Add(hero.Pos(), {0, -20}), {255, 70, 70, 255}, 16, 140, 5, 0.5f);
    Sfx::Play("leak", 0.4f);
}

// ================================================================ helpers

void BattleScene::Banner(const std::string& title, const std::string& sub, Color color, bool warning) {
    bannerTitle = title;
    bannerSub = sub;
    bannerColor = color;
    bannerWarning = warning;
    bannerTime = 0;
}

void BattleScene::SetOutcome(Outcome value) {
    if (outcome != Outcome::None) return;
    outcome = value;
    outcomeTime = 0;
    placing = selected = -1;
    targetingStrike = false;
    earnedStars = value == Outcome::Victory ? (lives >= level->lives ? 3 : lives >= level->lives * 0.6f ? 2 : 1) : 0;
    if (headless) return;
    if (Endless()) {
        newRecord = Progress::RecordSurvival(surviveTime, waveIndex + 1);
        Sfx::Play(newRecord ? "victory" : "defeat", 0.9f, 0);
    } else if (value == Outcome::Victory) {
        Progress::RecordStars(levelIndex, earnedStars);
        Sfx::Play("victory", 0.9f, 0);
    } else {
        Sfx::Play("defeat", 0.9f, 0);
    }
    Progress::AddKills(kills);
    Progress::Save();
}

void BattleScene::Emit(Fx kind, Vector2 at, Color color, int count, float speed, float size, float life) {
    auto& emitter = fx[(int)kind];
    if (!emitter) return;
    emitter->GetGameObject()->transform->position = at;
    emitter->GetGameObject()->transform->rotation = kind == Fx::Ember ? -90.0f : 0.0f;
    emitter->spreadAngle = kind == Fx::Ember ? 70.0f : 360.0f;
    emitter->startColor = color;
    emitter->endColor = WithAlpha(color, 0);
    emitter->speedMin = speed * (kind == Fx::Spark ? 0.4f : 0.25f);
    emitter->speedMax = speed;
    emitter->startSizeMin = size * 0.6f;
    emitter->startSizeMax = size * 1.3f;
    emitter->lifetimeMin = life * 0.6f;
    emitter->lifetimeMax = life * 1.2f;
    emitter->Burst(count);
}

void BattleScene::Burst(Vector2 at, Color color, int count, float speed, float size, float life) {
    Emit(Fx::Glow, at, color, count, speed, size * 2, life);
}

void BattleScene::Smoke(Vector2 at, int count, float size) {
    for (int i = 0; i < count; ++i) Emit(Fx::Smoke, Add(at, {Rand(-8, 8), Rand(-8, 4)}), {70, 64, 70, 150}, 1, 30, size * 0.6f, 1.0f);
}

void BattleScene::Sparks(Vector2 at, Color color, int count, float speed) { Emit(Fx::Spark, at, color, count, speed, 3, 0.3f); }

void BattleScene::Text(Vector2 at, const std::string& text, Color color, float size) {
    EffectLayer::TextOptions options;
    options.size = size;
    options.color = color;
    effects->Text(at, text, options);
}

void BattleScene::AddBeam(Vector2 a, Vector2 b, float width, float life, Color glow, Color core) {
    effects->Beam(a, b, {width, life, glow, core});
}

void BattleScene::AddRing(Vector2 c, float r0, float r1, float life, float width, Color color) {
    effects->Ring(c, {r0, r1, width, life, color});
}

void BattleScene::Bolt(Vector2 a, Vector2 b, Color color) {
    Vector2 d = Sub(b, a);
    Vector2 n = Normalize({-d.y, d.x});
    float jitter = std::min(14.0f, Len(d) * 0.12f);
    Vector2 prev = a;
    for (int i = 1; i <= 4; ++i) {
        Vector2 p = Add(a, Mul(d, i / 4.0f));
        if (i < 4) p = Add(p, Mul(n, Rand(-jitter, jitter)));
        AddBeam(prev, p, 2.6f, 0.2f, color, WithAlpha(WHITE, 0.8f));
        prev = p;
    }
}

std::weak_ptr<GameObject> BattleScene::Follow(MonsterActor& target) const { return target.object; }

Ref<Projectile> BattleScene::Launch(ProjectileMotion motion, Vector2 from) {
    auto object = CreateGameObject("projectile");
    object->transform->position = from;
    auto p = object->AddComponent<Projectile>();
    p->motion = motion;
    p->layerMask = kKaijuMask;
    p->renderOrder = 12;
    return p;
}

// ================================================================ bot

std::vector<GridPoint> BattleScene::RankedTiles() const {
    std::vector<std::pair<int, GridPoint>> scored;
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c) {
            if (!map.Buildable(c, r)) continue;
            Vector2 p = TileCenter(c, r);
            int score = 0;
            for (const auto& path : map.paths)
                for (float d = 0; d < path.length; d += 6)
                    if (Len(Sub(path.Sample(d), p)) < kTile * 2.6f) ++score;
            scored.push_back({score, {c, r}});
        }
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<GridPoint> out;
    for (auto& s : scored) out.push_back(s.second);
    return out;
}

void BattleScene::BotAct() {
    // Card slots, not heroes: each level's roster keeps the cheap opener first and the premium pick last.
    static const int order[] = {0, 4, 3, 1, 2, 5, 0, 4, 2, 1, 5, 3};
    auto alive = Alive();
    bool boss = std::any_of(alive.begin(), alive.end(), [](const auto& e) { return e->Def().boss; });
    const MonsterActor* front = nullptr;
    for (const auto& e : alive)
        if (!front || e->dist > front->dist) front = e.get();
    if (!botCasual) {
        if (RosterComplete() && (boss || alive.size() >= 8)) CastFusion();
        if (alive.size() >= 12 || (boss && alive.size() >= 4)) CastBrothers();
        if (front && alive.size() >= 5) CastStrike(front->Center());
    }

    if (botTiles.empty()) botTiles = RankedTiles();
    for (int guard = 0; guard < (botCasual ? 1 : 4); ++guard) {
        HeroId next = roster[order[botPurchases % 12] % roster.size()];
        HeroActor* cheapest = nullptr;
        for (auto& t : heroes) {
            int cost = UpgradeCost(t->hero, t->level);
            if (cost > 0 && (!cheapest || cost < UpgradeCost(cheapest->hero, cheapest->level))) cheapest = t.get();
        }
        bool wantBuild = heroes.size() < 7 || !cheapest || botPurchases % 3 != 2;
        if (wantBuild && gold >= HeroInfo(next).cost) {
            std::vector<GridPoint> free;
            for (const auto& tile : botTiles)
                if (towerGrid[tile.row][tile.col] < 0) free.push_back(tile);
            if (!free.empty()) {
                // A casual player knows roughly where towers go but rarely picks the best tile.
                int pick = botCasual ? GetRandomValue(0, std::max(0, (int)(free.size() * 0.4f) - 1)) : 0;
                BuyTower(next, free[pick].col, free[pick].row);
            }
            ++botPurchases;
        } else if (cheapest && gold >= UpgradeCost(cheapest->hero, cheapest->level)) {
            UpgradeTower(*cheapest);
            if (!wantBuild) ++botPurchases;
        } else {
            break;
        }
    }
}

BattleScene::BotResult BattleScene::RunBot(bool casual) {
    botCasual = casual;
    float decide = 0;
    SceneClock clock(kStep);
    while (Running() && simTime < 3600) {
        if (waveIndex < 0) {
            if (casual)
                for (int i = 0; i < 8; ++i) BotAct(); // players spend the opening gold before calling the first wave
            StartNextWave(false);
        }
        decide -= kStep;
        if (decide <= 0) {
            decide = casual ? 2.5f : 0.5f;
            BotAct();
        }
        clock.Step(*this);
    }
    return {outcome == Outcome::Victory, lives, waveIndex + 1, (int)heroes.size(), gold, kills, leakWave, leakCount, simTime / 60, exhaustions};
}

float BattleScene::BenchmarkDemo(int star, float seconds) {
    if (!demo || heroes.empty()) return 0;
    SetDemoStar(star);
    float start = simTime;
    SceneClock clock(kStep);
    while (simTime - start < seconds) clock.Step(*this);
    return heroes.front()->dealt / seconds;
}

// Only the skill itself: beams, projectiles, fields and particles, shifted so the hero sits at heroAt.
void BattleScene::DrawEmbedded(Rectangle clip, Vector2 heroAt) {
    if (heroes.empty()) return;
    Vector2 from = heroes.front()->Pos();
    Display::ClipScope clipScope(clip);
    rlPushMatrix();
    rlTranslatef(heroAt.x - from.x, heroAt.y - from.y, 0);
    ScopeActivation scope(GetCallbackScope());
    DrawZones();
    Scene::Render();
    rlPopMatrix();
}

void BattleScene::ReleaseEmbedded() {
    if (mapTexture.id) UnloadRenderTexture(mapTexture);
    mapTexture = {};
    Clear();
}


void BattleScene::DebugAutoplay(int startWave) {
    gold += 2500 + startWave * 400;
    for (int i = 0; i < 6; ++i) BotAct();
    waveIndex = startWave - 1;
    EnsureWaves(startWave + 1);
    StartNextWave(false);
    speedIndex = 2;
    ApplySpeed();
}

void BattleScene::DebugCast() {
    brothersCooldown = strikeCooldown = fusionCooldown = 0;
    CastBrothers();
    CastFusion(true);
    Vector2 target = map.basePos;
    for (const auto& e : Alive()) target = e->Center();
    CastStrike(target);
}

void BattleScene::DebugInspect(bool pause) {
    for (int i = 0; i < 2; ++i)
        if (!heroes.empty()) UpgradeTower(*heroes.front());
    selected = heroes.empty() ? -1 : heroes.front()->id;
    SetPaused(pause);
}

void BattleScene::DebugFinish() {
    headless = true; // keeps the player's save untouched
    kills = 286;
    if (Endless()) {
        lives = 0;
        SetOutcome(Outcome::Defeat);
        return;
    }
    lives = level->lives - 3;
    waveIndex = (int)level->waves.size() - 1;
    SetOutcome(Outcome::Victory);
}

int RunBalanceReport(const std::string& path) {
    std::string report;
    SetTraceLogLevel(LOG_NONE);
    int count = (int)Levels().size();
    for (int i = SurvivalLevel() ? kSurvivalIndex : 0; i < count; ++i) {
        for (int run = 0; run < 15; ++run) {
            bool casual = run >= 3;
            SetRandomSeed(1000 + run * 77 + i);
            auto scene = CreateRef<BattleScene>(i);
            scene->headless = true;
            scene->Load();
            auto r = scene->RunBot(casual);
            report += TextFormat("level %d %s %d: %s lives=%d wave=%d towers=%d gold=%d kills=%d firstLeak=%d "
                                 "minutes=%.1f exhausted=%d leaks:",
                                 i, casual ? "casual" : "expert", casual ? run - 3 : run, r.won ? "WIN " : "LOSE", r.lives, r.wave, r.towers, r.gold,
                                 r.kills, r.firstLeakWave, r.minutes, r.exhausted);
            for (int k = 0; k < kEnemyCount; ++k) report += TextFormat(" %d", r.leaks[k]);
            report += "\n";
            scene->Clear();
        }
    }
    for (int h = 0; h < kHeroCount; ++h) {
        report += TextFormat("hero %s cost=%d dps:", HeroInfo((HeroId)h).key, HeroInfo((HeroId)h).cost);
        for (int star : {0, 2}) {
            SetRandomSeed(500 + h);
            auto scene = CreateRef<BattleScene>((HeroId)h);
            scene->headless = true;
            scene->Load();
            report += TextFormat(" %.1f", scene->BenchmarkDemo(star, 90));
            scene->Clear();
        }
        report += "\n";
    }
    SaveFileText(path.c_str(), report.data());
    return 0;
}

// ================================================================ rendering

void BattleScene::Render() {
    Gui::BeginFrame();
    DrawWorld();
    DrawBossBar();
    DrawBanner();
    DrawIntroHint();
    DrawEnemyTooltip();
    // Touch screens have no right click: offer an explicit way out of placing/targeting.
    if ((placing >= 0 || targetingStrike) && Running() && !paused &&
        Gui::Button({kMapX + kMapW - 128, kMapY + kMapH - 58, 116, 46}, targetingStrike ? "取消轰炸" : "取消放置", Gui::Style::Red, true,
                    17)) {
        placing = -1;
        targetingStrike = false;
    }
    DrawHud();
    DrawPanel();
    DrawFusionCinematic();
    DrawPauseMenu();
    DrawResult();
}

bool BattleScene::Cinematic() const { return fusionAnim >= 0 && fusionAnim < kFusionImpactAt; }

void BattleScene::DrawFusionCinematic() {
    if (fusionAnim < 0) return;
    float t = fusionAnim;
    auto ramp = [](float x, float a, float b) { return Clamp((x - a) / (b - a), 0.0f, 1.0f); };
    float outro = 1 - ramp(t, kFusionEnd - 0.6f, kFusionEnd);
    Rectangle canvas{0, 0, (float)kCanvasW, (float)kCanvasH};
    Vector2 sky{kMapX + kMapW * 0.5f, kMapY + kMapH * 0.2f};
    Vector2 ground{kMapX + kMapW * 0.5f, kMapY + kMapH * 0.45f};
    static const Color hues[] = {{255, 70, 70, 255},  {255, 196, 60, 255},  {80, 255, 170, 255},
                                 {70, 170, 255, 255}, {190, 110, 255, 255}, {255, 120, 210, 255}};
    int n = (int)roster.size();

    // Time stop: dim the field and slide in letterbox bars.
    float dim = ramp(t, 0, 0.4f) * (t < kFusionImpactAt ? 0.6f : 0.35f) * outro;
    DrawRectangleRec(canvas, WithAlpha({4, 4, 16, 255}, dim));
    float bar = 70 * ramp(t, 0, 0.35f) * outro;
    DrawRectangleRec({0, 0, (float)kCanvasW, bar}, BLACK);
    DrawRectangleRec({0, kCanvasH - bar, (float)kCanvasW, bar}, BLACK);

    // Light pillars rise from every hero on the field, then bend into the sky orb.
    float rise = ramp(t, 0.35f, 1.4f), gather = ramp(t, 1.6f, 2.7f);
    Draw::BlendAdd();
    int k = 0;
    for (const auto& h : heroes) {
        Color c = HeroInfo(h->hero).beam;
        Vector2 foot = Add(h->Pos(), {0, -24});
        if (t < kFusionImpactAt) {
            Vector2 top = Lerp({foot.x, foot.y - 260 * rise}, sky, gather);
            Draw::Line(foot, top, 5 + 3 * gather, WithAlpha(c, 0.75f * rise));
            Draw::Line(foot, top, 2, WithAlpha(WHITE, 0.8f * rise));
            Draw::Glow(foot, 26, WithAlpha(c, 0.6f * rise));
        }
        if (++k >= 16) break;
    }
    // The orb swells and spins rainbow rings until it drops.
    if (t > 1.2f && t < kFusionImpactAt + 0.1f) {
        float grow = ramp(t, 1.2f, kFusionImpactAt);
        float r = 14 + 60 * grow;
        Draw::Glow(sky, r * 3.2f, WithAlpha({255, 230, 160, 255}, 0.45f * grow));
        for (int i = 0; i < 6; ++i) {
            float a = RealTime() * (2 + i * 0.35f) + i;
            Draw::Arc(sky, r * (0.8f + i * 0.16f), 4, a, a + PI * 1.2f, WithAlpha(hues[i], 0.85f), 24);
        }
        Draw::Glow(sky, r, WithAlpha(WHITE, 0.95f));
    }
    // Impact: a rainbow pillar slams down and lingers.
    if (t >= kFusionImpactAt) {
        float life = 1 - ramp(t, kFusionImpactAt, kFusionEnd);
        float w = 90 * (0.4f + 0.6f * life);
        for (int i = 0; i < 6; ++i) {
            float off = (i - 2.5f) * w * 0.18f;
            Draw::Line({ground.x + off, kMapY - 40}, {ground.x + off * 0.4f, ground.y}, w * 0.22f, WithAlpha(hues[i], 0.55f * life));
        }
        Draw::Line({ground.x, kMapY - 40}, ground, w * 0.45f, WithAlpha(WHITE, 0.85f * life));
        Draw::Glow(ground, 220 * (1.4f - life), WithAlpha({255, 240, 200, 255}, 0.6f * life));
        float flash = 1 - ramp(t, kFusionImpactAt, kFusionImpactAt + 0.35f);
        DrawRectangleRec(canvas, WithAlpha(WHITE, 0.85f * flash));
    }
    Draw::BlendNormal();

    // Cut-ins: each roster hero slides across a slanted band with their name.
    if (t < kFusionImpactAt) {
        float bandH = 92, gapY = 8;
        int rows = std::min(n, 6);
        float total = rows * bandH + (rows - 1) * gapY;
        float y0 = kCanvasH / 2.0f - total / 2;
        for (int i = 0; i < rows; ++i) {
            float start = 0.3f + i * 0.2f;
            float in = ramp(t, start, start + 0.35f), out = ramp(t, 2.2f, 2.6f);
            if (in <= 0) continue;
            const HeroDef& def = HeroInfo(roster[i]);
            bool fromLeft = i % 2 == 0;
            float slide = (1 - EaseOut(in)) * 700 + out * 900;
            float x = fromLeft ? 140 - slide : kCanvasW - 140 - 420 + slide;
            float y = y0 + i * (bandH + gapY);
            float skew = 26;
            Color band = WithAlpha(Mix(def.primary, {8, 10, 24, 255}, 0.35f), 0.92f * (1 - out));
            Draw::Quad({x + skew, y}, {x + 420 + skew, y}, {x + 420 - skew, y + bandH}, {x - skew, y + bandH}, band);
            Draw::BlendAdd();
            Draw::Line({x + skew, y}, {x + 420 + skew, y}, 3, WithAlpha(def.beam, 0.9f * (1 - out)));
            Draw::Line({x - skew, y + bandH}, {x + 420 - skew, y + bandH}, 3, WithAlpha(def.beam, 0.9f * (1 - out)));
            Draw::BlendNormal();
            float face = fromLeft ? x + 20 : x + 420 - 20 - 112;
            Art::DrawPortrait(roster[i], {face, y - 12, 112, 112}, WithAlpha(WHITE, 1 - out));
            float tx = fromLeft ? x + 150 : x + 40;
            Gui::TextOutline(def.name, tx, y + 16, 26, WithAlpha(WHITE, 1 - out), WithAlpha(BLACK, 1 - out), 1.6f, Gui::Align::Left);
            Gui::Text(def.move, tx + 2, y + 54, 17, WithAlpha(def.beam, 1 - out), Gui::Align::Left, true);
        }
    }

    // Title: "究极合体" during the build-up, "新世代闪光" on impact.
    float titleIn = ramp(t, 0.1f, 0.5f);
    if (t < kFusionImpactAt) {
        Gui::TextOutline("究极合体", kCanvasW / 2.0f, 16 + (1 - titleIn) * -30, 40, WithAlpha(Palette::gold, titleIn),
                         WithAlpha({60, 20, 0, 255}, titleIn), 2.4f);
    } else {
        float pop = ramp(t, kFusionImpactAt, kFusionImpactAt + 0.25f);
        float size = 64 + (1 - pop) * 40;
        Gui::TextOutline("新世代闪光", kCanvasW / 2.0f, kCanvasH / 2.0f - size / 2, size, WithAlpha({255, 244, 210, 255}, outro),
                         WithAlpha({120, 40, 0, 255}, outro), 3);
    }
}

void BattleScene::DrawWorld() {
    float s = shake * shake * 9;
    rlPushMatrix();
    if (s > 0.01f) rlTranslatef(Rand(-1, 1) * s, Rand(-1, 1) * s, 0);

    DrawMapTexture(mapTexture, MapRect());
    if (Siege()) DrawSiege();
    DrawMapLive(map, simTime, baseFlash);
    DrawZones();
    DrawPlacement();
    for (const auto& t : heroes) Draw::Shadow(Add(t->Pos(), {2, 12}), 22, 8, 0.45f);
    for (const auto& e : monsters)
        if (!e->dead) Draw::Shadow(Add(e->Feet(), {2, 1}), 15 * e->Scale(), 5 * e->Scale(), 0.45f * e->appear);

    Scene::Render(); // corpses, heroes and kaiju in depth order, then particle emitters

    if (fogTime > 0) {
        float a = Clamp(fogTime, 0.0f, 1.0f) * Clamp((4.0f - fogTime) * 3, 0.0f, 1.0f);
        DrawRectangleRec(MapRect(), WithAlpha({20, 10, 34, 255}, 0.42f * a));
        Rng mist(7);
        for (int i = 0; i < 14; ++i) {
            Vector2 c{kMapX + mist.Float() * kMapW + std::sin(simTime * 0.6f + i) * 30, kMapY + mist.Float() * kMapH};
            Draw::GlowStretched(c, mist.Range(90, 170), mist.Range(30, 60), 0, WithAlpha({70, 40, 90, 255}, 0.35f * a));
        }
        Gui::TextOutline("黑雾笼罩：英雄攻速减半", kMapX + kMapW / 2, kMapY + kMapH - 40, 18, WithAlpha({220, 200, 255, 255}, a),
                         WithAlpha(BLACK, a), 1.4f);
    }

    for (const auto& e : monsters) DrawHealthBar(*e);
    if (Siege()) {
        // Tower durability under the spire.
        float ratio = citadelMax > 0 ? Clamp(citadel / citadelMax, 0.0f, 1.0f) : 0;
        Rectangle bar{map.basePos.x - 52, map.basePos.y + 66, 104, 9};
        Color fill = ratio > 0.5f ? Color{110, 200, 255, 255} : ratio > 0.25f ? Color{250, 200, 70, 255} : Color{240, 70, 70, 255};
        Draw::RoundedPanel({bar.x - 3, bar.y - 3, bar.width + 6, bar.height + 6}, 5, {8, 10, 24, 220}, WithAlpha(fill, 0.5f + 0.5f * citadelPulse), 1.2f);
        DrawRectangleRec({bar.x, bar.y, bar.width * ratio, bar.height}, fill);
        DrawRectangleRec({bar.x, bar.y, bar.width * ratio, 3}, Lighten(fill, 0.4f));
    }

    if (placing >= 0) {
        Vector2 m = Input::GetMousePosition();
        int col = (int)std::floor((m.x - kMapX) / kTile), row = (int)std::floor((m.y - kMapY) / kTile);
        if (map.InGrid(col, row) && CheckCollisionPointRec(m, MapRect())) {
            Vector2 c = TileCenter(col, row);
            float size = 60;
            Art::DrawPortrait((HeroId)placing, {c.x - size / 2, c.y + 8 - size * 0.92f, size, size}, WithAlpha(WHITE, 0.75f));
        }
    }

    for (const auto& st : strikes) {
        float x = Lerp(kMapX - 140, kMapX + kMapW + 140, st.time / 1.8f);
        float y = st.target.y - 80;
        Draw::Shadow({x + 20, st.target.y + 20}, 40, 10, 0.3f);
        Color body{210, 214, 224, 255}, stripe{236, 120, 40, 255};
        Draw::Quad({x + 40, y}, {x - 30, y - 7}, {x - 34, y}, {x - 30, y + 7}, body);
        Draw::Tri({x + 4, y - 4}, {x - 18, y - 34}, {x - 26, y - 30}, Darken(body, 0.15f));
        Draw::Tri({x + 4, y + 4}, {x - 18, y + 34}, {x - 26, y + 30}, Darken(body, 0.2f));
        Draw::Tri({x - 24, y}, {x - 38, y - 16}, {x - 40, y}, stripe);
        Draw::Line({x - 30, y}, {x + 34, y}, 2, stripe);
        DrawCircleV({x + 26, y - 2}, 3.5f, {80, 160, 230, 255});
        Draw::BlendAdd();
        Draw::GlowStretched({x - 48, y}, 22, 6, 0, WithAlpha({255, 170, 80, 255}, 0.8f));
        Draw::BlendNormal();
    }

    rlPopMatrix();

    if (flashScreen > 0) {
        Draw::BlendAdd();
        DrawRectangleRec(MapRect(), WithAlpha({220, 240, 255, 255}, flashScreen * 0.5f));
        Draw::BlendNormal();
    }
}

void BattleScene::DrawZones() {
    for (const auto& z : zones) {
        float fade = Clamp(z.time / 0.25f, 0.0f, 1.0f) * Clamp((z.life - z.time) / 0.4f, 0.0f, 1.0f);
        Color gold{255, 200, 90, 255}, amber{255, 140, 50, 255};
        DrawCircleV(z.at, z.radius, WithAlpha(amber, 0.16f * fade));
        Draw::BlendAdd();
        Draw::Glow(z.at, z.radius * 1.3f, WithAlpha(gold, 0.22f * fade));
        DrawRing(z.at, z.radius - 2, z.radius + 1.5f, 0, 360, 64, WithAlpha(gold, 0.85f * fade));
        for (int i = 0; i < 3; ++i) {
            float a = simTime * (1.2f + i * 0.5f) + i * 2.1f;
            Draw::Arc(z.at, z.radius * (0.45f + i * 0.18f), 2.5f, a, a + PI * 0.7f, WithAlpha(gold, 0.6f * fade), 16);
        }
        // Rising curtain lines, the field's "dimension wall".
        for (int i = 0; i < 10; ++i) {
            float a = i * PI / 5 + simTime * 0.4f;
            Vector2 foot = Add(z.at, {std::cos(a) * z.radius, std::sin(a) * z.radius});
            float h = 18 + 10 * std::sin(simTime * 4 + i);
            Draw::Line(foot, Add(foot, {0, -h}), 2, WithAlpha(gold, 0.45f * fade));
        }
        Draw::BlendNormal();
    }
}

void BattleScene::DrawSiege() {
    float ratio = citadelMax > 0 ? Clamp(citadel / citadelMax, 0.0f, 1.0f) : 0;
    Vector2 c = map.basePos;
    float r = kTile * 1.75f;
    Color tone = Mix({255, 90, 80, 255}, {130, 210, 255, 255}, ratio);
    DrawCircleV(c, r, WithAlpha(tone, 0.07f));
    Draw::BlendAdd();
    DrawRing(c, r - 2, r + 2, 0, 360, 96, WithAlpha(tone, 0.35f + 0.25f * citadelPulse));
    for (int i = 0; i < 8; ++i) {
        float a = i * PI / 4 + simTime * 0.15f;
        Vector2 post = Add(c, Mul(Dir(a), r));
        Draw::Glow(post, 10, WithAlpha(tone, 0.6f));
        Draw::Arc(c, r, 3, a + 0.12f, a + PI / 4 - 0.12f, WithAlpha(tone, 0.5f * ratio), 8);
    }
    Draw::BlendNormal();
}

void BattleScene::DrawPlacement() {
    if (HeroActor* t = FindTower(selected)) {
        float r = HeroRange(t->hero, t->level);
        Color c = HeroInfo(t->hero).beam;
        DrawCircleV(t->Pos(), r, WithAlpha(c, 0.08f));
        DrawRing(t->Pos(), r - 1.5f, r + 1.0f, 0, 360, 72, WithAlpha(c, 0.7f));
        float pulse = 0.5f + 0.5f * std::sin(RealTime() * 6);
        DrawRing(Add(t->Pos(), {0, 10}), 24, 27 + pulse * 2, 0, 360, 36, WithAlpha(Palette::gold, 0.8f));
    }
    if (placing >= 0) {
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c)
                if (map.Buildable(c, r) && towerGrid[r][c] < 0)
                    DrawRectangleRounded({kMapX + c * kTile + 3, kMapY + r * kTile + 3, kTile - 6, kTile - 6}, 0.2f, 4,
                                         WithAlpha({140, 255, 170, 255}, 0.09f));
        Vector2 m = Input::GetMousePosition();
        int col = (int)std::floor((m.x - kMapX) / kTile), row = (int)std::floor((m.y - kMapY) / kTile);
        if (!map.InGrid(col, row) || !CheckCollisionPointRec(m, MapRect())) return;
        const HeroDef& def = HeroInfo((HeroId)placing);
        bool valid = map.Buildable(col, row) && towerGrid[row][col] < 0 && gold >= def.cost;
        Vector2 c = TileCenter(col, row);
        Color tone = valid ? Color{120, 255, 160, 255} : Color{255, 90, 90, 255};
        float r = HeroRange((HeroId)placing, 0);
        DrawCircleV(c, r, WithAlpha(tone, 0.08f));
        DrawRing(c, r - 1.5f, r + 1.0f, 0, 360, 72, WithAlpha(tone, 0.7f));
        Draw::RoundedPanel({kMapX + col * kTile + 2, kMapY + row * kTile + 2, kTile - 4, kTile - 4}, 6, WithAlpha(tone, 0.22f),
                           WithAlpha(tone, 0.9f), 2);
    }
    if (targetingStrike) {
        Vector2 m = Input::GetMousePosition();
        float t = RealTime();
        DrawCircleV(m, 100, WithAlpha({255, 120, 60, 255}, 0.08f));
        for (int i = 0; i < 12; ++i) {
            float a = t * 0.8f + i * PI / 6;
            Draw::Arc(m, 100, 2.5f, a, a + PI / 12, WithAlpha({255, 140, 70, 255}, 0.9f), 6);
        }
        DrawRing(m, 14, 17, 0, 360, 24, {255, 120, 60, 255});
        Draw::Line({m.x - 26, m.y}, {m.x - 8, m.y}, 2, {255, 120, 60, 255});
        Draw::Line({m.x + 8, m.y}, {m.x + 26, m.y}, 2, {255, 120, 60, 255});
        Draw::Line({m.x, m.y - 26}, {m.x, m.y - 8}, 2, {255, 120, 60, 255});
        Draw::Line({m.x, m.y + 8}, {m.x, m.y + 26}, 2, {255, 120, 60, 255});
        Gui::TextOutline("点击选择轰炸区域", m.x, m.y + 108, 15, WHITE, {40, 10, 0, 255}, 1.5f);
    }
}

void BattleScene::DrawHealthBar(const MonsterActor& e) {
    if (e.Def().boss || e.hp >= e.maxHp || e.dead) return;
    float w = 28 * std::max(1.0f, e.Scale());
    Vector2 top = Add(e.Feet(), {-w / 2, -e.Height() - 10});
    float ratio = Clamp(e.hp / e.maxHp, 0.0f, 1.0f);
    Color fill = ratio > 0.6f ? Color{110, 230, 120, 255} : ratio > 0.3f ? Color{250, 200, 70, 255} : Color{240, 70, 70, 255};
    DrawRectangleRec({top.x - 1, top.y - 1, w + 2, 6}, {10, 10, 18, 220});
    DrawRectangleRec({top.x, top.y, w * ratio, 4}, fill);
    DrawRectangleRec({top.x, top.y, w * ratio, 1.5f}, Lighten(fill, 0.4f));
    if (e.breakTime > 0) DrawRectangleRec({top.x + w + 3, top.y - 1, 4, 6}, {255, 120, 60, 255});
}

// ================================================================ HUD

void BattleScene::DrawHud() {
    Rectangle bar{0, 0, (float)kCanvasW, kMapY};
    UI::Capture(bar);
    Draw::VerticalGradientRect(bar, {26, 36, 70, 255}, {12, 18, 40, 255});
    DrawRectangleRec({0, kMapY - 2, (float)kCanvasW, 2}, WithAlpha(Palette::gold, 0.55f));

    // Emblem + level name
    Vector2 em{30, 30};
    DrawCircleV(em, 19, {10, 14, 30, 255});
    DrawRing(em, 15, 18, 0, 360, 32, Palette::silver);
    Draw::GradientEllipse(em, 13, 13, {20, 100, 210, 255}, {200, 245, 255, 255}, {em.x - 4, em.y - 4}, 8);
    Draw::Star(em, 7, 3, RealTime() * 0.5f, WHITE);
    Gui::TextShadow(Endless() ? level->name : level->tag + "  " + level->name, 58, 8, 21, Palette::text);
    if (Endless()) {
        auto clock = [](float s) { return TextFormat("%02d:%02d", (int)s / 60, (int)s % 60); };
        std::string now = clock(surviveTime);
        Gui::Text("生存 " + now + "   最佳 " + clock(Progress::SurvivalBestTime()), 58, 34, 13, Palette::cyan);
    } else {
        Gui::Text(level->subtitle, 58, 34, 13, Palette::muted);
    }

    auto chip = [](float x, float w, const std::string& value, Color color, float pulse) {
        Draw::RoundedPanel({x, 12, w, 36}, 10, {8, 12, 28, 200}, WithAlpha(color, 0.5f + pulse * 0.5f), 1.5f);
        Gui::TextShadow(value, x + 38, 18 - pulse * 2, 21 + pulse * 3, Mix(Palette::text, color, pulse));
    };
    if (Siege()) {
        float ratio = citadelMax > 0 ? Clamp(citadel / citadelMax, 0.0f, 1.0f) : 0;
        chip(236, 108, TextFormat("%d%%", (int)std::ceil(ratio * 100)), ratio > 0.3f ? Palette::cyan : Palette::red, livesPulse);
        Vector2 c{256, 30};
        Draw::Quad({c.x - 5, c.y + 9}, {c.x + 5, c.y + 9}, {c.x + 2, c.y - 6}, {c.x - 2, c.y - 6}, {200, 222, 250, 255});
        Draw::BlendAdd();
        Draw::Glow({c.x, c.y - 9}, 8, WithAlpha({255, 220, 150, 255}, 0.9f));
        Draw::BlendNormal();
    } else {
        chip(236, 108, TextFormat("%d", lives), {255, 90, 110, 255}, livesPulse);
        Gui::Heart({256, 30}, 10 * (1 + livesPulse * 0.3f));
    }
    chip(354, 118, TextFormat("%d", gold), Palette::gold, goldPulse);
    Gui::Coin({374, 30}, 10 * (1 + goldPulse * 0.2f));
    int total = (int)level->waves.size();
    chip(482, 128, Endless() ? TextFormat("第 %d 波", std::max(0, waveIndex + 1)) : TextFormat("%d / %d", std::max(0, waveIndex + 1), total),
         Palette::cyan, 0);
    {
        Vector2 c{502, 30};
        for (int i = 0; i < 3; ++i) Draw::Arc({c.x - 6, c.y}, 5.0f + i * 4, 2, -PI / 3, PI / 3, Palette::cyan, 8);
    }

    // Next wave controls
    Rectangle callBtn{624, 10, 214, 40};
    if (CanStartWave() && !paused) {
        std::string label =
            waveIndex < 0 ? "开始进攻  [空格]" : TextFormat("召唤下一波  %ds", (int)std::ceil(std::max(0.0f, nextWaveTimer)));
        float pulse = waveIndex < 0 ? 0.5f + 0.5f * std::sin(RealTime() * 5) : 0;
        if (pulse > 0) {
            Draw::BlendAdd();
            Draw::GlowStretched({callBtn.x + callBtn.width / 2, callBtn.y + callBtn.height / 2}, 140, 34, 0,
                                WithAlpha(Palette::gold, 0.25f * pulse));
            Draw::BlendNormal();
        }
        if (Gui::Button(callBtn, label, Gui::Style::Gold, true, 17)) StartNextWave(waveIndex >= 0);
    } else {
        bool lastWave = waveIndex >= total - 1;
        Draw::RoundedPanel(callBtn, 10, {8, 12, 28, 200}, Palette::line, 1.5f);
        Gui::Text(lastWave ? "最终波 · 坚守到底" : "怪兽来袭中…", callBtn.x + callBtn.width / 2, callBtn.y + 11, 16,
                  lastWave ? Palette::red : Palette::muted, Gui::Align::Center, true);
    }

    // Next wave preview
    if (waveIndex + 1 < total) {
        const WaveDef& next = level->waves[waveIndex + 1];
        float x = 852;
        Gui::Text("下一波", x, 8, 12, Palette::muted);
        x += 2;
        for (size_t i = 0; i < next.groups.size() && i < 4; ++i) {
            const auto& g = next.groups[i];
            Rectangle slot{x + i * 54.0f, 24, 50, 30};
            Draw::RoundedPanel(slot, 6, EnemyInfo(g.enemy).boss ? Color{80, 20, 30, 220} : Color{8, 12, 28, 200},
                               EnemyInfo(g.enemy).boss ? Palette::red : Palette::line, 1);
            Art::KaijuPose pose;
            pose.feet = {slot.x + 15, slot.y + 28};
            pose.scale = 0.42f / std::max(1.0f, EnemyInfo(g.enemy).size * 0.8f);
            pose.time = RealTime();
            Art::DrawKaiju(g.enemy, pose);
            Gui::Text(TextFormat("x%d", g.count), slot.x + 47, slot.y + 8, 13, Palette::text, Gui::Align::Right, true);
        }
    }

    // Speed & pause
    Rectangle speedBtn{1142, 10, 64, 40}, pauseBtn{1214, 10, 54, 40};
    if (Gui::Button(speedBtn, "", speedIndex == 0 ? Gui::Style::Dark : Gui::Style::Blue)) CycleSpeed();
    Gui::FastIcon({speedBtn.x + 24, speedBtn.y + 19}, 8, WHITE, speedIndex + 1);
    Gui::Text(TextFormat("%dx", speedIndex + 1), speedBtn.x + 58, speedBtn.y + 12, 14, WHITE, Gui::Align::Right, true);
    if (Gui::Button(pauseBtn, "", Gui::Style::Dark) && Running()) SetPaused(!paused);
    Gui::PauseIcon({pauseBtn.x + 27, pauseBtn.y + 19}, 9, WHITE);
}

void BattleScene::DrawPanel() {
    Rectangle panel{kPanelX, kMapY, kPanelW, kCanvasH - kMapY};
    UI::Capture(panel);
    Draw::VerticalGradientRect(panel, {20, 28, 58, 255}, {10, 14, 32, 255});
    DrawRectangleRec({kPanelX, kMapY, 2, panel.height}, WithAlpha(Palette::line, 0.9f));
    Gui::Header({kPanelX + 8, kMapY + 6, kPanelW - 16, 26}, "光之巨人", Palette::gold);
    Gui::Text("按 1-6 快速选择", kPanelX + kPanelW - 10, kMapY + 12, 11, Palette::muted, Gui::Align::Right);
    hoverCard = -1;
    for (int i = 0; i < (int)roster.size(); ++i) {
        Rectangle r = CardRect(i);
        if (Gui::Hover(r)) hoverCard = (int)roster[i];
        DrawHeroCard(i, r);
    }
    DrawSkills();
    DrawFusion();
    DrawInfo(InfoRect());
}

void BattleScene::DrawHeroCard(int slot, Rectangle r) {
    HeroId id = roster[slot];
    const HeroDef& def = HeroInfo(id);
    bool afford = gold >= def.cost;
    bool active = placing == (int)id;
    bool hover = hoverCard == (int)id && outcome == Outcome::None && !paused;
    float lift = hover ? -2.0f : 0.0f;
    Rectangle body{r.x, r.y + lift, r.width, r.height};
    Draw::RoundedPanel({r.x + 1, r.y + 4, r.width, r.height}, 10, WithAlpha(BLACK, 0.4f), BLANK, 0);
    Color top = Mix(def.primary, {12, 16, 34, 255}, 0.45f), bottom = Mix(def.primary, {8, 10, 24, 255}, 0.82f);
    Draw::RoundedPanel(body, 10, bottom, BLANK, 0);
    DrawRectangleGradientV((int)body.x + 2, (int)body.y + 2, (int)body.width - 4, 56, top, WithAlpha(bottom, 0));
    Draw::Glow({body.x + body.width / 2, body.y + 36}, 40, WithAlpha(Lighten(def.primary, 0.3f), 0.35f));
    Art::DrawPortrait(id, {body.x + 12, body.y + 1, 68, 68});
    Draw::RoundedPanel({body.x + 4, body.y + 66, body.width - 8, 28}, 6, {6, 8, 20, 210}, BLANK, 0);
    Gui::Text(def.name, body.x + body.width / 2, body.y + 66, 12, Palette::text, Gui::Align::Center, true);
    Gui::Coin({body.x + 28, body.y + 86}, 5);
    Gui::Text(TextFormat("%d", def.cost), body.x + 36, body.y + 79, 13, afford ? Palette::gold : Palette::red, Gui::Align::Left, true);
    Color border = active ? Palette::gold : hover ? Lighten(def.primary, 0.5f) : WithAlpha(Palette::line, 0.9f);
    Draw::RoundedPanel(body, 10, BLANK, border, active ? 2.6f : 1.5f);
    Draw::RoundedPanel({body.x + 4, body.y + 4, 18, 18}, 5, {6, 8, 20, 220}, WithAlpha(Palette::line, 0.8f), 1);
    Gui::Text(TextFormat("%d", slot + 1), body.x + 13, body.y + 5, 13, Palette::muted, Gui::Align::Center, true);
    if (!afford) Draw::RoundedPanel(body, 10, {4, 6, 14, 120}, BLANK, 0);
    if (active) {
        Draw::BlendAdd();
        Draw::GlowStretched({body.x + body.width / 2, body.y + body.height / 2}, 60, 74, 0,
                            WithAlpha(Palette::gold, 0.22f + 0.08f * std::sin(RealTime() * 6)));
        Draw::BlendNormal();
    }
    if (outcome == Outcome::None && !paused && !Cinematic() && Gui::Clicked(r)) {
        if (afford) {
            placing = placing == (int)id ? -1 : (int)id;
            selected = -1;
            targetingStrike = false;
        } else {
            Sfx::Play("error", 0.5f);
        }
    }
}

void BattleScene::DrawFusion() {
    Rectangle r = FusionRect();
    int have = RosterCount();
    int need = (int)roster.size();
    bool assembled = have >= need;
    bool ready = assembled && fusionCooldown <= 0 && waveIndex >= 0 && outcome == Outcome::None;
    bool charging = assembled && fusionCooldown > 0 && outcome == Outcome::None;
    bool hover = ready && !paused && Gui::Hover(r);
    float t = RealTime();
    const Color gold{255, 206, 80, 255};

    // Body: dark when assembling, navy while charging, gold gradient once ready.
    if (ready) {
        Draw::BlendAdd();
        Draw::GlowStretched({r.x + r.width / 2, r.y + r.height / 2}, 118, 40, 0, WithAlpha({255, 170, 60, 255}, 0.3f + 0.12f * std::sin(t * 5)));
        Draw::BlendNormal();
        Draw::RoundedPanel(r, 10, {150, 92, 16, 255}, BLANK, 0);
        DrawRectangleGradientV((int)r.x + 2, (int)r.y + 2, (int)r.width - 4, (int)r.height - 4, hover ? Color{255, 214, 110, 255} : Color{240, 184, 64, 255},
                               {150, 80, 10, 255});
        float sweep = std::fmod(t * 140, r.width + 80) - 40;
        Draw::BlendAdd();
        Draw::Quad({r.x + sweep, r.y + 2}, {r.x + sweep + 18, r.y + 2}, {r.x + sweep + 4, r.y + r.height - 2}, {r.x + sweep - 14, r.y + r.height - 2},
                   WithAlpha(WHITE, 0.35f));
        Draw::BlendNormal();
        Draw::RoundedPanel(r, 10, BLANK, WithAlpha({255, 240, 190, 255}, 0.6f + 0.4f * std::sin(t * 6)), 2.4f);
    } else {
        Draw::RoundedPanel(r, 10, charging ? Color{18, 22, 44, 245} : Color{24, 26, 34, 245}, charging ? WithAlpha(gold, 0.45f) : Color{60, 64, 76, 255},
                           1.4f);
    }

    // One slot per roster hero: lit once placed, a dark silhouette until then.
    const float slot = 24, gap = 4;
    float sx = r.x + (r.width - need * slot - (need - 1) * gap) / 2;
    for (int i = 0; i < need; ++i) {
        HeroId id = roster[i];
        bool placed = std::any_of(heroes.begin(), heroes.end(), [id](const Ref<HeroActor>& h) { return h->hero == id; });
        Rectangle s{sx + i * (slot + gap), r.y + 5, slot, slot};
        Color ring = ready ? Color{255, 250, 220, 255} : placed ? WithAlpha(HeroInfo(id).beam, 0.9f) : Color{56, 60, 72, 255};
        DrawCircleV({s.x + slot / 2, s.y + slot / 2}, slot / 2 + 1.5f, ring);
        DrawCircleV({s.x + slot / 2, s.y + slot / 2}, slot / 2, placed ? Mix(HeroInfo(id).primary, {10, 12, 24, 255}, 0.5f) : Color{14, 16, 22, 255});
        Art::DrawPortrait(id, {s.x - 3, s.y - 2, slot + 6, slot + 6}, placed ? WHITE : Color{30, 32, 40, 200});
    }

    std::string label;
    Color labelTone = Palette::muted;
    if (ready) {
        label = "究极合体  E";
        labelTone = {60, 24, 0, 255};
    } else if (!assembled) {
        label = TextFormat("集结 %d / %d", have, need);
    } else if (charging) {
        label = TextFormat("蓄能中  %d 秒", (int)std::ceil(fusionCooldown));
        labelTone = gold;
    } else {
        label = "全员集结";
        labelTone = gold;
    }
    Gui::Text(label, r.x + r.width / 2, r.y + 33, 14, labelTone, Gui::Align::Center, true);
    if (charging) {
        float k = 1 - Clamp(fusionCooldown / std::max(1.0f, fusionSpan), 0.0f, 1.0f);
        Rectangle bar{r.x + 8, r.y + r.height - 6, r.width - 16, 3};
        DrawRectangleRec(bar, {8, 10, 18, 255});
        DrawRectangleRec({bar.x, bar.y, bar.width * k, bar.height}, gold);
    }
    if (ready && !paused && Gui::Clicked(r)) CastFusion();
}

void BattleScene::DrawSkills() {
    for (int i = 0; i < 2; ++i) {
        Rectangle r = SkillRect(i);
        float cd = i == 0 ? brothersCooldown : strikeCooldown;
        float max = i == 0 ? kBrothersMax : kStrikeMax;
        bool ready = cd <= 0 && waveIndex >= 0 && outcome == Outcome::None;
        bool hover = Gui::Hover(r) && ready && !paused;
        Color tone = i == 0 ? Palette::cyan : Color{255, 150, 70, 255};
        // Unavailable skills read as switched off: grey body, desaturated icon, no glow.
        auto ink = [ready](Color c) { return ready ? c : Darken(Desaturate(c), 0.45f); };
        Color fill = ready ? Mix(tone, {10, 14, 30, 255}, hover ? 0.55f : 0.74f) : Color{26, 28, 36, 245};
        Draw::RoundedPanel(r, 10, fill, ready ? WithAlpha(tone, 0.6f + 0.4f * std::sin(RealTime() * 5)) : Color{60, 64, 76, 255},
                           ready ? 2.4f : 1.2f);
        Vector2 c{r.x + r.width / 2, r.y + 18};
        if (ready) {
            Draw::BlendAdd();
            Draw::Glow(c, 26, WithAlpha(tone, 0.35f + 0.15f * std::sin(RealTime() * 5)));
            Draw::BlendNormal();
        }
        if (i == 0) {
            if (ready) Draw::BlendAdd();
            for (int k = 0; k < 5; ++k) {
                float a = -PI / 2 + (k - 2) * 0.36f;
                Color beam = HeroInfo(roster[k % roster.size()]).beam;
                Draw::Line(c, Add(c, Mul(Dir(a), 20)), 3, WithAlpha(ink(beam), 0.9f));
            }
            if (ready) Draw::Glow(c, 16, WithAlpha(WHITE, 0.8f));
            if (ready) Draw::BlendNormal();
            Draw::Star(c, 9, 4, ready ? RealTime() : 0, ink(WHITE));
        } else {
            Draw::Quad({c.x + 18, c.y}, {c.x - 14, c.y - 4}, {c.x - 16, c.y}, {c.x - 14, c.y + 4}, ink({220, 224, 232, 255}));
            Draw::Tri({c.x + 2, c.y - 2}, {c.x - 10, c.y - 16}, {c.x - 14, c.y - 14}, ink({190, 196, 210, 255}));
            Draw::Tri({c.x + 2, c.y + 2}, {c.x - 10, c.y + 16}, {c.x - 14, c.y + 14}, ink({170, 176, 192, 255}));
            Draw::Line({c.x - 14, c.y}, {c.x + 16, c.y}, 1.6f, ink({236, 120, 40, 255}));
        }
        Gui::Text(i == 0 ? "合体光线" : "空中支援", c.x, r.y + 33, 12, ready ? Palette::text : Color{110, 114, 126, 255}, Gui::Align::Center,
                  true);
        Draw::RoundedPanel({r.x + 4, r.y + 4, 16, 16}, 4, ready ? Mix(tone, {6, 8, 20, 255}, 0.4f) : Color{14, 16, 22, 230}, BLANK, 0);
        Gui::Text(i == 0 ? "Q" : "W", r.x + 12, r.y + 5, 12, ready ? WHITE : Color{90, 94, 104, 255}, Gui::Align::Center, true);
        if (!ready) {
            float k = waveIndex < 0 ? 1.0f : Clamp(cd / max, 0.0f, 1.0f);
            // Clock wipe over the icon plus a refill bar along the bottom.
            DrawCircleSector(c, 19, -90, -90 + 360 * k, 32, {6, 8, 12, 170});
            Rectangle bar{r.x + 8, r.y + r.height - 7, r.width - 16, 3};
            DrawRectangleRec(bar, {10, 12, 18, 255});
            DrawRectangleRec({bar.x, bar.y, bar.width * (1 - k), bar.height}, WithAlpha(tone, 0.8f));
            if (waveIndex >= 0 && outcome == Outcome::None)
                Gui::TextOutline(TextFormat("%d", (int)std::ceil(cd)), c.x, c.y - 11, 20, WHITE, {0, 0, 0, 255}, 1.6f);
        }
        if (ready && !paused && !Cinematic() && Gui::Clicked(r)) {
            if (i == 0) CastBrothers();
            else {
                targetingStrike = !targetingStrike;
                placing = -1;
            }
        }
    }
}

void BattleScene::DrawInfo(Rectangle area) {
    Gui::Panel(area, {12, 18, 38, 240}, 10);
    float x = area.x + 10, y = area.y + 8, w = area.width - 20;
    if (HeroActor* t = FindTower(selected)) {
        const HeroDef& def = HeroInfo(t->hero);
        Draw::RoundedPanel({x, y, 42, 42}, 8, Mix(def.primary, {10, 14, 30, 255}, 0.6f), BLANK, 0);
        Art::DrawPortrait(t->hero, {x - 4, y - 4, 50, 50});
        Gui::Text(def.name, x + 50, y, 15, Palette::text, Gui::Align::Left, true);
        for (int i = 0; i < 3; ++i) Gui::StarIcon({x + 57 + i * 15.0f, y + 29}, 6, i <= t->level);
        Gui::Text(TextFormat("击杀 %d", t->kills), x + w, y + 22, 12, Palette::muted, Gui::Align::Right);
        y += 50;
        float dmg = HeroDamage(t->hero, t->level), cd = HeroCooldown(t->hero, t->level);
        Gui::Text(TextFormat("伤害 %.0f", dmg), x, y, 13, Palette::text);
        Gui::Text(TextFormat("攻速 %.2f/秒", 1 / cd), x + w, y, 13, Palette::text, Gui::Align::Right);
        y += 18;
        Gui::Text(TextFormat("射程 %.1f", HeroRange(t->hero, t->level) / kTile), x, y, 13, Palette::text);
        Gui::Text(TextFormat("输出 %.0f", t->dealt), x + w, y, 13, Palette::text, Gui::Align::Right);
        y += 18;
        if (Assault() > 0) {
            bool down = t->disabled == HeroActor::Disable::Exhausted;
            Gui::Text(down ? TextFormat("能量耗尽 · %.0f 秒后站起", std::ceil(t->disabledTime)) : TextFormat("能量 %.0f / %.0f", t->hp, t->maxHp), x, y,
                      13, down || t->hp < t->maxHp * 0.35f ? Palette::red : Palette::cyan);
            y += 18;
        }
        Gui::Paragraph(def.perk, x, y, w, 11, t->level >= 2 ? Palette::gold : Palette::muted, 1.35f);
        float by = area.y + area.height - 34;
        Rectangle modeBtn{x - 2, by, 54, 28}, upBtn{x + 56, by, 70, 28}, sellBtn{x + 130, by, 52, 28};
        if (Gui::Button(modeBtn, kModeNames[t->mode], Gui::Style::Dark, true, 13)) t->mode = (t->mode + 1) % 3;
        int cost = UpgradeCost(t->hero, t->level);
        if (cost > 0) {
            if (Gui::Button(upBtn, TextFormat("升级 %d", cost), Gui::Style::Green, gold >= cost, 13)) UpgradeTower(*t);
        } else {
            Gui::Button(upBtn, "已满级", Gui::Style::Dark, false, 13);
        }
        if (Gui::Button(sellBtn, TextFormat("卖 %d", SellValue(*t)), Gui::Style::Red, true, 13)) SellTower(t->id);
        return;
    }
    if (hoverCard >= 0) {
        const HeroDef& def = HeroInfo((HeroId)hoverCard);
        Gui::Text(def.name, x, y, 15, Palette::text, Gui::Align::Left, true);
        Gui::Text(def.role, x + w, y + 2, 12, def.beam, Gui::Align::Right, true);
        y += 22;
        Gui::Text(TextFormat("必杀 · %s", def.move), x, y, 12, Palette::gold);
        y += 18;
        y += Gui::Paragraph(def.desc, x, y, w, 11.5f, Palette::muted, 1.4f);
        y += 2;
        Gui::Text(TextFormat("伤害 %.0f   射程 %.1f   攻速 %.2f", HeroDamage((HeroId)hoverCard, 0), def.range, 1 / def.cooldown), x,
                  std::min(y, area.y + area.height - 20), 11.5f, Palette::text);
        return;
    }
    Gui::Text("作战提示", x, y, 14, Palette::gold, Gui::Align::Left, true);
    static const char* tips[] = {
        "在道路旁的空地上放置英雄，拐角处可覆盖更长的路径。",
        "点击已放置的英雄可以升级、出售或切换攻击目标。",
        "捷德的力场能让怪兽变慢，再让亚刻的光弹在怪群里爆开。",
        "红王护甲很厚，高伤害光线和欧布圣剑的破甲是克星。",
        "提前召唤下一波可以获得额外金币奖励。",
        "芝顿会展开护盾，护盾期间受到的伤害降低，可等护盾消失再集火。",
        "巴尔坦星人被击倒后会分裂，记得在后方保留火力。",
        "本关英雄全员上场后，右侧「究极合体」开始蓄能，按 E 发动。",
        "后期关卡的怪兽会攻击英雄。彩色计时器闪红时说明能量不足，升级可以补满能量。",
    };
    constexpr int kTips = (int)(sizeof(tips) / sizeof(tips[0]));
    int tip = ((int)(RealTime() / 6)) % kTips;
    Gui::Paragraph(tips[tip], x, y + 24, w, 12.5f, Palette::muted, 1.5f);
    Gui::Text("空格 召唤  F 倍速  Q/W/E 必杀  右键 取消", x, area.y + area.height - 22, 10.5f, WithAlpha(Palette::muted, 0.8f));
}

void BattleScene::DrawBanner() {
    if (bannerTime > 2.9f || bannerTitle.empty()) return;
    float t = bannerTime;
    float alpha = Clamp(t / 0.25f, 0.0f, 1.0f) * Clamp((2.9f - t) / 0.4f, 0.0f, 1.0f);
    float slide = (1 - EaseOut(t / 0.4f)) * 120;
    float cy = kMapY + kMapH * 0.3f;
    Rectangle band{kMapX, cy - 44, kMapW, 88};
    Color dark = bannerWarning ? Color{70, 6, 14, 255} : Color{8, 12, 30, 255};
    DrawRectangleGradientH((int)band.x, (int)band.y, (int)band.width / 2, (int)band.height, WithAlpha(dark, 0),
                           WithAlpha(dark, 0.82f * alpha));
    DrawRectangleGradientH((int)(band.x + band.width / 2), (int)band.y, (int)band.width / 2, (int)band.height,
                           WithAlpha(dark, 0.82f * alpha), WithAlpha(dark, 0));
    DrawRectangleGradientH((int)band.x, (int)band.y, (int)band.width / 2, 2, WithAlpha(bannerColor, 0), WithAlpha(bannerColor, alpha));
    DrawRectangleGradientH((int)(band.x + band.width / 2), (int)band.y, (int)band.width / 2, 2, WithAlpha(bannerColor, alpha),
                           WithAlpha(bannerColor, 0));
    DrawRectangleGradientH((int)band.x, (int)(band.y + band.height - 2), (int)band.width / 2, 2, WithAlpha(bannerColor, 0),
                           WithAlpha(bannerColor, alpha));
    DrawRectangleGradientH((int)(band.x + band.width / 2), (int)(band.y + band.height - 2), (int)band.width / 2, 2,
                           WithAlpha(bannerColor, alpha), WithAlpha(bannerColor, 0));
    float cx = kMapX + kMapW / 2 - slide;
    if (bannerWarning) {
        bool on = std::fmod(t, 0.5f) < 0.3f;
        float tw = Gui::Measure(bannerTitle, 40, true);
        Gui::WarningIcon({cx - tw / 2 - 40, cy - 14}, 16, WithAlpha(on ? Palette::red : Palette::gold, alpha));
        Gui::WarningIcon({cx + tw / 2 + 40, cy - 14}, 16, WithAlpha(on ? Palette::red : Palette::gold, alpha));
    }
    Gui::TextOutline(bannerTitle, cx, cy - 36, 40, WithAlpha(bannerColor, alpha), WithAlpha({10, 6, 16, 255}, alpha), 2.2f);
    Gui::TextOutline(bannerSub, cx + slide * 0.5f, cy + 10, 18, WithAlpha(Palette::text, alpha), WithAlpha({10, 6, 16, 255}, alpha), 1.4f,
                     Gui::Align::Center, false);
}

void BattleScene::DrawBossBar() {
    const MonsterActor* boss = nullptr;
    for (const auto& e : monsters)
        if (!e->dead && e->Def().boss && (!boss || e->hp > boss->hp)) boss = e.get();
    if (!boss) return;
    const EnemyDef& def = EnemyInfo(boss->type);
    Rectangle r{kMapX + kMapW / 2 - 260, kMapY + 8, 520, 40};
    Draw::RoundedPanel(r, 10, {14, 6, 16, 225}, WithAlpha(Palette::red, 0.8f), 1.5f);
    Gui::Text(TextFormat("%s · %s", def.title, def.name), r.x + 12, r.y + 4, 14, Palette::text, Gui::Align::Left, true);
    std::string status;
    if (boss->shieldTime > 0) status = TextFormat("护盾展开！伤害 -%d%%", (int)std::lround((1 - kShieldTaken) * 100));
    else if (boss->phaseTime > 0) status = "穿越异次元，无法攻击";
    else if (boss->type == EnemyId::Gatanothor) status = boss->enraged ? "外壳碎裂" : "外壳坚硬，护甲极高";
    else if (boss->enraged) status = "暴怒";
    if (!status.empty())
        Gui::Text(status, r.x + r.width - 12, r.y + 4, 13, boss->shieldTime > 0 ? Palette::gold : Palette::red, Gui::Align::Right, true);
    float ratio = Clamp(boss->hp / boss->maxHp, 0.0f, 1.0f);
    Rectangle bar{r.x + 12, r.y + 24, r.width - 24, 9};
    DrawRectangleRounded(bar, 0.5f, 6, {40, 10, 16, 255});
    if (ratio > 0)
        DrawRectangleRounded({bar.x, bar.y, bar.width * ratio, bar.height}, 0.5f, 6,
                             boss->shieldTime > 0 ? Color{255, 210, 90, 255} : Color{230, 50, 60, 255});
    DrawRectangleRec({bar.x + 3, bar.y + 1.5f, (bar.width - 6) * ratio, 2}, WithAlpha(WHITE, 0.35f));
}

void BattleScene::DrawEnemyTooltip() {
    if (placing >= 0 || targetingStrike || paused || outcome != Outcome::None) return;
    Vector2 m = Input::GetMousePosition();
    if (!CheckCollisionPointRec(m, MapRect())) return;
    const MonsterActor* hit = nullptr;
    for (const auto& e : monsters)
        if (!e->dead && Len(Sub(e->Center(), m)) < 18 * e->Scale()) hit = e.get();
    if (!hit) return;
    const EnemyDef& def = EnemyInfo(hit->type);
    Rectangle r{std::min(m.x + 16, kMapX + kMapW - 210), std::max(m.y - 80, kMapY + 4), 200, 70};
    Gui::Panel(r, {10, 14, 30, 240}, 8);
    Gui::Text(def.name, r.x + 10, r.y + 6, 15, Palette::text, Gui::Align::Left, true);
    Gui::Text(def.title, r.x + r.width - 10, r.y + 8, 12, Palette::muted, Gui::Align::Right);
    Gui::Text(TextFormat("生命 %.0f / %.0f", std::max(0.0f, hit->hp), hit->maxHp), r.x + 10, r.y + 28, 12.5f, Palette::text);
    Gui::Text(TextFormat("护甲 %.0f", hit->armor), r.x + r.width - 10, r.y + 28, 12.5f, Palette::text, Gui::Align::Right);
    std::string status;
    if (hit->slowTime > 0) status += "减速 ";
    if (hit->burnTime > 0) status += "灼烧 ";
    if (hit->breakTime > 0) status += "破甲 ";
    if (hit->stunTime > 0) status += "眩晕 ";
    if (hit->vulnTime > 0) status += "易伤 ";
    Gui::Text(status.empty() ? "状态 正常" : "状态 " + status, r.x + 10, r.y + 47, 12, status.empty() ? Palette::muted : Palette::cyan);
}

void BattleScene::DrawIntroHint() {
    if (waveIndex >= 0 || outcome != Outcome::None || paused) return;
    Rectangle r{kMapX + kMapW / 2 - 330, kMapY + kMapH - 70, 660, 52};
    Gui::Panel(r, {10, 14, 32, 230}, 12);
    float a = 0.6f + 0.4f * std::sin(RealTime() * 3);
    Gui::Text("1. 点击右侧英雄卡片    2. 在道路旁的草地放置    3. 按空格或点击「开始进攻」", r.x + r.width / 2, r.y + 16, 16,
              WithAlpha(Palette::text, a), Gui::Align::Center, true);
}

void BattleScene::DrawPauseMenu() {
    if (!paused || outcome != Outcome::None) return;
    DrawRectangle(0, 0, kCanvasW, kCanvasH, {4, 6, 16, 190});
    Rectangle r{kCanvasW / 2.0f - 190, kCanvasH / 2.0f - 200, 380, 400};
    Gui::Panel(r, Palette::panel, 16);
    Gui::TextOutline("暂停", r.x + r.width / 2, r.y + 22, 40, Palette::gold, {20, 10, 0, 255}, 2);
    float bx = r.x + 50, bw = r.width - 100, by = r.y + 92;
    auto& opt = Progress::Options();
    if (Gui::Button({bx, by, bw, 46}, "继续作战", Gui::Style::Gold, true, 19)) SetPaused(false);
    if (Gui::Button({bx, by + 58, bw, 46}, "重新开始", Gui::Style::Blue, true, 19)) {
        SetPaused(false);
        restartRequested = true;
    }
    if (Gui::Button({bx, by + 116, bw / 2 - 6, 46}, opt.sfx ? "音效：开" : "音效：关", Gui::Style::Dark, true, 17)) {
        opt.sfx = !opt.sfx;
        Sfx::ApplySettings();
        Progress::Save();
    }
    if (Gui::Button({bx + bw / 2 + 6, by + 116, bw / 2 - 6, 46}, opt.music ? "音乐：开" : "音乐：关", Gui::Style::Dark, true, 17)) {
        opt.music = !opt.music;
        Sfx::ApplySettings();
        Progress::Save();
    }
    if (Gui::Button({bx, by + 174, bw, 46}, Endless() ? "返回主页" : "返回关卡选择", Gui::Style::Red, true, 19))
        GoToMenu(Endless() ? MenuPage::Title : MenuPage::Levels);
    Gui::Text("Esc / P 继续", r.x + r.width / 2, r.y + r.height - 34, 14, Palette::muted, Gui::Align::Center);
}

void BattleScene::DrawResult() {
    if (outcome == Outcome::None || outcomeTime < 0.8f) return;
    float t = outcomeTime - 0.8f;
    float a = Clamp(t / 0.4f, 0.0f, 1.0f);
    DrawRectangle(0, 0, kCanvasW, kCanvasH, WithAlpha({4, 6, 16, 255}, 0.75f * a));
    bool win = outcome == Outcome::Victory;
    float pop = EaseOut(t / 0.45f);
    Rectangle r{kCanvasW / 2.0f - 270, kCanvasH / 2.0f - 210 + (1 - pop) * 40, 540, 420};
    Color accent = win ? Palette::gold : Palette::red;
    Draw::BlendAdd();
    Draw::Glow({r.x + r.width / 2, r.y + 40}, 260, WithAlpha(accent, 0.2f * a));
    Draw::BlendNormal();
    Gui::Panel(r, {14, 20, 42, (unsigned char)(245 * a)}, 18);
    if (Endless()) {
        auto clock = [](float s) { return std::string(TextFormat("%d 分 %02d 秒", (int)s / 60, (int)s % 60)); };
        Color tone = newRecord ? Palette::gold : Palette::cyan;
        Gui::Text("SURVIVAL", r.x + r.width / 2, r.y + 22, 14, WithAlpha(Palette::muted, a), Gui::Align::Center, true);
        Gui::TextOutline(newRecord ? "新纪录！" : Siege() ? "灯塔被摧毁" : "防线崩溃", r.x + r.width / 2, r.y + 42, 52, WithAlpha(tone, a),
                         WithAlpha({20, 8, 0, 255}, a), 2.5f);
        if (newRecord && Siege())
            Gui::Text("守护灯塔最终还是熄灭了", r.x + r.width / 2, r.y + 100, 15, WithAlpha(Palette::muted, a), Gui::Align::Center);
        Gui::Text("坚持了 " + clock(surviveTime), r.x + r.width / 2, r.y + 130, 28, WithAlpha(Palette::text, a), Gui::Align::Center, true);
        Gui::Text(TextFormat("到达第 %d 波    击败怪兽 %d", std::max(0, waveIndex + 1), kills), r.x + r.width / 2, r.y + 184, 18,
                  WithAlpha(Palette::text, a), Gui::Align::Center);
        Gui::Text(TextFormat("最佳纪录  %s · 第 %d 波", clock(Progress::SurvivalBestTime()).c_str(), Progress::SurvivalBestWave()), r.x + r.width / 2,
                  r.y + 222, 16, WithAlpha(Palette::muted, a), Gui::Align::Center);
        if (t < 0.6f) return;
        float by = r.y + r.height - 86;
        if (Gui::Button({r.x + 60, by, 200, 50}, "返回主页", Gui::Style::Dark, true, 18)) GoToMenu(MenuPage::Title);
        if (Gui::Button({r.x + 280, by, 200, 50}, "再来一次", Gui::Style::Gold, true, 20)) restartRequested = true;
        return;
    }
    Gui::Text(win ? "MISSION COMPLETE" : "MISSION FAILED", r.x + r.width / 2, r.y + 22, 14, WithAlpha(Palette::muted, a),
              Gui::Align::Center, true);
    Gui::TextOutline(win ? "守护成功！" : "基地失守", r.x + r.width / 2, r.y + 42, 52, WithAlpha(accent, a), WithAlpha({20, 8, 0, 255}, a),
                     2.5f);
    if (win) {
        for (int i = 0; i < 3; ++i) {
            float st = Clamp((t - 0.4f - i * 0.25f) / 0.3f, 0.0f, 1.0f);
            float s = st > 0 ? 1 + 0.4f * std::sin(st * PI) : 0;
            Vector2 c{r.x + r.width / 2 + (i - 1) * 74.0f, r.y + 150 - (i == 1 ? 12.0f : 0.0f)};
            Gui::StarIcon(c, 26, false);
            if (i < earnedStars && st > 0) Gui::StarIcon(c, 26 * s, true);
        }
    } else {
        Gui::Text("怪兽突破了防线……调整英雄布局，再来一次！", r.x + r.width / 2, r.y + 136, 17, WithAlpha(Palette::text, a),
                  Gui::Align::Center);
    }
    Gui::Text(TextFormat("坚守波次  %d / %d", std::max(0, waveIndex + 1), (int)level->waves.size()), r.x + 120, r.y + 200, 17,
              WithAlpha(Palette::text, a));
    Gui::Text(TextFormat("击败怪兽  %d", kills), r.x + r.width - 120, r.y + 200, 17, WithAlpha(Palette::text, a), Gui::Align::Right);
    Gui::Text(TextFormat("剩余生命  %d", lives), r.x + 120, r.y + 230, 17, WithAlpha(Palette::text, a));
    Gui::Text(TextFormat("英雄数量  %d", (int)heroes.size()), r.x + r.width - 120, r.y + 230, 17, WithAlpha(Palette::text, a),
              Gui::Align::Right);
    if (t < 0.6f) return;
    float by = r.y + r.height - 86;
    bool hasNext = levelIndex + 1 < (int)Levels().size();
    if (win && hasNext) {
        if (Gui::Button({r.x + 30, by, 150, 50}, "关卡选择", Gui::Style::Dark, true, 18)) GoToMenu(MenuPage::Levels);
        if (Gui::Button({r.x + 195, by, 150, 50}, "再来一次", Gui::Style::Blue, true, 18)) restartRequested = true;
        if (Gui::Button({r.x + 360, by, 150, 50}, "下一关", Gui::Style::Gold, true, 20)) {
            PrepareLevel(levelIndex + 1);
            GoToMenu(MenuPage::Prepare);
        }
    } else {
        if (Gui::Button({r.x + 60, by, 200, 50}, "关卡选择", Gui::Style::Dark, true, 18)) GoToMenu(MenuPage::Levels);
        if (Gui::Button({r.x + 280, by, 200, 50}, win ? "再来一次" : "再试一次", Gui::Style::Gold, true, 20)) restartRequested = true;
    }
}

} // namespace UG
