#pragma once

Level MechanicsLevel() {
    auto level = Flat();
    level.version = 2;
    return level;
}

void TestMechanics(const std::filesystem::path& directory) {
    auto iceLevel = MechanicsLevel();
    iceLevel.platforms[0].surface = Surface::Ice;
    iceLevel.platforms[0].grip = 0.12f;
    World steel(Flat()), ice(iceLevel);
    Step(steel, 3);
    Step(ice, 3);
    Step(steel, 65, {1});
    Step(ice, 65, {1});
    float steelStart = steel.player.position.x, iceStart = ice.player.position.x;
    Step(steel, 12);
    Step(ice, 12);
    Expect(steel.player.velocity.x == 0 && ice.player.velocity.x > 270 &&
               ice.player.position.x - iceStart > steel.player.position.x - steelStart + 35,
           "ice retains momentum after releasing movement");
    Step(ice, 12, {-1});
    Expect(ice.player.velocity.x > 0, "ice needs time to reverse direction");
    World airSteel(Flat()), airIce(iceLevel);
    airSteel.player.position = airIce.player.position = {100, 400};
    Step(airSteel, 10, {1});
    Step(airIce, 10, {1});
    Expect(airSteel.player.velocity.x == airIce.player.velocity.x,
           "ice changes ground traction without weakening air steering");

    auto beltLevel = MechanicsLevel();
    beltLevel.platforms[0].surface = Surface::Conveyor;
    beltLevel.platforms[0].beltSpeed = 120;
    World belt(beltLevel);
    Step(belt, 61);
    Expect(std::fabs(belt.player.position.x - 220) < 1 && belt.player.grounded,
           "conveyor carries an idle grounded runner at its configured speed");
    beltLevel.spawn.x = 500;
    beltLevel.platforms[0].beltSpeed = -120;
    World reverseBelt(beltLevel);
    Step(reverseBelt, 61);
    Expect(std::fabs(reverseBelt.player.position.x - 380) < 1, "conveyor supports reverse travel");
    beltLevel.spawn.x = 100;
    beltLevel.platforms[0].beltSpeed = 120;
    beltLevel.platforms.push_back({{260, 400, 40, 320}});
    World beltWall(beltLevel);
    Step(beltWall, 160);
    Expect(beltWall.player.position.x <= 246.01f, "conveyor cannot push through a solid wall");
    World jumpingBelt(beltLevel);
    Step(jumpingBelt, 2);
    float jumpX = jumpingBelt.player.position.x;
    jumpingBelt.Step({0, true, true}, 1.0f / 60);
    Step(jumpingBelt, 12, {0, false, true});
    Expect(std::fabs(jumpingBelt.player.position.x - jumpX) < 0.01f,
           "jumping immediately releases the belt carry");
    beltLevel.hazards.push_back({180, 670, 2, 50});
    World beltHazard(beltLevel);
    Step(beltHazard, 40);
    Expect(beltHazard.deaths == 1, "belt carry detects thin hazards");

    auto windLevel = MechanicsLevel();
    windLevel.airflows.push_back({{40, 200, 400, 520}, {0, -2700}});
    World wind(windLevel);
    Step(wind, 60);
    Expect(wind.player.position.y < 530 && wind.player.velocity.y < 0 && !wind.player.grounded,
           "updraft lifts a runner without a jump command");
    wind.player.position.x = 700;
    Step(wind, 30);
    Expect(wind.player.velocity.y > 0, "gravity resumes outside an updraft");
    World windDash(windLevel);
    windDash.player.position = {100, 500};
    windDash.Step({1, false, false, true}, 1.0f / 60);
    Expect(windDash.player.velocity.y == 0 && windDash.player.velocity.x == 880,
           "dash crosses airflow without being deflected");

    auto fragileLevel = MechanicsLevel();
    Platform fragile{{300, 600, 220, 22}, {}, 4, 0, true};
    fragile.surface = Surface::Fragile;
    fragile.breakDelay = 0.5f;
    fragile.rebuildDelay = 1;
    fragileLevel.platforms.push_back(fragile);
    World crumble(fragileLevel);
    crumble.player.position = {400, 600};
    Step(crumble, 1);
    Expect(crumble.platformTimers[1] == 0 && crumble.player.support == 1,
           "first landing starts the fragile platform warning");
    Step(crumble, 20);
    Expect(crumble.PlatformActive(1) && crumble.player.grounded,
           "fragile platform remains solid during its warning");
    Step(crumble, 16);
    Expect(!crumble.PlatformActive(1) && crumble.player.position.y > 600,
           "fragile platform removes support after its delay");
    Step(crumble, 60);
    Expect(crumble.PlatformActive(1) && crumble.platformTimers[1] < 0,
           "fragile platform rebuilds after its cooldown");
    crumble.platformTimers[1] = 1.5f;
    crumble.player.position = {400, 620};
    crumble.player.velocity = {};
    Step(crumble, 1);
    Expect(!crumble.PlatformActive(1), "rebuilding never embeds the runner in a platform");
    crumble.Restore(-1, 0);
    Expect(crumble.PlatformActive(1) && crumble.platformTimers[1] == -1,
           "checkpoint restore resets fragile platform state");
    crumble.player.position = {400, 600};
    Step(crumble, 1);
    crumble.Step({0, true, true}, 1.0f / 60);
    Step(crumble, 31, {0, false, true});
    Expect(!crumble.PlatformActive(1), "leaving a fragile platform does not cancel its timer");

    auto crawlerLevel = MechanicsLevel();
    crawlerLevel.crawlers.push_back({{200, 704}, 0, 4});
    World crawler(crawlerLevel);
    Step(crawler, 3);
    Step(crawler, 9, {1, false, false, true});
    Expect(crawler.deaths == 1, "dash does not bypass an armoured crawler");
    World jumpCrawler(crawlerLevel);
    Step(jumpCrawler, 3);
    jumpCrawler.Step({1, true, true}, 1.0f / 60);
    Step(jumpCrawler, 25, {1, false, true});
    Expect(jumpCrawler.deaths == 0 && jumpCrawler.player.position.x > 200,
           "crawler can be cleared by a normal jump");
    Crawler patrol{{500, 704}, 100, 4};
    Expect(patrol.At(0).x == 400 && patrol.At(2).x == 600 && patrol.At(4).x == 400 &&
               patrol.At(1).y == 704,
           "crawler patrol stays on the floor and reverses at its endpoints");

    auto sentryLevel = MechanicsLevel();
    sentryLevel.sentries.push_back({{300, 698}, -1, 250, 3, 0});
    World sentry(sentryLevel);
    Step(sentry, 60);
    Expect(sentry.deaths == 0, "sentry charges before the first bolt reaches the runner");
    Step(sentry, 30);
    Expect(sentry.deaths == 1, "sentry bolt hits a runner in its firing lane");
    World charge(sentryLevel);
    Vector2 bolt;
    charge.time = 0.5f;
    Expect(!charge.SentryBolt(charge.level.sentries[0], bolt), "charge preview is not a live bolt");
    charge.time = 1;
    Expect(charge.SentryBolt(charge.level.sentries[0], bolt) && bolt.x == 210,
           "bolt position follows the visible firing cycle");
    charge.time = 2;
    Expect(!charge.SentryBolt(charge.level.sentries[0], bolt), "sentry has a safe cooldown gap");
    World evade(sentryLevel);
    evade.time = 1.27f;
    evade.Step({1, false, false, true}, 1.0f / 60);
    Expect(evade.deaths == 0, "same-tick dash protects against an arriving bolt");
    sentryLevel.platforms.push_back({{180, 600, 30, 120}});
    World cover(sentryLevel);
    Step(cover, 360);
    Expect(cover.deaths == 0 && cover.SentryRange(cover.level.sentries[0]) == 81,
           "solid cover blocks bolts for repeated firing cycles");
    sentryLevel.platforms.push_back({{285, 600, 10, 120}});
    World muzzleCover(sentryLevel);
    Expect(muzzleCover.SentryRange(muzzleCover.level.sentries[0]) == 0,
           "cover within the bolt radius blocks it at the muzzle");
    auto rightLevel = MechanicsLevel();
    rightLevel.spawn.x = 500;
    rightLevel.sentries.push_back({{300, 698}, 1, 250, 3, 0});
    World rightShot(rightLevel);
    Step(rightShot, 90);
    Expect(rightShot.deaths == 1, "sentries can also fire to the right");
    World wrap(rightLevel);
    wrap.time = 2.99f;
    wrap.Step({}, 0.05f);
    Expect(wrap.deaths == 0, "cycle wrap does not sweep a phantom bolt back to the muzzle");

    auto rejects = [](Level level) {
        try {
            level.Validate();
            return false;
        } catch (const std::runtime_error&) {
            return true;
        }
    };
    auto invalid = iceLevel;
    invalid.platforms[0].grip = std::numeric_limits<float>::quiet_NaN();
    Expect(rejects(invalid), "nonfinite surface traction is rejected");
    invalid = beltLevel;
    invalid.platforms[0].beltSpeed = 1000;
    Expect(rejects(invalid), "excessive conveyor speed is rejected");
    invalid = fragileLevel;
    invalid.platforms[1].breakDelay = 0;
    Expect(rejects(invalid), "fragile platforms require a warning interval");
    invalid = windLevel;
    invalid.airflows[0].force.y = -10000;
    Expect(rejects(invalid), "unbounded airflow is rejected");
    invalid = crawlerLevel;
    invalid.crawlers[0].origin.y = 500;
    Expect(rejects(invalid), "crawler patrol without a floor is rejected");
    invalid = sentryLevel;
    invalid.sentries[0].period = 0.8f;
    Expect(rejects(invalid), "sentry cycle must include charge flight and cooldown");
    invalid = iceLevel;
    invalid.version = 1;
    Expect(rejects(invalid), "new mechanics require version two without changing legacy levels");

    std::filesystem::create_directories(directory);
    auto file = directory / "mechanics.level";
    auto write = [&](const std::string& content) {
        std::ofstream out(file);
        out << content;
    };
    std::string header =
        "version 2\nname test\nsize 3000 960\nspawn 100 720\ngoal 2800 720\ncell 2600 650\n";
    write(header + "ice 0 720 3000 240 0.12\ncrawler 500 704 100 4\n"
                   "airflow 800 300 200 400 0 -2700\nsentry 1500 698 -1 400 3 0\n"
                   "fragile 1800 550 200 20 0.7 2.5\nbelt 2100 600 200 30 -100\n");
    auto loaded = Level::Load(file.string(), 0);
    Expect(loaded.platforms.size() == 3 && loaded.platforms[0].grip == 0.12f &&
               loaded.platforms[2].beltSpeed == -100 && loaded.crawlers.size() == 1 &&
               loaded.sentries.size() == 1 && loaded.airflows.size() == 1,
           "version two directives load complete mechanics data");
    write(header + "ice 0 720 3000 240\n");
    bool malformed = false;
    try {
        Level::Load(file.string(), 0);
    } catch (const std::runtime_error&) {
        malformed = true;
    }
    Expect(malformed, "missing surface parameters are rejected");
    write("version 1\nname legacy\nsize 3000 960\nspawn 100 720\ngoal 2800 720\n"
          "cell 2600 650\nsolid 0 720 3000 240\n");
    Expect(Level::Load(file.string(), 0).version == 1, "legacy version one levels still load");
}
