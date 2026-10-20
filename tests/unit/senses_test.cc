#include "core/objects.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/props_engine.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cmath>

namespace {
    dx::UObject* findActor(dx::World& world, const char* name)
    {
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->name == dx::Name(name)) {
                return a;
            }
        }
        return nullptr;
    }
}

TEST_CASE("pawn senses follow the original hearing, smell and apparent size rules", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::World world;
    REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
    dx::UObject* player = world.player();
    dx::UObject* paul = findActor(world, "PaulDenton0");
    REQUIRE(player != nullptr);
    REQUIRE(paul != nullptr);
    dx::actor::bDetectable(player) = 1;
    dx::pawn::HearingThreshold(paul) = 0.1f;
    dx::actor::Location(paul) = { -4760, 10430, -256 };
    dx::actor::Location(player) = { -4460, 10430, -206 };
    float expected = static_cast<float>(1.0 - std::sqrt(300.0 * 300.0 + 100.0 * 100.0) / 800.0) - 0.1f;
    CHECK(dx::aiCanHear(paul, player, 1.0f, 800.0f) == Catch::Approx(expected).margin(1e-5));
    dx::actor::Location(player) = { -4760, 10430, 144 };
    CHECK(dx::aiCanHear(paul, player, 1.0f, 800.0f) == 0.0f);
    CHECK(dx::aiCanSmell(paul, player, 1.0f) == 0.0f);
    dx::pawn::MinAngularSize(paul) = 0.01f;
    dx::actor::Location(player) = { -4760 + 600, 10430, -256 + 38 };
    float r = dx::actor::CollisionRadius(player);
    float h = dx::actor::CollisionHeight(player);
    REQUIRE((r * r + h * h) / (600.0f * 600.0f) < 0.01f);
    CHECK(dx::aiCanSee(paul, player, 1.0f, false, false, false, false) == 0.0f);
    dx::pawn::MinAngularSize(paul) = 0.0f;
    dx::pawn::VisibilityThreshold(paul) = 0.0f;
    double size = (static_cast<double>(r) * r + static_cast<double>(h) * h) / (600.0 * 600.0);
    CHECK(dx::aiCanSee(paul, player, 1.0f, false, false, false, false) == Catch::Approx(size * 64.0).margin(1e-5));
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}
