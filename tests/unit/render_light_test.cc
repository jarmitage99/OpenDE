#include "core/objects.h"
#include "engine/props_engine.h"
#include "engine/world.h"
#include "render/lighting.h"

#include <catch_amalgamated.hpp>

namespace {
    dx::UObject* makeSpotlight()
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UClass* lightClass = dx::objects::findClass("Engine.Light");
        REQUIRE(lightClass != nullptr);
        dx::UObject* light = dx::objects::newObject(lightClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
        dx::actor::LightRadius(light) = 3;
        dx::actor::LightEffect(light) = 12;
        dx::actor::LightCone(light) = 128;
        dx::actor::Rotation(light) = { -16384, 0, 0 };
        return light;
    }
}

TEST_CASE("prepared spotlight illumination matches hand computed values", "[data]")
{
    dx::UObject* light = makeSpotlight();
    dx::Vector location { 0, 0, 100 };
    dx::Vector up { 0, 0, 1 };
    dx::PreparedLight prepared = dx::prepareLight(light, location);
    CHECK(prepared.radius == 100.0f);
    CHECK(prepared.spotDirection.z == Catch::Approx(1.0f).margin(0.0001));
    CHECK(prepared.outer == Catch::Approx(0.5f).margin(0.00001));
    CHECK(dx::lightIllumination(prepared, { 0, 0, 50 }, up, 0) == Catch::Approx(0.5f).margin(0.00005));
    CHECK(dx::lightIllumination(prepared, { 30, 0, 50 }, up, 0) == Catch::Approx(0.16499f).margin(0.00005));
    CHECK(dx::lightIllumination(prepared, { 60, 0, 90 }, up, 0) == 0.0f);
    CHECK(dx::lightIllumination(light, location, { 30, 0, 50 }, up, 0) == Catch::Approx(0.16499f).margin(0.00005));
}
