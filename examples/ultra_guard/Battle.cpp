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
constexpr float kFusionMax = 52;
constexpr float kWaveGap = 16;
constexpr unsigned kKaijuMask = 1u << kKaijuLayer;
constexpr const char* kModeNames[] = {"最前", "最强", "最近"};

Rectangle MapRect() { return {kMapX, kMapY, kMapW, kMapH}; }
Rectangle CardRect(int i) { return {1086 + (i % 2) * 98.0f, 90 + (i / 2) * 104.0f, 92, 98}; }
Rectangle SkillRect(int i) { return {1086 + i * 98.0f, 404, 92, 52}; }
Rectangle FusionRect() { return {1086, 460, 190, 40}; }
Rectangle InfoRect() { return {1086, 506, 190, 206}; }

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
    for (float* timer : {&slowTime, &burnTime, &breakTime, &vulnTime, &stunTime, &shieldTime}) *timer = std::max(0.0f, *timer - dt);
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
    }

    float slow = slowTime > 0 ? slowAmount * (1 - def.slowResist) : 0;
    float v = stunTime > 0 ? 0 : def.speed * kTile * speedMul * (1 - slow);
    dist += v * dt;
    gameObject->transform->position = line.Sample(dist, &dir);
    if (dir.x > 0.2f) face = 1;
    else if (dir.x < -0.2f) face = -1;
    if (dist >= line.length) b.Leak(*this);
}

void MonsterActor::Render() {
    if (dead) return;
    Art::KaijuPose pose;
    pose.feet = Add(Feet(), {0, (1 - appear) * 6});
    pose.scale = Scale() * (0.6f + 0.4f * appear);
    pose.time = anim;
    pose.face = face;
    pose.flash = flash;
    pose.alpha = appear;
    pose.chill = slowTime > 0 ? std::min(1.0f, slowTime) : 0;
    pose.burn = burnTime > 0 ? 1.0f : 0.0f;
    pose.shielded = shieldTime > 0;
    pose.enraged = enraged;
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
    flash = std::max(0.0f, flash - dt * 4);
    cooldown -= dt;
    if (cooldown > 0) return;
    MonsterActor* best = nullptr;
    float bestScore = -1e30f;
    for (MonsterActor* e : battle->KaijuInCircle(Pos(), HeroRange(hero, level))) {
        if (e->appear < 0.3f) continue;
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
    cooldown = HeroCooldown(hero, level);
}

void HeroActor::Render() {
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
    Rectangle dest{p.x - size / 2, p.y + 10 - size * 0.92f + bob, size, size};
    Art::DrawPortrait(hero, dest);
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

void BattleScene::OnLoad() {
    level = &Levels()[levelIndex];
    map.Build(*level);
    if (!mapTexture.id && !headless) mapTexture = RenderMap(map, 2.0f);
    for (auto& row : towerGrid) row.fill(-1);
    monsters.clear();
    heroes.clear();
    strikes.clear();
    spawning.clear();
    CreateEmitters();
    gold = level->gold;
    lives = level->lives;
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
    flashScreen = shake = baseFlash = goldPulse = livesPulse = 0;
    ApplySpeed();
    Banner(level->name, level->subtitle, Palette::gold);
    EventBus::Subscribe("ug.kill", [this](const EventData&) { goldPulse = 1; });
    Sfx::PlayMusic("battle");
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
        emitter->maxParticles = 900;
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
    if (restartRequested) {
        restartRequested = false;
        Restart();
        return;
    }
    float real = GameTime::UnscaledDeltaTime();
    HandleInput();
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
    if (!headless) SetGameSpeed(paused ? 0.0f : kSpeeds[speedIndex]);
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
    if (paused) return;

    for (int i = 0; i < kHeroCount; ++i) {
        if (Input::GetKeyDown(KEY_ONE + i)) {
            placing = placing == i ? -1 : i;
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
    }

    Scene::FixedUpdate(); // kaiju walk and use abilities, heroes attack, projectiles fly
    monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](const Ref<MonsterActor>& m) { return m->dead; }), monsters.end());

    if (Running()) {
        if (lives <= 0) {
            lives = 0;
            SetOutcome(Outcome::Defeat);
        } else if (waveIndex == (int)level->waves.size() - 1 && spawning.empty() && monsters.empty()) {
            SetOutcome(Outcome::Victory);
        }
    }
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
    if (waveIndex >= 0) gold += 20 + 5 * waveIndex;
    waveIndex++;
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
    e->armor = def.armor * (1 + 0.05f * wave);
    e->speedMul = def.boss ? 1.0f : Rand(0.94f, 1.06f);
    e->abilityA = type == EnemyId::Belial ? 6.0f : 4.0f;
    e->abilityB = 6.5f;
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

void BattleScene::Leak(MonsterActor& e) {
    const EnemyDef& def = e.Def();
    e.dead = true;
    Destroy(e.GetGameObject());
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
        float slow = t.level >= 2 ? 0.6f : def.slow;
        for (MonsterActor* e : KaijuInCircle(t.Pos(), range)) {
            DamageEnemy(*e, dmg, t.id);
            if (e->dead) continue;
            e->slowAmount = e->slowTime > 0 ? std::max(e->slowAmount, slow) : slow;
            e->slowTime = 1.8f;
            if (t.level >= 2) e->vulnTime = 2.0f;
            Sparks(e->Center(), def.beam, 3, 90);
        }
        Vector2 core = Add(t.Pos(), {0, 4});
        Burst(core, def.beam, 22, 210, 6, 0.45f);
        Burst(core, {255, 180, 60, 255}, 10, 120, 7, 0.3f);
        Burst(core, WHITE, 6, 70, 8, 0.22f);
        AddRing(core, 8, range, 0.55f, 6, def.beam);
        AddRing(core, 6, range * 0.72f, 0.45f, 3, {255, 170, 50, 255});
        AddRing(core, 4, range * 0.4f, 0.3f, 2, WHITE);
        Smoke(core, 4, 10);
        shake = std::max(shake, 0.16f);
        Sfx::Play("explode", 0.35f);
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
            Text(Add(origin, {0, -30}), "集束光线", {90, 255, 160, 255}, 13);
        }
        Sfx::Play("zero", 0.3f);
        break;
    }
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
    shake = std::max(shake, 0.35f);
}

void BattleScene::DamageEnemy(MonsterActor& e, float amount, int towerId, bool ignoreArmor) {
    if (e.dead || amount <= 0) return;
    float armor = e.breakTime > 0 ? e.armor * 0.4f : e.armor;
    float dmg = ignoreArmor ? amount : std::max(amount * 0.25f, amount - armor);
    if (e.vulnTime > 0) dmg *= 1.2f;
    if (e.shieldTime > 0) dmg *= 0.42f;
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
        Color c = HeroInfo((HeroId)(i++ % kHeroCount)).beam;
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
    std::array<bool, kHeroCount> seen{};
    int count = 0;
    for (const auto& hero : heroes)
        if (!seen[(int)hero->hero]) {
            seen[(int)hero->hero] = true;
            ++count;
        }
    return count;
}

bool BattleScene::RosterComplete() const { return RosterCount() >= kHeroCount; }

void BattleScene::CastFusion(bool force) {
    if (fusionCooldown > 0 || !Running() || waveIndex < 0) return;
    if (!force && !RosterComplete()) return;
    fusionCooldown = kFusionMax;
    Vector2 center{kMapX + kMapW * 0.5f, kMapY + kMapH * 0.45f};
    static const Color hues[] = {{255, 70, 70, 255},  {255, 196, 60, 255},  {80, 255, 170, 255},
                                 {70, 170, 255, 255}, {190, 110, 255, 255}, {255, 120, 210, 255}};
    int beam = 0;
    for (const auto& hero : heroes) {
        Color c = HeroInfo(hero->hero).beam;
        AddBeam(Add(hero->Pos(), {0, -28}), center, 8, 0.7f, c, WHITE);
        if (++beam >= 10) break;
    }
    AddBeam({center.x, kMapY - 30}, {center.x, kMapY + kMapH}, 22, 0.85f, {255, 244, 210, 255}, WHITE);
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
        AddRing(center, 10, 90.0f + i * 78, 0.5f + i * 0.08f, 6, hues[i]);
        Burst(center, hues[i], 16, 260, 7, 0.55f);
    }
    Burst(center, WHITE, 22, 140, 10, 0.35f);
    flashScreen = 1.4f;
    shake = 1.8f;
    Banner("究极合体 · 新世代闪光", "六位奥特曼同时放出必杀光线！", Palette::gold);
    Sfx::Play("fusion", 1.0f, 0);
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
    t->mode = def.kind == AttackKind::Snipe ? 1 : 0;
    t->ySort = true;
    int before = RosterCount();
    heroes.push_back(t);
    towerGrid[row][col] = t->id;
    if (before < kHeroCount && RosterComplete()) Banner("六位奥特曼集结！", "究极合体已就绪，按 E 发动", Palette::gold);
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
    if (value == Outcome::Victory) {
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
    static const HeroId order[] = {HeroId::Ultraman, HeroId::Taro, HeroId::Cosmos, HeroId::Seven, HeroId::Tiga, HeroId::Zero,
                                   HeroId::Ultraman, HeroId::Taro, HeroId::Tiga,   HeroId::Seven, HeroId::Zero, HeroId::Cosmos};
    auto alive = Alive();
    bool boss = std::any_of(alive.begin(), alive.end(), [](const auto& e) { return e->Def().boss; });
    const MonsterActor* front = nullptr;
    for (const auto& e : alive)
        if (!front || e->dist > front->dist) front = e.get();
    if (RosterComplete() && (boss || alive.size() >= 8)) CastFusion();
    if (alive.size() >= 12 || (boss && alive.size() >= 4)) CastBrothers();
    if (front && alive.size() >= 5) CastStrike(front->Center());

    if (botTiles.empty()) botTiles = RankedTiles();
    for (int guard = 0; guard < 4; ++guard) {
        HeroId next = order[botPurchases % 12];
        HeroActor* cheapest = nullptr;
        for (auto& t : heroes) {
            int cost = UpgradeCost(t->hero, t->level);
            if (cost > 0 && (!cheapest || cost < UpgradeCost(cheapest->hero, cheapest->level))) cheapest = t.get();
        }
        bool wantBuild = heroes.size() < 7 || !cheapest || botPurchases % 3 != 2;
        if (wantBuild && gold >= HeroInfo(next).cost) {
            for (const auto& tile : botTiles)
                if (towerGrid[tile.row][tile.col] < 0) {
                    BuyTower(next, tile.col, tile.row);
                    break;
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

BattleScene::BotResult BattleScene::RunBot() {
    float decide = 0;
    while (Running() && simTime < 3600) {
        if (waveIndex < 0) StartNextWave(false);
        decide -= kStep;
        if (decide <= 0) {
            decide = 0.5f;
            BotAct();
        }
        GameTime::FixedStepScope step(kStep, simTime);
        FixedUpdate();
    }
    return {outcome == Outcome::Victory, lives, waveIndex + 1, (int)heroes.size(), gold, kills, leakWave, leakCount, simTime / 60};
}

void BattleScene::DebugAutoplay(int startWave) {
    gold += 2500 + startWave * 400;
    for (int i = 0; i < 6; ++i) BotAct();
    waveIndex = startWave - 1;
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
    lives = level->lives - 3;
    waveIndex = (int)level->waves.size() - 1;
    kills = 286;
    SetOutcome(Outcome::Victory);
}

int RunBalanceReport(const std::string& path) {
    std::string report;
    SetTraceLogLevel(LOG_NONE);
    for (int i = 0; i < (int)Levels().size(); ++i) {
        for (int run = 0; run < 3; ++run) {
            SetRandomSeed(1000 + run * 77 + i);
            auto scene = CreateRef<BattleScene>(i);
            scene->headless = true;
            scene->Load();
            auto r = scene->RunBot();
            report += TextFormat("level %d run %d: %s lives=%d wave=%d towers=%d gold=%d kills=%d firstLeak=%d "
                                 "minutes=%.1f leaks:",
                                 i, run, r.won ? "WIN " : "LOSE", r.lives, r.wave, r.towers, r.gold, r.kills, r.firstLeakWave, r.minutes);
            for (int k = 0; k < kEnemyCount; ++k) report += TextFormat(" %d", r.leaks[k]);
            report += "\n";
            scene->Clear();
        }
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
    DrawPauseMenu();
    DrawResult();
}

void BattleScene::DrawWorld() {
    float s = shake * shake * 9;
    rlPushMatrix();
    if (s > 0.01f) rlTranslatef(Rand(-1, 1) * s, Rand(-1, 1) * s, 0);

    DrawMapTexture(mapTexture, MapRect());
    DrawMapLive(map, simTime, baseFlash);
    DrawPlacement();
    for (const auto& t : heroes) Draw::Shadow(Add(t->Pos(), {2, 12}), 22, 8, 0.45f);
    for (const auto& e : monsters)
        if (!e->dead) Draw::Shadow(Add(e->Feet(), {2, 1}), 15 * e->Scale(), 5 * e->Scale(), 0.45f * e->appear);

    Scene::Render(); // corpses, heroes and kaiju in depth order, then particle emitters

    for (const auto& e : monsters) DrawHealthBar(*e);

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
    Gui::TextShadow(level->name, 58, 8, 21, Palette::text);
    Gui::Text(level->subtitle, 58, 34, 13, Palette::muted);

    auto chip = [](float x, float w, const std::string& value, Color color, float pulse) {
        Draw::RoundedPanel({x, 12, w, 36}, 10, {8, 12, 28, 200}, WithAlpha(color, 0.5f + pulse * 0.5f), 1.5f);
        Gui::TextShadow(value, x + 38, 18 - pulse * 2, 21 + pulse * 3, Mix(Palette::text, color, pulse));
    };
    chip(236, 108, TextFormat("%d", lives), {255, 90, 110, 255}, livesPulse);
    Gui::Heart({256, 30}, 10 * (1 + livesPulse * 0.3f));
    chip(354, 118, TextFormat("%d", gold), Palette::gold, goldPulse);
    Gui::Coin({374, 30}, 10 * (1 + goldPulse * 0.2f));
    int total = (int)level->waves.size();
    chip(482, 128, TextFormat("%d / %d", std::max(0, waveIndex + 1), total), Palette::cyan, 0);
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
    for (int i = 0; i < kHeroCount; ++i) {
        Rectangle r = CardRect(i);
        if (Gui::Hover(r)) hoverCard = i;
        DrawHeroCard(i, r);
    }
    DrawSkills();
    DrawFusion();
    DrawInfo(InfoRect());
}

void BattleScene::DrawHeroCard(int index, Rectangle r) {
    const HeroDef& def = HeroInfo((HeroId)index);
    bool afford = gold >= def.cost;
    bool active = placing == index;
    bool hover = hoverCard == index && outcome == Outcome::None && !paused;
    float lift = hover ? -2.0f : 0.0f;
    Rectangle body{r.x, r.y + lift, r.width, r.height};
    Draw::RoundedPanel({r.x + 1, r.y + 4, r.width, r.height}, 10, WithAlpha(BLACK, 0.4f), BLANK, 0);
    Color top = Mix(def.primary, {12, 16, 34, 255}, 0.45f), bottom = Mix(def.primary, {8, 10, 24, 255}, 0.82f);
    Draw::RoundedPanel(body, 10, bottom, BLANK, 0);
    DrawRectangleGradientV((int)body.x + 2, (int)body.y + 2, (int)body.width - 4, 56, top, WithAlpha(bottom, 0));
    Draw::Glow({body.x + body.width / 2, body.y + 36}, 40, WithAlpha(Lighten(def.primary, 0.3f), 0.35f));
    Art::DrawPortrait((HeroId)index, {body.x + 12, body.y + 1, 68, 68});
    Draw::RoundedPanel({body.x + 4, body.y + 66, body.width - 8, 28}, 6, {6, 8, 20, 210}, BLANK, 0);
    Gui::Text(def.name, body.x + body.width / 2, body.y + 66, 12, Palette::text, Gui::Align::Center, true);
    Gui::Coin({body.x + 28, body.y + 86}, 5);
    Gui::Text(TextFormat("%d", def.cost), body.x + 36, body.y + 79, 13, afford ? Palette::gold : Palette::red, Gui::Align::Left, true);
    Color border = active ? Palette::gold : hover ? Lighten(def.primary, 0.5f) : WithAlpha(Palette::line, 0.9f);
    Draw::RoundedPanel(body, 10, BLANK, border, active ? 2.6f : 1.5f);
    Draw::RoundedPanel({body.x + 4, body.y + 4, 18, 18}, 5, {6, 8, 20, 220}, WithAlpha(Palette::line, 0.8f), 1);
    Gui::Text(TextFormat("%d", index + 1), body.x + 13, body.y + 5, 13, Palette::muted, Gui::Align::Center, true);
    if (!afford) Draw::RoundedPanel(body, 10, {4, 6, 14, 120}, BLANK, 0);
    if (active) {
        Draw::BlendAdd();
        Draw::GlowStretched({body.x + body.width / 2, body.y + body.height / 2}, 60, 74, 0,
                            WithAlpha(Palette::gold, 0.22f + 0.08f * std::sin(RealTime() * 6)));
        Draw::BlendNormal();
    }
    if (outcome == Outcome::None && !paused && Gui::Clicked(r)) {
        if (afford) {
            placing = placing == index ? -1 : index;
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
    bool assembled = have >= kHeroCount;
    bool ready = assembled && fusionCooldown <= 0 && waveIndex >= 0 && outcome == Outcome::None;
    bool hover = ready && !paused && Gui::Hover(r);
    Color tone = ready ? Color{255, 210, 80, 255} : Palette::muted;
    Draw::RoundedPanel(r, 8, Mix(tone, {8, 10, 24, 255}, hover ? 0.45f : 0.78f),
                       ready ? WithAlpha(tone, 0.55f + 0.45f * std::sin(RealTime() * 6)) : Palette::line, ready ? 2.2f : 1.4f);
    if (ready) {
        Draw::BlendAdd();
        Draw::GlowStretched({r.x + r.width / 2, r.y + r.height / 2}, 100, 22, 0, WithAlpha({255, 180, 80, 255}, 0.28f));
        Draw::BlendNormal();
    }
    const char* label = "集结完成";
    if (ready) label = "究极合体  [E]";
    else if (!assembled) label = TextFormat("集结 %d / %d", have, kHeroCount);
    else if (fusionCooldown > 0 && waveIndex >= 0) label = "冷却中";
    if (assembled && fusionCooldown > 0 && waveIndex >= 0 && outcome == Outcome::None) {
        float k = Clamp(fusionCooldown / kFusionMax, 0.0f, 1.0f);
        DrawRectangleRec({r.x + 2, r.y + 2, (r.width - 4) * k, r.height - 4}, {4, 6, 14, 150});
        Gui::Text(TextFormat("%d", (int)std::ceil(fusionCooldown)), r.x + r.width - 8, r.y + 12, 13, WHITE, Gui::Align::Right, true);
    }
    Gui::Text(label, r.x + r.width / 2, r.y + 12, 14, ready ? Palette::gold : Palette::muted, Gui::Align::Center, true);
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
        Draw::RoundedPanel(r, 10, Mix(tone, {10, 14, 30, 255}, hover ? 0.6f : 0.78f),
                           ready ? WithAlpha(tone, 0.6f + 0.4f * std::sin(RealTime() * 5)) : Palette::line, ready ? 2.2f : 1.5f);
        Vector2 c{r.x + r.width / 2, r.y + 16};
        if (i == 0) {
            Draw::BlendAdd();
            for (int k = 0; k < 5; ++k) {
                float a = -PI / 2 + (k - 2) * 0.36f;
                Draw::Line(c, Add(c, Mul(Dir(a), 22)), 3, WithAlpha(HeroInfo((HeroId)k).beam, 0.9f));
            }
            Draw::Glow(c, 16, WithAlpha(WHITE, 0.8f));
            Draw::BlendNormal();
            Draw::Star(c, 9, 4, RealTime(), WHITE);
        } else {
            Draw::Quad({c.x + 18, c.y}, {c.x - 14, c.y - 4}, {c.x - 16, c.y}, {c.x - 14, c.y + 4}, {220, 224, 232, 255});
            Draw::Tri({c.x + 2, c.y - 2}, {c.x - 10, c.y - 16}, {c.x - 14, c.y - 14}, {190, 196, 210, 255});
            Draw::Tri({c.x + 2, c.y + 2}, {c.x - 10, c.y + 16}, {c.x - 14, c.y + 14}, {170, 176, 192, 255});
            Draw::Line({c.x - 14, c.y}, {c.x + 16, c.y}, 1.6f, {236, 120, 40, 255});
        }
        Gui::Text(i == 0 ? "合体光线" : "空中支援", c.x, r.y + 32, 12, Palette::text, Gui::Align::Center, true);
        Draw::RoundedPanel({r.x + 4, r.y + 4, 16, 16}, 4, {6, 8, 20, 220}, BLANK, 0);
        Gui::Text(i == 0 ? "Q" : "W", r.x + 12, r.y + 5, 12, Palette::muted, Gui::Align::Center, true);
        if (!ready) {
            float k = waveIndex < 0 ? 1.0f : Clamp(cd / max, 0.0f, 1.0f);
            DrawRectangleRec({r.x + 2, r.y + 2 + (r.height - 4) * (1 - k), r.width - 4, (r.height - 4) * k}, {4, 6, 14, 170});
            if (waveIndex >= 0 && outcome == Outcome::None)
                Gui::TextOutline(TextFormat("%d", (int)std::ceil(cd)), c.x, c.y - 10, 20, WHITE, {0, 0, 0, 255}, 1.4f);
        }
        if (ready && !paused && Gui::Clicked(r)) {
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
        "在道路旁的草地上放置英雄，拐角处可覆盖更长的路径。",
        "点击已放置的英雄可以升级、出售或切换攻击目标。",
        "高斯的月神光波能减速怪兽，与泰罗的爆炸配合效果极佳。",
        "红王护甲很厚，迪迦的高伤害光线和赛文的破甲头镖是克星。",
        "提前召唤下一波可以获得额外金币奖励。",
        "杰顿会展开护盾，护盾期间伤害大幅降低，可等护盾消失再集火。",
        "巴尔坦星人被击倒后会分裂，记得在后方保留火力。",
    };
    int tip = ((int)(RealTime() / 6)) % 7;
    Gui::Paragraph(tips[tip], x, y + 24, w, 12.5f, Palette::muted, 1.5f);
    Gui::Text("空格 召唤  F 倍速  Q/W 必杀  右键 取消", x, area.y + area.height - 22, 10.5f, WithAlpha(Palette::muted, 0.8f));
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
    std::string status = boss->shieldTime > 0 ? "护盾展开！伤害 -85%" : boss->enraged ? "暴怒" : "";
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
    if (Gui::Button({bx, by + 174, bw, 46}, "返回关卡选择", Gui::Style::Red, true, 19)) GoToMenu(MenuPage::Levels);
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
        if (Gui::Button({r.x + 360, by, 150, 50}, "下一关", Gui::Style::Gold, true, 20)) GoToBattle(levelIndex + 1);
    } else {
        if (Gui::Button({r.x + 60, by, 200, 50}, "关卡选择", Gui::Style::Dark, true, 18)) GoToMenu(MenuPage::Levels);
        if (Gui::Button({r.x + 280, by, 200, 50}, win ? "再来一次" : "再试一次", Gui::Style::Gold, true, 20)) restartRequested = true;
    }
}

} // namespace UG
