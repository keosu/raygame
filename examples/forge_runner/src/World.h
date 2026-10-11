#pragma once
#include <raygame/Core.h>
#include <cstdint>

namespace Forge {
enum class Surface { Steel, Ice, Conveyor, Fragile };
struct Platform {
    Rectangle rect{};
    Vector2 travel{};
    float period = 4, phase = 0;
    bool oneWay = false;
    Surface surface = Surface::Steel;
    float grip = 1, beltSpeed = 0, breakDelay = 0.7f, rebuildDelay = 2.5f;
    Rectangle At(float time) const;
};
struct Laser {
    Rectangle rect{};
    float period = 3, phase = 0;
    bool Active(float time) const;
};
struct Drone {
    Vector2 origin{};
    float range = 100, period = 4;
    Vector2 At(float time) const;
};
struct Crawler {
    Vector2 origin{}; // Centre of the chassis; patrol endpoints must be on a floor.
    float range = 100, period = 4;
    Vector2 At(float time) const;
};
struct Airflow {
    Rectangle rect{};
    Vector2 force{};
};
struct Sentry {
    Vector2 origin{}; // Muzzle; one horizontal bolt per cycle after a visible charge.
    int direction = -1;
    float range = 420, period = 3.6f, phase = 0;
    static constexpr float chargeTime = 0.75f, boltSpeed = 360;
    float Cycle(float time) const;
};
struct Sign {
    Vector2 position{};
    std::string text;
};
struct Level {
    int version = 1, index = 0;
    std::string name, subtitle;
    Vector2 size{6400, 960}, spawn{160, 720}, goal{};
    std::vector<Platform> platforms;
    std::vector<Rectangle> hazards;
    std::vector<Vector2> cells, checkpoints;
    std::vector<Laser> lasers;
    std::vector<Drone> drones;
    std::vector<Crawler> crawlers;
    std::vector<Airflow> airflows;
    std::vector<Sentry> sentries;
    std::vector<Sign> signs;
    static Level Load(const std::string& file, int index);
    void Validate() const;
};
struct Command {
    float move = 0;
    bool jump = false, jumpHeld = false, dash = false, down = false;
};
enum Event : unsigned {
    Jump = 1,
    Dash = 2,
    Land = 4,
    Cell = 8,
    Checkpoint = 16,
    Death = 32,
    Finish = 64
};
struct Runner {
    Vector2 position{}, velocity{};
    bool grounded = false;
    int facing = 1, wall = 0, support = -1;
    float coyote = 0, jumpBuffer = 0, dashTime = 0, dashCooldown = 0, dropTime = 0;
    int airJumps = 1;
    Rectangle Bounds() const {
        return {position.x - 14, position.y - 44, 28, 44};
    }
};
class World {
public:
    Level level;
    Runner player;
    float time = 0, elapsed = 0, deathTimer = 0, finishTimer = 0;
    int checkpoint = -1, deaths = 0;
    std::uint64_t collected = 0;
    unsigned events = 0;
    bool complete = false;
    // -1: intact, then seconds since the first landing; reset on respawn.
    std::vector<float> platformTimers;
    explicit World(Level value);
    void Step(Command input, float dt);
    void Respawn();
    void Restore(int checkpointIndex, std::uint64_t mask);
    void Kill();
    int CellCount() const;
    bool PlatformActive(size_t index) const;
    float SentryRange(const Sentry& sentry) const;
    bool SentryBolt(const Sentry& sentry, Vector2& position) const;
    unsigned TakeEvents() {
        auto result = events;
        events = 0;
        return result;
    }

private:
    void MoveAxis(float amount, bool vertical);
    void ScanInteractions();
    void ScanThreats(Rectangle before, float previousTime);
};
} // namespace Forge
