#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/path_builder.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace {
    struct Island {
        std::unique_ptr<dx::World> world;

        Island()
        {
            dx::engine::initialize(DEUSEX_GAME_DATA);
            dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
            world = std::make_unique<dx::World>();
            REQUIRE(world->loadMap("01_NYC_UNATCOIsland"));
        }

        ~Island()
        {
            world.reset();
            dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
        }

        dx::UObject* actor(const char* name)
        {
            for (dx::UObject* a : world->actors) {
                if (a && !a->destroyed && a->name == dx::Name(name)) {
                    return a;
                }
            }
            return nullptr;
        }
    };

    dx::UObject* walker(Island& island)
    {
        dx::UObject* paul = island.actor("PaulDenton0");
        REQUIRE(paul != nullptr);
        dx::actor::Physics(paul) = dx::PHYS_Walking;
        dx::actor::Velocity(paul) = {};
        dx::actor::Acceleration(paul) = {};
        dx::vm::stateFrame(paul).latentAction = 0;
        return paul;
    }

    void moveTo(dx::UObject* self, const dx::Vector& destination, float speed)
    {
        dx::vm::Call call(self, dx::Name("MoveTo"));
        call.v(destination).f(speed).run();
    }

    dx::UObject* objectResult(dx::vm::Call& call)
    {
        return *reinterpret_cast<dx::UObject**>(call.returnValue());
    }

    bool boolResult(dx::vm::Call& call)
    {
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    }
}

TEST_CASE("MoveTo caps the speed at MaxDesiredSpeed and times the move by 1.3 times the distance over the ground speed", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::pawn::GroundSpeed(paul) = 300.0f;
    dx::pawn::MaxDesiredSpeed(paul) = 0.5f;
    dx::Vector start = dx::actor::Location(paul);
    moveTo(paul, start + dx::Vector { 390, 0, 0 }, 0.8f);
    CHECK(dx::pawn::DesiredSpeed(paul) == 0.5f);
    CHECK(dx::pawn::MoveTimer(paul) == Catch::Approx(1.0 + 1.3 * 390.0 / 150.0).margin(1e-4));
    moveTo(paul, start + dx::Vector { 390, 0, 0 }, 0.3f);
    CHECK(dx::pawn::DesiredSpeed(paul) == 0.3f);
    CHECK(dx::pawn::MoveTimer(paul) == Catch::Approx(1.0 + 1.3 * 390.0 / 90.0).margin(1e-4));
    moveTo(paul, start + dx::Vector { 390, 0, 0 }, 0.0f);
    CHECK(dx::pawn::MoveTimer(paul) == 0.5f);
    CHECK(dx::vm::stateFrame(paul).latentAction == 501);
}

TEST_CASE("MoveToward a pawn allows 1.2 seconds and walking toward a pawn keeps the pitch", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* player = island.world->player();
    REQUIRE(player != nullptr);
    dx::actor::Location(player) = dx::actor::Location(paul) + dx::Vector { 300, 0, 300 };
    dx::vm::Call call(paul, dx::Name("MoveToward"));
    call.obj(player).f(1.0f).run();
    CHECK(dx::pawn::MoveTimer(paul) == 1.2f);
    CHECK(dx::pawn::MoveTarget(paul) == player);
    CHECK(std::abs(dx::actor::DesiredRotation(paul).pitch - 8192) <= 1);
}

TEST_CASE("a move ends within 16 units in 2D when the height difference is below 48", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    REQUIRE(dx::actor::CollisionHeight(paul) < 48.0f);
    dx::actor::Physics(paul) = dx::PHYS_Flying;
    dx::Vector start = dx::actor::Location(paul);
    moveTo(paul, start + dx::Vector { 12, 0, 40 }, 1.0f);
    CHECK(dx::actor::Acceleration(paul).x == 0.0f);
    CHECK(dx::actor::Acceleration(paul).z == 0.0f);
    moveTo(paul, start + dx::Vector { 12, 0, 60 }, 1.0f);
    double length = std::sqrt(12.0 * 12.0 + 60.0 * 60.0);
    CHECK(dx::actor::Acceleration(paul).x == Catch::Approx(12.0 / length * dx::pawn::AccelRate(paul)).margin(1e-3));
    CHECK(dx::actor::Acceleration(paul).z == Catch::Approx(60.0 / length * dx::pawn::AccelRate(paul)).margin(1e-3));
}

TEST_CASE("TurnTo leaves the acceleration and TurnToward and StrafeFacing aim with FaceTarget", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* player = island.world->player();
    dx::actor::Acceleration(paul) = { 100, 0, 0 };
    {
        dx::vm::Call call(paul, dx::Name("TurnTo"));
        call.v(dx::actor::Location(paul) + dx::Vector { 0, 100, 0 }).run();
    }
    CHECK(dx::actor::Acceleration(paul).x == 100.0f);
    CHECK(dx::vm::stateFrame(paul).latentAction == 509);
    dx::UObject* marker = island.actor("PatrolPoint54");
    REQUIRE(marker != nullptr);
    dx::pawn::MoveTarget(paul) = marker;
    {
        dx::vm::Call call(paul, dx::Name("TurnToward"));
        call.obj(player).run();
    }
    CHECK(dx::pawn::MoveTarget(paul) == marker);
    CHECK(dx::pawn::FaceTarget(paul) == player);
    dx::vm::stateFrame(paul).latentAction = 0;
    {
        dx::vm::Call call(paul, dx::Name("StrafeFacing"));
        call.v(dx::actor::Location(paul)).obj(nullptr).f(1.0f).run();
    }
    CHECK(dx::vm::stateFrame(paul).latentAction == 0);
}

TEST_CASE("WaitForLanding waits 2.5 seconds before LongFall", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::actor::Physics(paul) = dx::PHYS_Falling;
    dx::vm::Call call(paul, dx::Name("WaitForLanding"));
    call.run();
    CHECK(dx::actor::LatentFloat(paul) == 2.5f);
    CHECK(dx::vm::stateFrame(paul).latentAction == 528);
}

TEST_CASE("ClearPaths resets every navigation point and FindRandomDest returns None", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* node = island.actor("PatrolPoint54");
    REQUIRE(node != nullptr);
    dx::navigationpoint::visitedWeight(node) = 5;
    dx::navigationpoint::bEndPoint(node) = 1;
    dx::navigationpoint::cost(node) = 77;
    dx::navigationpoint::ExtraCost(node) = 12;
    {
        dx::vm::Call call(paul, dx::Name("ClearPaths"));
        call.run();
    }
    CHECK(dx::navigationpoint::visitedWeight(node) == 10000000);
    CHECK(dx::navigationpoint::bEndPoint(node) == 0);
    CHECK(dx::navigationpoint::cost(node) == 12);
    dx::navigationpoint::visitedWeight(node) = 5;
    dx::vm::Call call(paul, dx::Name("FindRandomDest"));
    call.b(true).run();
    CHECK(objectResult(call) == nullptr);
    CHECK(dx::navigationpoint::visitedWeight(node) == 10000000);
}

TEST_CASE("FindPathToward leaves RouteCache alone, clears SpecialPause and never returns the goal pawn", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* player = island.world->player();
    dx::UObject* marker = island.actor("PatrolPoint54");
    dx::pawn::RouteCache(paul, 0) = marker;
    dx::pawn::SpecialPause(paul) = 3.0f;
    dx::actor::Location(player) = dx::actor::Location(paul) + dx::Vector { 120, 0, 0 };
    dx::vm::Call call(paul, dx::Name("FindPathToward"));
    call.obj(player).b(false).b(true).run();
    CHECK(objectResult(call) != player);
    CHECK(dx::pawn::RouteCache(paul, 0) == marker);
    CHECK(dx::pawn::SpecialPause(paul) == 0.0f);
}

TEST_CASE("pointReachable fails while falling and limits points to 1000 units in 2D", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    REQUIRE(player != nullptr);
    dx::actor::Physics(player) = dx::PHYS_Walking;
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 1100, 0, 0 }, start));
    auto reachable = [&](const dx::Vector& point) {
        dx::vm::Call call(player, dx::Name("pointReachable"));
        call.v(point).run();
        return boolResult(call);
    };
    CHECK(reachable(start + dx::Vector { 100, 0, 0 }));
    CHECK_FALSE(reachable(start + dx::Vector { 0, 1001, 0 }));
    dx::actor::Physics(player) = dx::PHYS_Falling;
    CHECK_FALSE(reachable(start + dx::Vector { 100, 0, 0 }));
}

TEST_CASE("EAdjustJump simulates the arc in 0.05 second steps from the current velocity", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* zone = dx::zoneOf(paul);
    REQUIRE(zone != nullptr);
    dx::zoneinfo::ZoneGravity(zone) = { 0, 0, -950 };
    dx::actor::Velocity(paul) = { 0, 0, 500 };
    dx::pawn::JumpZ(paul) = 400.0f;
    dx::pawn::GroundSpeed(paul) = 2000.0f;
    dx::pawn::Destination(paul) = dx::actor::Location(paul) + dx::Vector { 300, 0, 0 };
    dx::vm::Call call(paul, dx::Name("EAdjustJump"));
    call.run();
    dx::Vector velocity = *reinterpret_cast<dx::Vector*>(call.returnValue());
    CHECK(velocity.z == 500.0f);
    CHECK(velocity.x == Catch::Approx(300.0 / (1.05 - 23.625 / 497.5)).margin(0.05));
    CHECK(velocity.y == Catch::Approx(0.0).margin(1e-3));
}

TEST_CASE("RemovePawn unlinks the pawn and leaves its nextPawn", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* level = island.world->levelInfo;
    dx::UObject* first = dx::levelinfo::PawnList(level);
    REQUIRE(first != nullptr);
    dx::UObject* previous = nullptr;
    for (dx::UObject* p = first; p && p != paul; p = dx::pawn::nextPawn(p)) {
        previous = p;
    }
    REQUIRE(previous != nullptr);
    dx::UObject* after = dx::pawn::nextPawn(paul);
    dx::vm::Call call(paul, dx::Name("RemovePawn"));
    call.run();
    CHECK(dx::pawn::nextPawn(previous) == after);
    CHECK(dx::pawn::nextPawn(paul) == after);
}

TEST_CASE("PickTarget ignores the incoming best distance and only takes pawns within 2000 units", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* player = island.world->player();
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 1100, 0, 0 }, start));
    dx::actor::Location(paul) = start + dx::Vector { 500, 0, 0 };
    REQUIRE(dx::pawn::Health(paul) > 0);
    REQUIRE(dx::actor::bProjTarget(paul) != 0);
    auto pick = [&](const char* function, float& aim, float& distance) {
        dx::vm::Call call(player, dx::Name(function));
        call.f(aim).f(distance).v({ 1, 0, 0 }).v(start).run();
        aim = *reinterpret_cast<float*>(call.parameter(0));
        distance = *reinterpret_cast<float*>(call.parameter(1));
        return objectResult(call);
    };
    float aim = 0.5f;
    float distance = 0.0f;
    CHECK(pick("PickTarget", aim, distance) == paul);
    CHECK(aim == 1.0f);
    CHECK(distance == 500.0f);
    aim = 0.5f;
    distance = 0.0f;
    CHECK(pick("PickAnyTarget", aim, distance) != paul);
    dx::actor::Location(paul) = start + dx::Vector { 2100, 0, 0 };
    aim = 0.5f;
    distance = 0.0f;
    CHECK(pick("PickTarget", aim, distance) == nullptr);
}

TEST_CASE("ComputePathnodeDistances starts from the anchor's reach specs", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* node = island.actor("PatrolPoint54");
    REQUIRE(node != nullptr);
    std::vector<std::pair<dx::UObject*, int32_t>> neighbours;
    for (int i = 0; i < 16; i++) {
        int32_t index = dx::navigationpoint::Paths(node, i);
        if (index == -1) {
            break;
        }
        const dx::ReachSpec& spec = island.world->level->reachSpecs[static_cast<size_t>(index)];
        if (spec.collisionRadius >= static_cast<int32_t>(dx::actor::CollisionRadius(paul)) && spec.collisionHeight >= static_cast<int32_t>(dx::actor::CollisionHeight(paul)) && spec.reachFlags == 1) {
            neighbours.emplace_back(spec.end, spec.distance);
        }
    }
    REQUIRE_FALSE(neighbours.empty());
    for (dx::UObject* n = dx::levelinfo::NavigationPointList(island.world->levelInfo); n; n = dx::navigationpoint::nextNavigationPoint(n)) {
        dx::navigationpoint::visitedWeight(n) = 20000000;
    }
    dx::vm::Call call(paul, dx::Name("ComputePathnodeDistances"));
    call.obj(node).run();
    for (const auto& [end, distance] : neighbours) {
        CHECK(dx::navigationpoint::visitedWeight(end) <= distance);
        CHECK(dx::navigationpoint::visitedWeight(end) > 0);
    }
}

TEST_CASE("AIDirectionReachable walks the direction until it is between the distances and fails while falling", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    dx::actor::Physics(player) = dx::PHYS_Walking;
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 400, 0, 0 }, start));
    dx::Vector best { 1, 2, 3 };
    auto reach = [&]() {
        dx::vm::Call call(player, dx::Name("AIDirectionReachable"));
        call.v(start).i(0).i(8000).f(100.0f).f(200.0f).v(best).run();
        best = *reinterpret_cast<dx::Vector*>(call.parameter(5));
        return boolResult(call);
    };
    CHECK(reach());
    double distance = std::sqrt((best.x - start.x) * (best.x - start.x) + (best.y - start.y) * (best.y - start.y));
    CHECK(distance > 175.0);
    CHECK(distance <= 200.0);
    CHECK(best.y == Catch::Approx(start.y).margin(0.5));
    CHECK(dx::actor::Location(player) == start);
    dx::actor::Physics(player) = dx::PHYS_Falling;
    best = { 1, 2, 3 };
    CHECK_FALSE(reach());
    CHECK(best == dx::Vector { 1, 2, 3 });
}

TEST_CASE("FindStairRotation eases the view pitch toward level on flat ground", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    dx::pawn::ViewRotation(player) = { 2000, 0, 0 };
    auto stair = [&](float delta) {
        dx::vm::Call call(player, dx::Name("FindStairRotation"));
        call.f(delta).run();
        return *reinterpret_cast<int32_t*>(call.returnValue());
    };
    CHECK(stair(0.1f) == 399);
    CHECK(stair(0.5f) == 2000);
    dx::pawn::ViewRotation(player) = { 60000, 0, 0 };
    stair(0.1f);
    CHECK(dx::pawn::ViewRotation(player).pitch == -5536);
}

TEST_CASE("PickWallAdjust steps a walking pawn sideways by its radius plus 16 and refuses while falling", "[data]")
{
    Island island;
    dx::UObject* paul = walker(island);
    dx::UObject* player = island.world->player();
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 400, 0, 0 }, start));
    dx::actor::Location(player) = start + dx::Vector { -300, 0, 0 };
    dx::actor::Location(paul) = start;
    dx::pawn::Destination(paul) = start + dx::Vector { 300, 0, 0 };
    REQUIRE(dx::pawn::bIsPlayer(paul) == 0);
    auto adjust = [&]() {
        dx::vm::Call call(paul, dx::Name("PickWallAdjust"));
        call.run();
        return boolResult(call);
    };
    CHECK(adjust());
    float radius = dx::actor::CollisionRadius(paul);
    CHECK(dx::pawn::Destination(paul).x == Catch::Approx(start.x).margin(1e-3));
    CHECK(dx::pawn::Destination(paul).y == Catch::Approx(start.y - (radius + 16.0f)).margin(1e-3));
    CHECK(dx::actor::Physics(paul) == dx::PHYS_Walking);
    dx::actor::Physics(paul) = dx::PHYS_Falling;
    dx::pawn::Destination(paul) = start + dx::Vector { 300, 0, 0 };
    CHECK_FALSE(adjust());
    CHECK(dx::pawn::Destination(paul).x == Catch::Approx(start.x + 300.0f).margin(1e-3));
}

TEST_CASE("FindBestInventoryPath leads a pawn off the network to the nearest reachable node first", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    dx::actor::Physics(player) = dx::PHYS_Walking;
    dx::pawn::MoveTarget(player) = nullptr;
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 400, 0, 0 }, start));
    dx::actor::Location(player) = start + dx::Vector { 400, 0, 0 };
    dx::vm::Call call(player, dx::Name("FindBestInventoryPath"));
    call.f(0.0f).b(false).run();
    dx::UObject* path = objectResult(call);
    REQUIRE(path != nullptr);
    CHECK(path->name == dx::Name("PathNode28"));
    CHECK(*reinterpret_cast<float*>(call.parameter(0)) == 0.00005f);
}

TEST_CASE("walkReachable steps 16 units in the editor and so gives up after 101 steps", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    dx::actor::Physics(player) = dx::PHYS_Walking;
    REQUIRE(dx::pawn::bCanJump(player) != 0);
    dx::Vector start = dx::actor::Location(player);
    dx::Vector destination = start + dx::Vector { 1700, 0, 0 };
    REQUIRE(dx::pawnWalkReachable(player, destination, 15.0f, 0, nullptr) == 1);
    CHECK(dx::actor::Location(player) == start);
    dx::objects::setEditorMode(true);
    int inEditor = dx::pawnWalkReachable(player, destination, 15.0f, 0, nullptr);
    int nearInEditor = dx::pawnWalkReachable(player, start + dx::Vector { 1500, 0, 0 }, 15.0f, 0, nullptr);
    dx::objects::setEditorMode(false);
    CHECK(inEditor == 0);
    CHECK(nearInEditor == 1);
}

TEST_CASE("the editor lifts the reachability distance limits and the pain zone check for non-pawn goals", "[data]")
{
    Island island;
    dx::UObject* player = island.world->player();
    dx::actor::Physics(player) = dx::PHYS_Walking;
    dx::pawn::MoveTarget(player) = nullptr;
    dx::Vector start = dx::actor::Location(player);
    REQUIRE(island.world->fastLineCheck(start + dx::Vector { 1500, 0, 0 }, start));
    dx::UClass* switchClass = dx::objects::findClass("DeusEx.Switch1");
    dx::objects::flushLoads();
    REQUIRE(switchClass != nullptr);
    dx::UObject* farSwitch = island.world->spawn(switchClass, nullptr, dx::Name(), start + dx::Vector { 850, 0, 0 }, {}, nullptr, true);
    dx::UObject* nearSwitch = island.world->spawn(switchClass, nullptr, dx::Name(), start + dx::Vector { 300, 0, 0 }, {}, nullptr, true);
    REQUIRE(farSwitch != nullptr);
    REQUIRE(nearSwitch != nullptr);
    dx::UObject* painZone = nullptr;
    for (dx::UObject* a : island.world->actors) {
        if (a && !a->destroyed && a->isA(dx::engineClasses().zoneInfo) && a != dx::zoneOf(nearSwitch)) {
            painZone = a;
        }
    }
    REQUIRE(painZone != nullptr);
    dx::zoneinfo::bPainZone(painZone) = 1;
    dx::zoneinfo::DamageType(painZone) = dx::Name("Burned");
    REQUIRE(dx::pawn::ReducedDamageType(player) != dx::Name("Burned"));
    dx::region(nearSwitch).zone = painZone;
    auto actorReachable = [&](dx::UObject* target) {
        dx::vm::Call call(player, dx::Name("actorReachable"));
        call.obj(target).run();
        return boolResult(call);
    };
    auto pointReachable = [&](const dx::Vector& point) {
        dx::vm::Call call(player, dx::Name("pointReachable"));
        call.v(point).run();
        return boolResult(call);
    };
    dx::Vector farPoint = start + dx::Vector { 1100, 0, 0 };
    bool farActorInGame = actorReachable(farSwitch);
    bool painActorInGame = actorReachable(nearSwitch);
    bool farPointInGame = pointReachable(farPoint);
    dx::objects::setEditorMode(true);
    bool farActorInEditor = actorReachable(farSwitch);
    bool painActorInEditor = actorReachable(nearSwitch);
    bool farPointInEditor = pointReachable(farPoint);
    dx::objects::setEditorMode(false);
    CHECK_FALSE(farActorInGame);
    CHECK_FALSE(painActorInGame);
    CHECK_FALSE(farPointInGame);
    CHECK(farActorInEditor);
    CHECK(painActorInEditor);
    CHECK(farPointInEditor);
}
