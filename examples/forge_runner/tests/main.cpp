#include "../src/World.h"
#include "../src/Profile.h"
#include <cstdio>
#include <fstream>
#include <unordered_set>

using namespace Forge;
int failures = 0, passes = 0;
void Expect(bool condition, const char* name) {
    if (condition)
        ++passes;
    else {
        ++failures;
        std::printf("FAIL: %s\n", name);
    }
}
#include "touch.h"
Level Flat() {
    Level l;
    l.name = "test";
    l.size = {3000, 960};
    l.spawn = {100, 720};
    l.goal = {2800, 720};
    l.cells = {{2600, 650}};
    l.platforms.push_back({{0, 720, 3000, 240}});
    return l;
}
void Step(World& w, int frames, Command c = {}) {
    for (int i = 0; i < frames; ++i)
        w.Step(c, 1.0f / 60);
}
#include "mechanics.h"
// Find a route using the same command/physics contract as a human player.
// No teleports, injected collection bits or modified collision geometry.
bool Reach(World& world, Vector2 target, int cell) {
    struct Node {
        World value;
        std::vector<Command> path;
        float score;
    };
    auto reached = [&](const World& w) {
        return cell >= 0 ? (w.collected & (std::uint64_t{1} << cell)) != 0 : w.complete;
    };
    const Command actions[] = {{1, false, true}, {1, true, true},        {1, false, true, true},
                               {0, false, true}, {0, true, true},        {-1, false, true},
                               {-1, true, true}, {-1, false, true, true}};
    for (int round = 0; round < 65 && !reached(world); ++round) {
        std::vector<Node> beam{{world, {}, 0}}, best;
        bool found = false;
        Node winner{world, {}, 0};
        for (int depth = 0; depth < 12 && !found; ++depth) {
            std::vector<Node> next;
            std::unordered_set<std::string> seen;
            for (const auto& node : beam)
                for (auto command : actions) {
                    Node child = node;
                    child.path.push_back(command);
                    for (int tick = 0; tick < 8; ++tick) {
                        child.value.Step(command, 1.0f / 60);
                        command.jump = command.dash = false;
                        if (reached(child.value)) {
                            winner = std::move(child);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                    if (child.value.deaths > world.deaths)
                        continue;
                    auto p = child.value.player;
                    child.score = std::fabs(p.position.x - target.x) +
                                  std::fabs(p.position.y - 22 - target.y) * 1.5f +
                                  (p.position.y > 800 ? 300.0f : 0.0f);
                    std::string key = std::to_string((int)p.position.x / 18) + ":" +
                                      std::to_string((int)p.position.y / 18) + ":" +
                                      std::to_string((int)p.velocity.x / 90) + ":" +
                                      std::to_string((int)p.velocity.y / 140) + ":" +
                                      std::to_string(p.airJumps) + ":" +
                                      std::to_string((int)(p.dashCooldown * 4));
                    if (seen.insert(key).second)
                        next.push_back(std::move(child));
                }
            if (found)
                break;
            if (next.empty())
                break;
            std::sort(next.begin(), next.end(),
                      [](const Node& a, const Node& b) { return a.score < b.score; });
            if (next.size() > 100)
                next.erase(next.begin() + 100, next.end());
            beam = std::move(next);
            best = beam;
        }
        if (found) {
            world = std::move(winner.value);
            return true;
        }
        if (best.empty())
            return false;
        auto command = best.front().path.front();
        for (int tick = 0; tick < 8; ++tick) {
            world.Step(command, 1.0f / 60);
            command.jump = command.dash = false;
        }
    }
    return reached(world);
}
int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::printf("Usage: forge_tests <assets> <isolated-data>\n");
            return 2;
        }
        TestTouchControls();
        TestMechanics(std::filesystem::path(argv[2]) / "mechanics");
        TestLevelText(argv[1], std::filesystem::path(argv[2]) / "level-text");
        TextCatalog catalog;
        catalog.Load(std::string(argv[1]) + "/locales/zh-CN.tsv");
        Expect(catalog.Translate("CONTINUE", Language::Chinese) == "继续旅程" &&
                   catalog.Translate("CONTINUE", Language::English) == "CONTINUE",
               "language catalog resolves both languages");
        Expect(catalog.Translate("custom level sign", Language::Chinese) == "custom level sign",
               "custom untranslated text falls back to its source");
        auto glyphs = catalog.ChineseCodepoints();
        Expect(std::find(glyphs.begin(), glyphs.end(), 0x7ee7) != glyphs.end() &&
                   std::find(glyphs.begin(), glyphs.end(), 'A') != glyphs.end(),
               "glyph collection includes Chinese and ASCII");
        for (int i = 0; i < 3; ++i) {
            const char* names[] = {"01_foundry.level", "02_conduit.level", "03_core.level"};
            auto level = Level::Load(std::string(argv[1]) + "/levels/" + names[i], i);
            Expect(level.version == 2 && level.cells.size() >= 8 && level.checkpoints.size() >= 3,
                   "campaign content is complete");
            bool translated = true;
            for (const auto& sign : level.signs)
                translated &= catalog.Translate(sign.text, Language::Chinese) != sign.text;
            Expect(translated, "campaign mechanic signs all have Chinese translations");
            bool safeCheckpoints = true;
            for (size_t cp = 0; cp < level.checkpoints.size(); ++cp) {
                World restored(level);
                restored.Restore((int)cp, 0);
                Step(restored, 300);
                safeCheckpoints &= restored.deaths == 0;
            }
            Expect(safeCheckpoints, "campaign checkpoints stay safe through enemy firing cycles");
            World w(level);
            Step(w, 60);
            Expect(w.player.grounded && w.deaths == 0, "campaign spawn is safe");
            w.player.position = level.goal;
            w.Step({}, 1.0f / 60);
            Expect(!w.complete, "exit rejects missing power cells");
            w.collected = (std::uint64_t{1} << level.cells.size()) - 1;
            w.player.position = level.goal;
            w.Step({}, 1.0f / 60);
            Expect(w.complete, "all campaign sectors can finish");
            World route(level);
            Step(route, 3);
            bool navigable = true;
            for (size_t cell = 0; cell < level.cells.size(); ++cell) {
                if (!Reach(route, level.cells[cell], (int)cell)) {
                    std::printf("Route blocked: sector %d, cell %zu, at %.0f %.0f\n", i + 1,
                                cell + 1, route.player.position.x, route.player.position.y);
                    std::printf("t=%.2f velocity=(%.0f, %.0f) ground=%d air=%d dash=%.2f/%.2f\n",
                                route.time, route.player.velocity.x, route.player.velocity.y,
                                route.player.grounded, route.player.airJumps, route.player.dashTime,
                                route.player.dashCooldown);
                    navigable = false;
                    break;
                }
            }
            if (navigable)
                navigable = Reach(route, {level.goal.x, level.goal.y - 22}, -1);
            Expect(navigable, "campaign can be completed through normal controls");
        }
        World w(Flat());
        Step(w, 3);
        Expect(w.player.grounded, "runner lands on a solid floor");
        w.Step({1, true, true}, 1.0f / 60);
        Expect(w.player.velocity.y < -600 && !w.player.grounded, "jump has immediate response");
        float fullY = w.player.position.y;
        Step(w, 16, {0, false, true});
        float fullHeight = fullY - w.player.position.y;
        World shortJump(Flat());
        Step(shortJump, 3);
        shortJump.Step({0, true, true}, 1.0f / 60);
        Step(shortJump, 16);
        Expect(fullHeight > 720 - shortJump.player.position.y + 30, "holding jump changes height");
        w.Step({0, true, true}, 1.0f / 60);
        Expect(w.player.airJumps == 0 && w.player.velocity.y < -600, "one air jump is available");
        auto vy = w.player.velocity.y;
        w.Step({0, true, true}, 1.0f / 60);
        Expect(w.player.velocity.y > vy, "third air jump is rejected");
        World dash(Flat());
        Step(dash, 3);
        dash.Step({1, false, false, true}, 1.0f / 60);
        Expect(dash.player.velocity.x > 800 && dash.player.dashCooldown > 0,
               "dash accelerates immediately");
        Step(dash, 12, {1, false, false, true});
        Expect(dash.player.dashCooldown > 0 && dash.player.dashTime == 0,
               "dash cooldown prevents repetition");
        Level wallLevel = Flat();
        wallLevel.platforms.push_back({{300, 400, 40, 320}});
        World wall(wallLevel);
        Step(wall, 100, {1});
        Expect(wall.player.position.x <= 286.01f, "dash and running cannot cross solid walls");
        Level edge = Flat();
        edge.platforms[0].rect.width = 200;
        World coyote(edge);
        Step(coyote, 3);
        coyote.player.position.x = 201;
        coyote.Step({1, true, true}, 1.0f / 60);
        Expect(coyote.player.velocity.y < -600, "coyote grace allows edge jumps");
        Level oneWay = Flat();
        oneWay.platforms.push_back({{300, 600, 200, 20}, {}, 4, 0, true});
        World ledge(oneWay);
        ledge.player.position = {400, 550};
        ledge.player.velocity.y = 200;
        Step(ledge, 20);
        Expect(ledge.player.grounded && std::fabs(ledge.player.position.y - 600) < 0.1f,
               "one-way ledge catches falling runner");
        ledge.Step({0, true, false, false, true}, 1.0f / 60);
        Step(ledge, 35);
        Expect(std::fabs(ledge.player.position.y - 720) < 0.1f,
               "down and jump drops through one-way ledge");
        Level moving = Flat();
        moving.platforms.push_back({{300, 600, 200, 20}, {160, 0}, 4, 0, true});
        World lift(moving);
        lift.player.position = {400, 600};
        Step(lift, 40);
        Expect(lift.player.position.x > 430 && lift.player.grounded,
               "moving platform carries runner");
        Level danger = Flat();
        danger.hazards.push_back({250, 660, 4, 60});
        World hazard(danger);
        Step(hazard, 3);
        Step(hazard, 12, {1, false, false, true});
        Expect(hazard.deaths > 0, "substeps detect thin hazard during dash");
        Step(hazard, 50);
        Expect(hazard.player.position.x < 250 && hazard.deathTimer == 0,
               "death respawns at safe checkpoint");
        Level guardedExit = Flat();
        guardedExit.drones.push_back({guardedExit.goal, 0, 4});
        World guarded(guardedExit);
        guarded.collected = 1;
        guarded.player.position = guarded.level.goal;
        guarded.Step({}, 1.0f / 60);
        Expect(guarded.deaths == 1 && !guarded.complete,
               "dying at the exit cannot also complete a sector");
        Level checkpointLevel = Flat();
        checkpointLevel.checkpoints = {{600, 720}};
        World checkpoint(checkpointLevel);
        checkpoint.player.position = {600, 720};
        Step(checkpoint, 1);
        Expect(checkpoint.checkpoint == 0, "checkpoint activates");
        checkpoint.Kill();
        Step(checkpoint, 45);
        Expect(std::fabs(checkpoint.player.position.x - 600) < 0.1f, "checkpoint controls respawn");
        checkpoint.Restore(400, ~std::uint64_t{0});
        Expect(checkpoint.checkpoint == -1 && checkpoint.collected == 1,
               "invalid resume fields are bounded");
        float oldTime = checkpoint.time;
        checkpoint.Step({}, std::numeric_limits<float>::infinity());
        Expect(checkpoint.time == oldTime, "invalid simulation delta rejected");
        Profile profile(argv[2]);
        std::filesystem::create_directories(profile.directory);
        profile.settings.music = 0.3f;
        profile.settings.fps = 144;
        profile.settings.controlSize = 1.4f;
        profile.settings.showDown = true;
        Expect(profile.SaveSettings() && profile.NewRun(), "new run and settings persist");
        profile.Record(checkpoint);
        Profile restored(argv[2]);
        restored.Load();
        Expect(restored.active && restored.collected == 1 && restored.settings.fps == 144,
               "resume and settings round trip");
        Expect(restored.settings.controlSize == 1.4f && restored.settings.showDown,
               "touch size and optional down button persist");
        profile.settings.language = Language::English;
        profile.SaveSettings();
        restored.Load();
        Expect(restored.settings.language == Language::English && restored.collected == 1,
               "English preference reloads without changing progress");
        profile.settings.language = Language::Chinese;
        profile.SaveSettings();
        restored.Load();
        Expect(restored.settings.language == Language::Chinese, "Chinese preference reloads");
        RayGame::SaveSystem legacySettings;
        legacySettings.SetFloat("music", 0.2f);
        legacySettings.SetString("language", "invalid");
        legacySettings.Save(profile.directory / "settings.sav");
        restored.Load();
        Expect(restored.settings.language == Language::Chinese && restored.settings.music == 0.2f,
               "unknown language defaults safely without discarding settings");
        legacySettings.Remove("language");
        legacySettings.Save(profile.directory / "settings.sav");
        restored.Load();
        Expect(restored.settings.language == Language::Chinese, "older saves default to Chinese");
        Expect(restored.settings.controlSize == 1.2f && !restored.settings.showDown,
               "older saves use larger controls and hide down button");
        checkpoint.collected = 1;
        checkpoint.player.position = checkpoint.level.goal;
        checkpoint.Step({}, 1.0f / 60);
        profile.Record(checkpoint);
        Expect(profile.unlocked == 1 && profile.best[0] > 0, "completion stores sector record");
        Expect(profile.level == 1 && profile.collected == 0 && profile.checkpoint == -1,
               "saved completion continues into the next sector");
        profile.SaveProgress();
        {
            std::ofstream broken(profile.directory / "campaign.sav");
            broken << "damaged";
        }
        Profile recovered(argv[2]);
        recovered.Load();
        Expect(recovered.recovered && recovered.active, "damaged campaign recovers valid backup");
        checkpoint.level.index = 2;
        profile.Record(checkpoint);
        Profile finished(argv[2]);
        finished.Load();
        Expect(finished.completed && !finished.active && finished.level == 2,
               "final completion is persistent and has no active run");
        Profile failedSave(profile.directory / "blocked");
        {
            std::ofstream blocker(failedSave.directory);
            blocker << "file";
        }
        Expect(!failedSave.NewRun() && !failedSave.SaveSettings(),
               "unwritable data directory reports save failures");
        auto invalid = Flat();
        invalid.platforms[0].period = 0;
        bool threw = false;
        try {
            invalid.Validate();
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Expect(threw, "invalid platform period rejected");
        std::printf("FORGE logic: %d passed, %d failed\n", passes, failures);
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::printf("ERROR: %s\n", e.what());
        return 1;
    }
}
