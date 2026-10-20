#include "core/config.h"
#include "core/objects.h"
#include "engine/actor.h"
#include "engine/console.h"
#include "engine/props_engine.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

namespace {
    std::unique_ptr<dx::World> loadWorld(const std::string& map)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::config::system().set("Engine.Engine", "Root", "DeusEx.DeusExRootWindow");
        dx::objects::unloadPackage(dx::Name(map));
        auto world = std::make_unique<dx::World>();
        REQUIRE(world->loadMap(map));
        return world;
    }

    void unloadWorld(std::unique_ptr<dx::World>& world, const std::string& map)
    {
        dx::console::resetInput(world->player());
        dx::objects::unloadPackage(dx::Name(map));
        world.reset();
    }

    std::vector<dx::UObject*> overlappingCylinders(dx::World& world, const dx::Vector& boxMin, const dx::Vector& boxMax)
    {
        std::vector<dx::UObject*> expected;
        for (dx::UObject* a : world.actors) {
            if (!a || a->destroyed || !dx::actor::bCollideActors(a) || dx::isBrush(a)) {
                continue;
            }
            dx::Vector location = dx::actor::Location(a);
            float radius = dx::actor::CollisionRadius(a);
            float height = dx::actor::CollisionHeight(a);
            if (location.x + radius < boxMin.x || location.x - radius > boxMax.x || location.y + radius < boxMin.y || location.y - radius > boxMax.y || location.z + height < boxMin.z || location.z - height > boxMax.z) {
                continue;
            }
            expected.push_back(a);
        }
        return expected;
    }

    std::vector<dx::UObject*> withoutBrushes(const std::vector<dx::UObject*>& candidates)
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* a : candidates) {
            if (!dx::isBrush(a)) {
                result.push_back(a);
            }
        }
        return result;
    }
}

TEST_CASE("collision hash returns exactly the overlapping cylinders in actor order", "[data]")
{
    std::unique_ptr<dx::World> world = loadWorld("01_NYC_UNATCOIsland");
    std::mt19937 random(12345);
    std::uniform_real_distribution<float> coordinate(-12000.0f, 12000.0f);
    std::uniform_real_distribution<float> height(-1500.0f, 1500.0f);
    std::uniform_real_distribution<float> size(1.0f, 3000.0f);
    size_t nonEmpty = 0;
    for (int i = 0; i < 2000; i++) {
        dx::Vector center { coordinate(random), coordinate(random), height(random) };
        dx::Vector half { size(random) * 0.5f, size(random) * 0.5f, size(random) * 0.25f };
        dx::Vector boxMin = center - half;
        dx::Vector boxMax = center + half;
        std::vector<dx::UObject*> expected = overlappingCylinders(*world, boxMin, boxMax);
        std::vector<dx::UObject*> actual = withoutBrushes(world->collisionHash.query(boxMin, boxMax));
        INFO("query " << i);
        REQUIRE(actual == expected);
        if (!expected.empty()) {
            nonEmpty++;
        }
    }
    CHECK(nonEmpty > 200);

    dx::UObject* player = world->player();
    REQUIRE(player != nullptr);
    dx::Vector destination { -1900, -653, -95 };
    dx::console::execute(player, "teleport -1900 -653 -95 32768");
    dx::Vector moved = dx::actor::Location(player);
    CHECK((moved - destination).size() < 8.0f);
    std::vector<dx::UObject*> near = world->collisionHash.query(moved - dx::Vector { 4, 4, 4 }, moved + dx::Vector { 4, 4, 4 });
    CHECK(std::find(near.begin(), near.end(), player) != near.end());
    std::vector<dx::UObject*> start = world->collisionHash.query({ -4764, 10434, -263 }, { -4756, 10426, -255 });
    CHECK(std::find(start.begin(), start.end(), player) == start.end());

    for (int i = 0; i < 300; i++) {
        world->tick(1.0f / 30.0f);
    }
    for (int i = 0; i < 500; i++) {
        dx::Vector center { coordinate(random), coordinate(random), height(random) };
        dx::Vector half { size(random) * 0.5f, size(random) * 0.5f, size(random) * 0.25f };
        std::vector<dx::UObject*> expected = overlappingCylinders(*world, center - half, center + half);
        std::vector<dx::UObject*> actual = withoutBrushes(world->collisionHash.query(center - half, center + half));
        INFO("query after ticking " << i);
        REQUIRE(actual == expected);
    }
    unloadWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("collision hash finds movers where traces hit them", "[data]")
{
    std::unique_ptr<dx::World> world = loadWorld("01_NYC_UNATCOIsland");
    const dx::Vector directions[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    size_t hits = 0;
    for (dx::UObject* a : world->actors) {
        if (!a || a->destroyed || !dx::isMover(a) || !dx::actor::bCollideActors(a)) {
            continue;
        }
        dx::Vector target = dx::actor::Location(a);
        for (const dx::Vector& direction : directions) {
            dx::CheckResult hit;
            if (!world->lineCheck(hit, target + direction * 400.0f, target, {}, nullptr, dx::TRACE_Movers)) {
                continue;
            }
            if (hit.actor != a) {
                continue;
            }
            hits++;
            std::vector<dx::UObject*> found = world->collisionHash.query(hit.location - dx::Vector { 1, 1, 1 }, hit.location + dx::Vector { 1, 1, 1 });
            INFO(a->name.c_str());
            CHECK(std::find(found.begin(), found.end(), a) != found.end());
        }
    }
    CHECK(hits >= 10);
    unloadWorld(world, "01_NYC_UNATCOIsland");
}

TEST_CASE("live actor snapshot drops destroyed actors and picks up spawned ones", "[data]")
{
    std::unique_ptr<dx::World> world = loadWorld("01_NYC_UNATCOIsland");
    std::shared_ptr<const std::vector<dx::UObject*>> before = world->liveActors();
    dx::UObject* victim = nullptr;
    for (dx::UObject* a : *before) {
        if (a->name == dx::Name("Terrorist1")) {
            victim = a;
        }
    }
    REQUIRE(victim != nullptr);
    REQUIRE(world->destroy(victim));
    std::shared_ptr<const std::vector<dx::UObject*>> after = world->liveActors();
    CHECK(after->size() < before->size());
    CHECK(std::find(after->begin(), after->end(), victim) == after->end());
    CHECK(std::find(before->begin(), before->end(), victim) != before->end());
    dx::UObject* spawned = world->spawn(dx::objects::findClass("DeusEx.MedKit"), nullptr, dx::Name(), { -4700, 10430, -250 }, {}, nullptr, false);
    REQUIRE(spawned != nullptr);
    std::shared_ptr<const std::vector<dx::UObject*>> later = world->liveActors();
    CHECK(later->size() == after->size() + 1);
    CHECK(later->back() == spawned);
    unloadWorld(world, "01_NYC_UNATCOIsland");
}
