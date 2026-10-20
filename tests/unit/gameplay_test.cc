#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/console.h"
#include "engine/input.h"
#include "engine/props_engine.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>

namespace {
    std::unique_ptr<dx::World> freshWorld(const std::string& map)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::config::system().set("Engine.Engine", "Root", "DeusEx.DeusExRootWindow");
        dx::objects::unloadPackage(dx::Name(map));
        auto world = std::make_unique<dx::World>();
        world->inputHook = [](dx::UObject* player, float delta) {
            dx::console::readInput(player, delta);
        };
        REQUIRE(world->loadMap(map));
        return world;
    }

    void releaseWorld(std::unique_ptr<dx::World>& world, const std::string& map)
    {
        dx::console::resetInput(world->player());
        dx::objects::unloadPackage(dx::Name(map));
        world.reset();
    }

    dx::UObject* findActor(dx::World& world, const char* name)
    {
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->name == dx::Name(name)) {
                return a;
            }
        }
        return nullptr;
    }

    void tick(dx::World& world, int frames)
    {
        for (int i = 0; i < frames; i++) {
            world.tick(1.0f / 30.0f);
        }
    }
}

TEST_CASE("player climbs a ladder by walking into it", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    REQUIRE(player != nullptr);
    dx::console::execute(player, "teleport 6040 -1742 260 16384");
    tick(*world, 10);
    float startZ = dx::actor::Location(player).z;
    CHECK(startZ < 270.0f);
    uint8_t forward = dx::console::keyFromName("W");
    dx::console::key(forward, dx::IST_Press, 0);
    tick(*world, 30);
    dx::console::key(forward, dx::IST_Release, 0);
    float climbed = dx::actor::Location(player).z - startZ;
    CHECK(climbed > 60.0f);
    CHECK(dx::actor::Physics(player) == 1);
    tick(*world, 30);
    CHECK(dx::actor::Location(player).z - startZ > 60.0f);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("traces do not report actors behind level geometry", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("04_NYC_NSFHQ");
    dx::UObject* barrel = findActor(*world, "Barrel64");
    REQUIRE(barrel != nullptr);
    dx::Vector start { 61.99f, 1328.6f, -240.0f };
    dx::Vector end { 61.99f, 6328.6f, -240.0f };
    std::vector<dx::CheckResult> hits = world->multiLineCheck(start, end, {}, nullptr, dx::TRACE_AllColliding);
    REQUIRE_FALSE(hits.empty());
    CHECK(hits.front().actor == world->levelInfo);
    bool barrelHit = false;
    for (const dx::CheckResult& hit : hits) {
        if (hit.actor == barrel) {
            barrelHit = true;
        }
    }
    CHECK_FALSE(barrelHit);
    std::vector<dx::CheckResult> actorsOnly = world->multiLineCheck(start, end, {}, nullptr, dx::TRACE_AllColliding & ~(dx::TRACE_Level | dx::TRACE_LevelGeometry));
    bool barrelWithoutLevel = false;
    for (const dx::CheckResult& hit : actorsOnly) {
        if (hit.actor == barrel) {
            barrelWithoutLevel = true;
        }
    }
    CHECK(barrelWithoutLevel);
    releaseWorld(world, "04_NYC_NSFHQ");
}

TEST_CASE("level traces report the texture of the surface that was hit", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::CheckResult hit;
    REQUIRE(world->lineCheck(hit, { 6040, -1742, 300 }, { 6040, -1600, 300 }, {}, nullptr, dx::TRACE_World));
    REQUIRE(hit.material != nullptr);
    CHECK(hit.material->name == dx::Name("LadrBrwnMetal"));
    REQUIRE(hit.material->outer != nullptr);
    CHECK(hit.material->outer->name == dx::Name("Ladder"));
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("player swims in a water zone", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    REQUIRE(player != nullptr);
    dx::console::execute(player, "teleport -2802 6641 -480 0");
    tick(*world, 20);
    REQUIRE(dx::region(player).zone != nullptr);
    CHECK(dx::region(player).zone->name == dx::Name("WaterZone1"));
    CHECK(dx::actor::Physics(player) == 3);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("frobbing an unlocked door opens it", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* door = findActor(*world, "DeusExMover9");
    REQUIRE(door != nullptr);
    dx::Vector closed = dx::actor::Location(door);
    dx::Rotator closedRotation = dx::actor::Rotation(door);
    dx::vm::Call frob(door, dx::Name("Frob"));
    REQUIRE(frob.valid());
    frob.obj(player).obj(nullptr).run();
    tick(*world, 90);
    dx::Vector moved = dx::actor::Location(door) - closed;
    dx::Rotator turned = dx::actor::Rotation(door);
    bool changed = moved.sizeSquared() > 1.0f || turned.yaw != closedRotation.yaw || turned.pitch != closedRotation.pitch;
    CHECK(changed);
    CHECK(dx::mover::KeyNum(door) == 1);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("each mission starts its mission script with a living player", "[data]")
{
    const std::pair<const char*, const char*> missions[] = {
        { "00_Training", "Mission00" },
        { "01_NYC_UNATCOIsland", "Mission01" },
        { "02_NYC_BatteryPark", "Mission02" },
        { "03_NYC_UNATCOIsland", "Mission03" },
        { "04_NYC_UNATCOIsland", "Mission04" },
        { "05_NYC_UNATCOMJ12lab", "Mission05" },
        { "06_HongKong_Helibase", "Mission06" },
        { "08_NYC_Street", "Mission08" },
        { "09_NYC_Dockyard", "Mission09" },
        { "10_Paris_Catacombs", "Mission10" },
        { "11_Paris_Cathedral", "Mission11" },
        { "12_Vandenberg_Cmd", "Mission12" },
        { "14_Vandenberg_Sub", "Mission14" },
        { "15_Area51_Bunker", "Mission15" },
        { "99_Endgame1", "MissionEndgame" },
    };
    for (const auto& [map, script] : missions) {
        INFO(map);
        std::unique_ptr<dx::World> world = freshWorld(map);
        tick(*world, 60);
        dx::UObject* mission = nullptr;
        for (dx::UObject* a : world->actors) {
            if (a && !a->destroyed && a->isA(dx::Name("MissionScript"))) {
                mission = a;
            }
        }
        REQUIRE(mission != nullptr);
        CHECK(mission->cls->name == dx::Name(script));
        REQUIRE(world->player() != nullptr);
        CHECK(dx::pawn::Health(world->player()) > 0);
        releaseWorld(world, map);
    }
}

TEST_CASE("animals looking for food do not stall the ocean lab", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("14_OceanLab_Lab");
    auto start = std::chrono::steady_clock::now();
    tick(*world, 300);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    CHECK(seconds < 60.0);
    releaseWorld(world, "14_OceanLab_Lab");
}

TEST_CASE("picking a weak lock with one lockpick unlocks the door and uses the pick", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* door = findActor(*world, "DeusExMover10");
    REQUIRE(door != nullptr);
    dx::UProperty* locked = door->cls->findProperty(dx::Name("bLocked"));
    REQUIRE(locked != nullptr);
    REQUIRE(locked->toText(door->data + locked->offset) == "True");
    dx::console::execute(player, "teleport 1872 -1808 -100 16384");
    dx::console::execute(player, "lookat 1872 -1748 -80");
    dx::console::execute(player, "giveitem DeusEx.Lockpick");
    dx::UObject* pick = nullptr;
    for (dx::UObject* item = dx::actor::Inventory(player); item; item = dx::actor::Inventory(item)) {
        if (item->isA(dx::Name("Lockpick"))) {
            pick = item;
        }
    }
    REQUIRE(pick != nullptr);
    dx::vm::Call(player, dx::Name("PutInHand")).obj(pick).run();
    tick(*world, 15);
    dx::vm::Call(door, dx::Name("Frob")).obj(player).obj(pick).run();
    tick(*world, 240);
    CHECK(locked->toText(door->data + locked->offset) == "False");
    bool stillHasPick = false;
    for (dx::UObject* item = dx::actor::Inventory(player); item; item = dx::actor::Inventory(item)) {
        if (item->isA(dx::Name("Lockpick"))) {
            stillHasPick = true;
        }
    }
    CHECK_FALSE(stillHasPick);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("putting an item in hand does not close the open inventory screen", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::console::execute(player, "giveitem DeusEx.Lockpick");
    dx::console::execute(player, "giveitem DeusEx.Medkit");
    dx::vm::Call(player, dx::Name("ShowInventoryWindow")).run();
    tick(*world, 5);
    dx::UProperty* rootProperty = player->cls->findProperty(dx::Name("rootWindow"));
    dx::UObject* root = *reinterpret_cast<dx::UObject**>(player->data + rootProperty->offset);
    REQUIRE(root != nullptr);
    dx::vm::Call top(root, dx::Name("GetTopWindow"));
    REQUIRE(top.valid());
    top.run();
    dx::UObject* screen = *reinterpret_cast<dx::UObject**>(top.returnValue());
    REQUIRE(screen != nullptr);
    CHECK(screen->cls->name == dx::Name("PersonaScreenInventory"));
    for (dx::UObject* item = dx::actor::Inventory(player); item; item = dx::actor::Inventory(item)) {
        if (item->isA(dx::Name("Lockpick")) || item->isA(dx::Name("MedKit"))) {
            dx::vm::Call(player, dx::Name("PutInHand")).obj(item).run();
            tick(*world, 5);
        }
    }
    dx::vm::Call after(root, dx::Name("GetTopWindow"));
    after.run();
    CHECK(*reinterpret_cast<dx::UObject**>(after.returnValue()) == screen);
    CHECK_FALSE(screen->destroyed);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("shooting an explosive barrel destroys it and hurts a nearby player", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* barrel = findActor(*world, "Barrel0");
    REQUIRE(barrel != nullptr);
    dx::Vector barrelLocation = dx::actor::Location(barrel);
    dx::console::execute(player, "teleport 1433 -10144 -260 32768");
    tick(*world, 5);
    REQUIRE((dx::actor::Location(player) - barrelLocation).size2D() < 120.0f);
    int startHealth = dx::pawn::Health(player);
    dx::vm::Call damage(barrel, dx::Name("TakeDamage"));
    REQUIRE(damage.valid());
    damage.i(1000).obj(player).v(barrelLocation).v({}).n(dx::Name("Shot")).run();
    tick(*world, 60);
    CHECK((barrel->destroyed || dx::actor::bDeleteMe(barrel)));
    CHECK(dx::pawn::Health(player) < startHealth);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("killing a terrorist leaves a carcass whose items the player can take", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* terrorist = findActor(*world, "Terrorist1");
    REQUIRE(terrorist != nullptr);
    tick(*world, 30);
    REQUIRE(dx::actor::Inventory(terrorist) != nullptr);
    std::vector<dx::UObject*> existing;
    for (dx::UObject* a : world->actors) {
        if (a && !a->destroyed && a->isA(dx::Name("DeusExCarcass"))) {
            existing.push_back(a);
        }
    }
    dx::vm::Call damage(terrorist, dx::Name("TakeDamage"));
    REQUIRE(damage.valid());
    damage.i(150).obj(player).v(dx::actor::Location(terrorist)).v({}).n(dx::Name("Shot")).run();
    tick(*world, 150);
    CHECK(terrorist->destroyed);
    dx::UObject* carcass = nullptr;
    for (dx::UObject* a : world->actors) {
        if (a && !a->destroyed && a->isA(dx::Name("TerroristCarcass")) && std::find(existing.begin(), existing.end(), a) == existing.end()) {
            carcass = a;
        }
    }
    REQUIRE(carcass != nullptr);
    dx::Vector spot = dx::actor::Location(carcass);
    dx::console::execute(player, "teleport " + std::to_string(spot.x + 60) + " " + std::to_string(spot.y) + " " + std::to_string(spot.z + 30) + " 32768");
    dx::vm::Call(carcass, dx::Name("Frob")).obj(player).obj(nullptr).run();
    tick(*world, 5);
    bool gotItem = false;
    for (dx::UObject* item = dx::actor::Inventory(player); item; item = dx::actor::Inventory(item)) {
        if (item->isA(dx::Name("WeaponRifle")) || item->isA(dx::Name("Ammo3006")) || item->isA(dx::Name("WeaponCombatKnife")) || item->isA(dx::Name("TechGoggles"))) {
            gotItem = true;
        }
    }
    CHECK(gotItem);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("player picks up a light decoration and throws it", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::console::execute(player, "teleport -4300 10400 -259 32768");
    tick(*world, 5);
    dx::Vector start = dx::actor::Location(player);
    dx::UObject* box = world->spawn(dx::objects::findClass("DeusEx.BoxSmall"), nullptr, dx::Name(), start + dx::Vector { -60, 0, -30 }, {}, nullptr, false);
    REQUIRE(box != nullptr);
    tick(*world, 20);
    dx::Vector spot = dx::actor::Location(box);
    dx::console::execute(player, "lookat " + std::to_string(spot.x) + " " + std::to_string(spot.y) + " " + std::to_string(spot.z));
    tick(*world, 10);
    dx::UProperty* frobTarget = player->cls->findProperty(dx::Name("FrobTarget"));
    REQUIRE(*reinterpret_cast<dx::UObject**>(player->data + frobTarget->offset) == box);
    dx::vm::Call(player, dx::Name("GrabDecoration")).run();
    tick(*world, 10);
    dx::UProperty* carried = player->cls->findProperty(dx::Name("CarriedDecoration"));
    REQUIRE(*reinterpret_cast<dx::UObject**>(player->data + carried->offset) == box);
    dx::console::execute(player, "lookat -4600 10400 -200");
    tick(*world, 5);
    dx::vm::Call(player, dx::Name("DropDecoration")).run();
    CHECK(*reinterpret_cast<dx::UObject**>(player->data + carried->offset) == nullptr);
    CHECK(dx::actor::Physics(box) == 2);
    tick(*world, 30);
    CHECK(dx::actor::Location(box).x < start.x - 80.0f);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("a security camera spots the player and raises the alarm", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* camera = findActor(*world, "SecurityCamera3");
    REQUIRE(camera != nullptr);
    dx::UProperty* seen = camera->cls->findProperty(dx::Name("bPlayerSeen"));
    dx::UProperty* triggered = camera->cls->findProperty(dx::Name("bEventTriggered"));
    CHECK(seen->toText(camera->data + seen->offset) == "False");
    dx::console::execute(player, "teleport 191 223 -60 0");
    tick(*world, 180);
    CHECK(seen->toText(camera->data + seen->offset) == "True");
    CHECK(triggered->toText(camera->data + triggered->offset) == "True");
    REQUIRE(dx::actor::AmbientSound(camera) != nullptr);
    CHECK(dx::actor::AmbientSound(camera)->name == dx::Name("Klaxon2"));
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("a patrolling terrorist reacts to a nearby explosion", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* player = world->player();
    dx::UObject* crate = findActor(*world, "CrateExplosiveSmall10");
    dx::UObject* terrorist = findActor(*world, "Terrorist2");
    REQUIRE(crate != nullptr);
    REQUIRE(terrorist != nullptr);
    for (int i = 0; i < 60 && dx::vm::stateName(terrorist) != dx::Name("Patrolling"); i++) {
        dx::actor::LastRenderTime(terrorist) = world->timeSeconds;
        tick(*world, 1);
    }
    REQUIRE(dx::vm::stateName(terrorist) == dx::Name("Patrolling"));
    dx::actor::LastRenderTime(terrorist) = world->timeSeconds;
    dx::vm::Call damage(crate, dx::Name("TakeDamage"));
    damage.i(1000).obj(player).v(dx::actor::Location(crate)).v({}).n(dx::Name("Shot")).run();
    bool reacted = false;
    for (int i = 0; i < 90 && !reacted; i++) {
        tick(*world, 1);
        reacted = dx::vm::stateName(terrorist) != dx::Name("Patrolling");
    }
    INFO(dx::vm::stateName(terrorist).str());
    CHECK(reacted);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("a door's bounding box spans the faces that traces hit from both sides", "[data]")
{
    std::unique_ptr<dx::World> world = freshWorld("01_NYC_UNATCOIsland");
    dx::UObject* door = findActor(*world, "DeusExMover10");
    REQUIRE(door != nullptr);
    dx::CheckResult front;
    REQUIRE(world->lineCheck(front, { 1872, -1808, -60 }, { 1872, -1608, -60 }, {}, nullptr, dx::TRACE_Movers));
    REQUIRE(front.actor == door);
    dx::CheckResult back;
    REQUIRE(world->lineCheck(back, { 1872, -1608, -60 }, { 1872, -1808, -60 }, {}, nullptr, dx::TRACE_Movers));
    REQUIRE(back.actor == door);
    dx::vm::Call box(door, dx::Name("GetBoundingBox"));
    REQUIRE(box.valid());
    box.v({}).v({}).b(false).v(dx::actor::Location(door)).r(dx::actor::Rotation(door));
    box.run();
    dx::Vector minimum = *reinterpret_cast<dx::Vector*>(box.parameter(0));
    dx::Vector maximum = *reinterpret_cast<dx::Vector*>(box.parameter(1));
    CHECK(minimum.y == Catch::Approx(front.location.y).margin(1.0));
    CHECK(maximum.y == Catch::Approx(back.location.y).margin(1.0));
    CHECK(minimum.x <= 1872.0f);
    CHECK(maximum.x >= 1872.0f);
    CHECK(minimum.z <= -60.0f);
    CHECK(maximum.z >= -60.0f);
    releaseWorld(world, "01_NYC_UNATCOIsland");
}
