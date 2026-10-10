#include "World.h"
#include <raygame/FileSystem.h>
#include <sstream>
#include <stdexcept>

namespace Forge {
Rectangle Platform::At(float t) const {
    float factor = (1 - std::cos((t / period + phase) * 2 * PI)) * 0.5f;
    return {rect.x + travel.x * factor, rect.y + travel.y * factor, rect.width, rect.height};
}
bool Laser::Active(float t) const { return std::fmod(t + phase, period) < period * 0.54f; }
Vector2 Drone::At(float t) const { return {origin.x + std::sin(t * 2 * PI / period) * range, origin.y + std::sin(t * 3) * 8}; }

Level Level::Load(const std::string& file, int number) {
    std::string text;
    if (!RayGame::FileSystem::ReadText(file, text, 128 * 1024)) throw std::runtime_error("Cannot read level: " + file);
    Level level; level.index = number; level.version = 0;
    std::istringstream lines(text); std::string line; int lineNumber = 0;
    while (std::getline(lines, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == ';') continue;
        std::istringstream fields(line); std::string key; fields >> key;
        if (key == "version") fields >> level.version;
        else if (key == "name" || key == "subtitle") {
            std::string value; std::getline(fields >> std::ws, value);
            (key == "name" ? level.name : level.subtitle) = value;
        } else if (key == "size") fields >> level.size.x >> level.size.y;
        else if (key == "spawn") fields >> level.spawn.x >> level.spawn.y;
        else if (key == "goal") fields >> level.goal.x >> level.goal.y;
        else if (key == "solid" || key == "ledge" || key == "moving") {
            Platform p; fields >> p.rect.x >> p.rect.y >> p.rect.width >> p.rect.height;
            p.oneWay = key != "solid";
            if (key == "moving") fields >> p.travel.x >> p.travel.y >> p.period >> p.phase;
            level.platforms.push_back(p);
        } else if (key == "hazard") {
            Rectangle h; fields >> h.x >> h.y >> h.width >> h.height; level.hazards.push_back(h);
        } else if (key == "cell" || key == "checkpoint") {
            Vector2 p; fields >> p.x >> p.y;
            (key == "cell" ? level.cells : level.checkpoints).push_back(p);
        } else if (key == "laser") {
            Laser l; fields >> l.rect.x >> l.rect.y >> l.rect.width >> l.rect.height >> l.period >> l.phase;
            level.lasers.push_back(l);
        } else if (key == "drone") {
            Drone d; fields >> d.origin.x >> d.origin.y >> d.range >> d.period; level.drones.push_back(d);
        } else if (key == "sign") {
            Sign s; fields >> s.position.x >> s.position.y; std::getline(fields >> std::ws, s.text); level.signs.push_back(s);
        } else throw std::runtime_error("Unknown level directive at line " + std::to_string(lineNumber));
        if (fields.fail()) throw std::runtime_error("Invalid level field at line " + std::to_string(lineNumber));
        std::string extra;
        if (key != "name" && key != "subtitle" && key != "sign" && fields >> extra)
            throw std::runtime_error("Extra level field at line " + std::to_string(lineNumber));
        if (level.platforms.size() + level.cells.size() + level.checkpoints.size() + level.hazards.size()
            + level.lasers.size() + level.drones.size() + level.signs.size() > 1024)
            throw std::runtime_error("Level exceeds the content limit");
    }
    level.Validate(); return level;
}
void Level::Validate() const {
    auto point = [&](Vector2 p) { return std::isfinite(p.x) && std::isfinite(p.y) && p.x >= 0 && p.y >= 0 && p.x <= size.x && p.y <= size.y; };
    auto rect = [&](Rectangle r) { return point({r.x,r.y}) && std::isfinite(r.width) && std::isfinite(r.height)
        && r.width > 0 && r.height > 0 && r.x + r.width <= size.x && r.y + r.height <= size.y; };
    if (version != 1 || name.empty() || name.size() > 80 || subtitle.size() > 160 || !std::isfinite(size.x)
        || !std::isfinite(size.y) || size.x < 1280 || size.x > 32768 || size.y < 720 || size.y > 4096
        || !point(spawn) || !point(goal) || cells.empty() || cells.size() > 32 || checkpoints.size() > 32)
        throw std::runtime_error("Invalid level metadata");
    for (auto p : cells) if (!point(p)) throw std::runtime_error("Invalid cell");
    for (auto p : checkpoints) if (!point(p)) throw std::runtime_error("Invalid checkpoint");
    for (const auto& s : signs) if (!point(s.position) || s.text.empty() || s.text.size()>120)
        throw std::runtime_error("Invalid sign");
    for (auto h : hazards) if (!rect(h)) throw std::runtime_error("Invalid hazard");
    for (auto p : platforms) if (!rect(p.rect) || !std::isfinite(p.period) || p.period <= 0 || !std::isfinite(p.phase)
        || !std::isfinite(p.travel.x) || !std::isfinite(p.travel.y)
        || !rect({p.rect.x + p.travel.x, p.rect.y + p.travel.y,p.rect.width,p.rect.height})) throw std::runtime_error("Invalid platform");
    for (auto l : lasers) if (!rect(l.rect) || !std::isfinite(l.period) || l.period <= 0 || !std::isfinite(l.phase) || l.phase < 0)
        throw std::runtime_error("Invalid laser");
    for (auto d : drones) if (!point(d.origin) || !std::isfinite(d.period) || d.period <= 0 || !std::isfinite(d.range)
        || d.range < 0 || !point({d.origin.x-d.range,d.origin.y}) || !point({d.origin.x+d.range,d.origin.y})) throw std::runtime_error("Invalid drone");
}
World::World(Level value) : level(std::move(value)) { Respawn(); }
void World::Restore(int index, std::uint64_t mask) {
    checkpoint = index >= -1 && index < (int)level.checkpoints.size() ? index : -1;
    collected = mask & ((std::uint64_t{1} << level.cells.size()) - 1);
    Respawn();
}
void World::Respawn() {
    player = {}; player.position = checkpoint >= 0 ? level.checkpoints[checkpoint] : level.spawn;
    deathTimer = 0; player.dashCooldown = 0; player.airJumps = 1;
}
void World::Kill() {
    if (deathTimer > 0 || complete) return;
    deathTimer = 0.65f; ++deaths; events |= Death;
}
int World::CellCount() const { int count = 0; for (size_t i=0;i<level.cells.size();++i) if (collected & (std::uint64_t{1}<<i)) ++count; return count; }
void World::MoveAxis(float amount, bool vertical) {
    int segments = std::max(1, (int)std::ceil(std::fabs(amount) / 8));
    float delta = amount / segments;
    for (int step=0;step<segments;++step) {
        auto before = player.Bounds();
        if (vertical) player.position.y += delta; else player.position.x += delta;
        auto bounds = player.Bounds();
        for (size_t i=0;i<level.platforms.size();++i) {
            auto& platform = level.platforms[i]; auto r = platform.At(time);
            if (platform.oneWay && (!vertical || delta < 0 || player.dropTime > 0 || before.y+before.height > r.y+1)) continue;
            if (!CheckCollisionRecs(bounds, r)) continue;
            if (vertical) {
                if (delta >= 0) { player.position.y = r.y; player.grounded = true; player.support = (int)i; }
                else player.position.y = r.y + r.height + 44;
                player.velocity.y = 0;
            } else {
                if (delta > 0) { player.position.x = r.x - 14; player.wall = 1; }
                else if (delta < 0) { player.position.x = r.x + r.width + 14; player.wall = -1; }
                player.velocity.x = 0; player.dashTime = 0;
            }
            bounds = player.Bounds();
        }
        for (auto r : level.hazards) if (CheckCollisionRecs(bounds,r)) Kill();
        for (auto l : level.lasers) if (l.Active(time) && CheckCollisionRecs(bounds,l.rect)) Kill();
    }
}
void World::Step(Command input, float dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 0.05f) return;
    if (complete) { finishTimer += dt; return; }
    time += dt; elapsed += dt;
    if (deathTimer > 0) { deathTimer -= dt; if (deathTimer <= 0) Respawn(); return; }
    bool wasGrounded = player.grounded;
    if (player.support >= 0 && player.grounded) {
        auto before = level.platforms[player.support].At(time-dt), after = level.platforms[player.support].At(time);
        player.position.x += after.x-before.x; player.position.y += after.y-before.y;
    }
    player.coyote = wasGrounded ? 0.11f : std::max(0.0f,player.coyote-dt);
    player.jumpBuffer = input.jump ? 0.13f : std::max(0.0f,player.jumpBuffer-dt);
    player.dashCooldown = std::max(0.0f,player.dashCooldown-dt);
    player.dropTime = std::max(0.0f,player.dropTime-dt);
    if (input.move != 0) player.facing = input.move > 0 ? 1 : -1;
    if (input.down && input.jump && player.support >= 0 && level.platforms[player.support].oneWay) {
        player.dropTime = 0.22f; player.jumpBuffer = 0; player.position.y += 3;
    }
    if (input.dash && player.dashCooldown <= 0) {
        player.dashTime = 0.16f; player.dashCooldown = 0.65f; events |= Dash;
        player.velocity = {player.facing * 880.0f,0};
    }
    if (player.dashTime > 0) { player.dashTime = std::max(0.0f,player.dashTime-dt); player.velocity.y = 0; }
    else {
        float target = Clamp(input.move,-1.0f,1.0f) * 360;
        float acceleration = wasGrounded ? 3000.0f : 1900.0f;
        player.velocity.x += Clamp(target-player.velocity.x,-acceleration*dt,acceleration*dt);
        if (player.jumpBuffer > 0 && (player.coyote > 0 || player.wall != 0 || player.airJumps > 0)) {
            if (player.coyote <= 0 && player.wall == 0) --player.airJumps;
            if (player.wall != 0 && !wasGrounded) player.velocity.x = -player.wall * 430.0f;
            player.velocity.y = -730; player.jumpBuffer = player.coyote = 0;
            player.support = -1; player.grounded = false; events |= Jump;
        }
        if (!input.jumpHeld && player.velocity.y < -260) player.velocity.y = -260;
        float gravity = (input.jumpHeld && std::fabs(player.velocity.y) < 90) ? 1000.0f : 1850.0f;
        player.velocity.y = std::min(900.0f,player.velocity.y+gravity*dt);
        if (player.wall != 0 && input.move == player.wall && player.velocity.y > 120) player.velocity.y = 120;
    }
    player.grounded = false; player.support = -1; player.wall = 0;
    MoveAxis(player.velocity.x*dt,false);
    MoveAxis(player.velocity.y*dt,true);
    if (player.grounded) {
        player.airJumps = 1;
        if (!wasGrounded) events |= Land;
    }
    player.position.x = Clamp(player.position.x,14.0f,level.size.x-14);
    if (player.position.y > level.size.y+100) Kill();
    ScanInteractions();
}
void World::ScanInteractions() {
    if (deathTimer > 0) return;
    auto center = Vector2{player.position.x,player.position.y-22};
    for (size_t i=0;i<level.cells.size();++i) if (!(collected & (std::uint64_t{1}<<i)) && Vector2Distance(center,level.cells[i]) < 40) {
        collected |= std::uint64_t{1}<<i; events |= Cell;
    }
    for (size_t i=0;i<level.checkpoints.size();++i) if ((int)i > checkpoint && Vector2Distance(player.position,level.checkpoints[i]) < 52) {
        checkpoint = (int)i; events |= Checkpoint;
    }
    for (auto drone : level.drones) if (CheckCollisionCircleRec(drone.At(time),22,player.Bounds()) && player.dashTime <= 0) Kill();
    if (deathTimer > 0) return;
    if (Vector2Distance(player.position,level.goal) < 62 && CellCount() == (int)level.cells.size()) {
        complete = true; events |= Finish;
    }
}
}
