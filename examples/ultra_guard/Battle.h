#pragma once

#include "Map.h"

namespace UG {

class BattleScene;
constexpr int kKaijuLayer = 1;

// Kaiju: walks its path, runs abilities, renders itself (y-sorted). Collides only as a
// query target; its CircleCollider sits on the body and never generates contacts.
class MonsterActor : public Component {
public:
    BattleScene* battle = nullptr;
    std::weak_ptr<GameObject> object; // for projectiles that home in on this kaiju
    EnemyId type = EnemyId::Gomora;
    int path = 0;
    float dist = 0;
    Vector2 dir{1, 0};
    float hp = 1, maxHp = 1, armor = 0, speedMul = 1;
    float slowTime = 0, slowAmount = 0, burnTime = 0, burnDps = 0;
    float breakTime = 0, vulnTime = 0, stunTime = 0, shieldTime = 0;
    float phaseTime = 0; // Tregear inside a dimension gate: untargetable and immune
    float buffTime = 0;  // driven berserk by Tregear: faster and tougher
    float abilityA = 0, abilityB = 0;
    float strikeTimer = 0, strikePause = 0; // attacks on heroes; ordinary kaiju halt briefly to swing
    float flash = 0, face = 1, anim = 0, appear = 0;
    float siegeReach = 0, siegeSide = 0, siegeTimer = 0; // siege maps: stop this far from the tower and batter it
    bool dead = false, enraged = false, sieging = false;

    const EnemyDef& Def() const { return EnemyInfo(type); }
    float Scale() const { return Def().size * 1.05f; }
    float Height() const;
    Vector2 Feet() const { return gameObject->transform->position; }
    Vector2 Center() const { return Add(Feet(), {0, -Height() * 0.45f}); }

    void FixedUpdate() override;
    void Render() override;
};

// Ultra hero standing on a pedestal: picks targets with physics queries and attacks.
class HeroActor : public Component {
public:
    BattleScene* battle = nullptr;
    int id = 0;
    HeroId hero = HeroId::Ultraman;
    int col = 0, row = 0, level = 0, mode = 0, invested = 0, shots = 0, kills = 0;
    float cooldown = 0.35f, recoil = 0, flash = 0, build = 0, dealt = 0;
    float nextCooldownScale = 1; // set by Fire for one shot, e.g. Trigger's rapid Sky Type
    float disabledTime = 0;
    // Metron / Gatanothor / energy drained by kaiju strikes
    enum class Disable { None, Confused, Petrified, Exhausted } disabled = Disable::None;
    float hp = 1, maxHp = 1, calm = 0, hurt = 0; // calm: seconds since the last strike, for regeneration
    const MonsterActor* focusTarget = nullptr;    // Teo: compared only, never dereferenced
    int focus = 0;
    float lunge = 0; // Leo's flying kick throws the figure toward lungeTo
    Vector2 lungeTo{};

    Vector2 Pos() const { return gameObject->transform->position; }
    float RenderDepth() const override { return Pos().y + 10; } // pedestal base
    void FixedUpdate() override;
    void Render() override;
};

// Sinking, fading kaiju; removed by a DestroyAfter on the same object.
class CorpseActor : public Component {
public:
    EnemyId type = EnemyId::Gomora;
    float face = 1, scale = 1, t = 0;
    void Update() override { t += GameTime::DeltaTime(); }
    void Render() override;
};

struct Airstrike {
    Vector2 target;
    float time = 0;
    int dropped = 0;
};

struct ActiveSpawn {
    SpawnGroup group;
    int spawned = 0;
    float timer = 0;
};

// Nexus's Meta Field: a lingering dome that burns and slows whatever stands inside.
struct Zone {
    Vector2 at;
    float radius = 0, dps = 0, slow = 0, time = 0, life = 0;
    int tower = -1;
    bool vuln = false;
};

class BattleScene : public Scene {
public:
    explicit BattleScene(int levelIndex);
    explicit BattleScene(HeroId demoHero); // skill preview on the database page and in --balance
    void OnLoad() override;
    void OnUnload() override;
    void Update() override;
    void FixedUpdate() override;
    void Render() override;

    // Development aids: a simple bot that buys and upgrades heroes.
    struct BotResult {
        bool won;
        int lives, wave, towers, gold, kills, firstLeakWave;
        std::array<int, kEnemyCount> leaks;
        float minutes;
        int exhausted; // heroes drained by kaiju strikes
    };
    void DebugAutoplay(int startWave = 0);
    void DebugCast();
    void DebugFinish();
    void DebugInspect(bool pause);
    // casual: no skills, slower decisions and one purchase at a time.
    BotResult RunBot(bool casual = false);
    // Demo scenes only: damage per second the hero deals to the passing parade.
    float BenchmarkDemo(int star, float seconds);
    bool headless = false;
    // Skill preview on the database page: the menu drives it with a SceneClock and draws only the
    // attack effects, so the scene never touches music, game speed or input.
    bool embedded = false;
    void DrawEmbedded(Rectangle clip, Vector2 heroAt);
    void ReleaseEmbedded();

private:
    friend class MonsterActor;
    friend class HeroActor;
    enum class Outcome { None, Victory, Defeat };
    enum class Fx { Spark, Glow, Smoke, Ember, Count };

    int levelIndex;
    bool demo = false;
    HeroId demoHero = HeroId::Ultraman;
    int demoStar = 2;
    float demoSpawn = 0, demoClock = 0, demoDealt = 0;
    int demoCount = 0;
    const LevelDef* level = nullptr;
    LevelDef endless;           // survival copy whose waves grow as they are needed; also the demo level
    std::vector<HeroId> roster; // the six battle cards, chosen on the preparation page
    float surviveTime = 0;
    bool newRecord = false;
    MapData map;
    RenderTexture2D mapTexture{};
    std::array<std::array<int, kCols>, kRows> towerGrid{};

    std::vector<Ref<MonsterActor>> monsters;
    std::vector<Ref<HeroActor>> heroes;
    std::array<Ref<ParticleEmitter>, (int)Fx::Count> fx;
    Ref<EffectLayer> effects;
    std::vector<Airstrike> strikes;
    std::vector<ActiveSpawn> spawning;
    std::vector<Zone> zones;

    int gold = 0, lives = 0, kills = 0;
    float citadel = 0, citadelMax = 0, citadelShot = 0, citadelPulse = 0; // siege: the central tower
    int waveIndex = -1;
    int pathToggle = 0;
    float nextWaveTimer = -1;
    int nextTowerId = 1;
    float simTime = 0;
    int speedIndex = 0;
    bool paused = false;
    Outcome outcome = Outcome::None;
    float outcomeTime = 0;
    int earnedStars = 0;

    int placing = -1; // hero index being placed
    int hoverCard = -1;
    int selected = -1; // tower id
    bool targetingStrike = false;
    bool restartRequested = false;
    float brothersCooldown = 0, strikeCooldown = 0, fusionCooldown = 0;
    float fusionSpan = 1; // length of the current charge, for the progress bar
    float fusionAnim = -1; // real seconds into the fusion cut-in; < 0 when idle
    float flashScreen = 0, shake = 0, baseFlash = 0;
    float fogTime = 0; // Gatanothor's dark fog: heroes reload at half speed
    float goldPulse = 0, livesPulse = 0;

    std::string bannerTitle, bannerSub;
    Color bannerColor{};
    float bannerTime = 0;
    bool bannerWarning = false;

    int botPurchases = 0;
    bool botCasual = false;
    std::vector<GridPoint> botTiles;
    std::array<int, kEnemyCount> leakCount{};
    int leakWave = -1;
    int exhaustions = 0;

    // simulation
    void StepEffects(float dt);
    Ref<Projectile> Launch(ProjectileMotion motion, Vector2 from);
    std::weak_ptr<GameObject> Follow(MonsterActor& target) const;
    void StartNextWave(bool early);
    void SpawnEnemy(EnemyId type, int path, float dist);
    void DamageEnemy(MonsterActor& e, float amount, int towerId, bool ignoreArmor = false);
    void KillEnemy(MonsterActor& e, int towerId);
    void Leak(MonsterActor& e);
    bool Siege() const { return level->siege > 0; }
    void SiegeHit(MonsterActor& e);
    void StepSiege(float dt);
    void StepZones(float dt);
    void StepDemo(float dt);
    void SetDemoStar(int star);
    void Knock(MonsterActor& e, float tiles);
    MonsterActor* NearestKaiju(Vector2 at, float radius, const std::vector<const MonsterActor*>& skip) const;
    void Fire(HeroActor& t, MonsterActor& target);
    void FireSignature(HeroActor& t, MonsterActor& target, float dmg, float range, Vector2 origin, Vector2 aim);
    void PierceBeam(HeroActor& t, Vector2 origin, Vector2 toward, float damage, float range, Color glow, bool burn);
    void Explode(Vector2 at, float radius, float damage, int towerId, bool burn, bool stun, Color hue);
    void CastBrothers();
    void CastStrike(Vector2 target);
    void CastFusion(bool force = false);
    void UpdateFusion(float real);
    void FusionImpact();
    bool Cinematic() const;
    void DrawFusionCinematic();
    bool RosterComplete() const;
    int RosterCount() const;
    std::vector<MonsterActor*> KaijuInCircle(Vector2 center, float radius) const;
    std::vector<Ref<MonsterActor>> Alive() const;
    HeroActor* FindTower(int id);
    HeroActor* NearestHero(Vector2 at, float radius, bool skipDisabled = true);
    void DisableHero(HeroActor& hero, HeroActor::Disable kind, float seconds, Vector2 from);
    float Assault() const; // kaiju strike multiplier for the current wave; 0 = they never attack
    void KaijuAttack(MonsterActor& e, HeroActor& target);
    void DamageHero(HeroActor& hero, float amount);
    bool Endless() const { return level->endless; }
    void EnsureWaves(int count);
    void Banner(const std::string& title, const std::string& sub, Color color, bool warning = false);
    void SetOutcome(Outcome value);
    void Restart();
    bool CanStartWave() const;
    bool Running() const { return outcome == Outcome::None; }

    // input
    void HandleInput();
    void PlaceTower(int col, int row);
    bool BuyTower(HeroId hero, int col, int row);
    void UpgradeTower(HeroActor& t);
    void SellTower(int id);
    int SellValue(const HeroActor& t) const;
    void SetPaused(bool value);
    void CycleSpeed();
    void ApplySpeed() const;
    void BotAct();
    std::vector<GridPoint> RankedTiles() const;

    // effects
    void CreateEmitters();
    void Emit(Fx kind, Vector2 at, Color color, int count, float speed, float size, float life);
    void Burst(Vector2 at, Color color, int count, float speed, float size, float life);
    void Smoke(Vector2 at, int count, float size);
    void Sparks(Vector2 at, Color color, int count, float speed);
    void Text(Vector2 at, const std::string& text, Color color, float size = 16);
    void AddBeam(Vector2 a, Vector2 b, float width, float life, Color glow, Color core);
    void AddRing(Vector2 c, float r0, float r1, float life, float width, Color color);
    void Bolt(Vector2 a, Vector2 b, Color color);

    // drawing
    void DrawWorld();
    void DrawZones();
    void DrawSiege();
    void DrawHealthBar(const MonsterActor& e);
    void DrawPlacement();
    void DrawHud();
    void DrawPanel();
    void DrawHeroCard(int index, Rectangle r);
    void DrawSkills();
    void DrawFusion();
    void DrawInfo(Rectangle area);
    void DrawBanner();
    void DrawBossBar();
    void DrawEnemyTooltip();
    void DrawPauseMenu();
    void DrawResult();
    void DrawIntroHint();
};

} // namespace UG
