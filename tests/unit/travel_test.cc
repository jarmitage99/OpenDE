#include "core/objects.h"
#include "core/vm.h"
#include "engine/props_extension.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

namespace {
    bool boolFlag(dx::UObject* flagBase, const char* name)
    {
        dx::vm::Call call(flagBase, dx::Name("GetBool"));
        call.n(dx::Name(name));
        call.run();
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    }
}

TEST_CASE("ClientTravel sends PreTravel to the player and the mission script", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::World world;
    REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
    for (int i = 0; i < 30; i++) {
        world.tick(1.0f / 30.0f);
    }
    dx::UObject* player = world.player();
    REQUIRE(player != nullptr);
    dx::UObject* flagBase = dx::playerpawnext::FlagBase(player);
    REQUIRE(flagBase != nullptr);
    dx::UObject* mission = nullptr;
    for (dx::UObject* a : world.actors) {
        if (a && !a->destroyed && a->isA(dx::Name("MissionScript"))) {
            mission = a;
        }
    }
    REQUIRE(mission != nullptr);
    uint8_t* missionFlags = mission->propertyAddress(dx::Name("flags"));
    REQUIRE(missionFlags != nullptr);
    CHECK(*reinterpret_cast<dx::UObject**>(missionFlags) == flagBase);
    CHECK_FALSE(boolFlag(flagBase, "PlayerTraveling"));
    {
        dx::vm::Call call(player, dx::Name("ClientTravel"));
        call.s("02_NYC_BatteryPark").byte(0).b(true);
        call.run();
    }
    CHECK(boolFlag(flagBase, "PlayerTraveling"));
    CHECK(*reinterpret_cast<dx::UObject**>(missionFlags) == nullptr);
    CHECK(world.travelPending);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}

TEST_CASE("travel carries changed values and keeps defaults from overwriting the new player", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    std::vector<dx::TravelActor> travel;
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        uint8_t* augmentations = player->propertyAddress(dx::Name("AugmentationSystem"));
        uint8_t* credits = player->propertyAddress(dx::Name("Credits"));
        REQUIRE(augmentations != nullptr);
        REQUIRE(credits != nullptr);
        *reinterpret_cast<dx::UObject**>(augmentations) = nullptr;
        *reinterpret_cast<int32_t*>(credits) = 1234;
        travel = world.collectTravel();
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::World next;
    next.incomingTravel = std::move(travel);
    REQUIRE(next.loadMap("01_NYC_UNATCOIsland"));
    dx::UObject* player = next.player();
    REQUIRE(player != nullptr);
    dx::UObject* augmentations = *reinterpret_cast<dx::UObject**>(player->propertyAddress(dx::Name("AugmentationSystem")));
    REQUIRE(augmentations != nullptr);
    CHECK_FALSE(augmentations->destroyed);
    CHECK(*reinterpret_cast<int32_t*>(player->propertyAddress(dx::Name("Credits"))) == 1234);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}

TEST_CASE("the travel string uses the original GetTravelString layout", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::World world;
    REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
    for (int i = 0; i < 30; i++) {
        world.tick(1.0f / 30.0f);
    }
    dx::UObject* player = world.player();
    REQUIRE(player != nullptr);
    std::string text = world.travelString(player);
    std::string header = "Class=DeusEx.JCDentonMale Name=" + player->name.str() + " Ptr=";
    REQUIRE(text.rfind(header, 0) == 0);
    size_t open = text.find("\r\n{\r\n");
    size_t close = text.find("}\r\n");
    REQUIRE(open != std::string::npos);
    REQUIRE(close != std::string::npos);
    std::string block = text.substr(open, close - open);
    const char* expected[] = { "\r\nSkillSystem=", "\r\nKeyRing=", "\r\nBarkManager=", "\r\nThemeManager=", "\r\nFlagBase=" };
    for (const char* line : expected) {
        INFO(line);
        CHECK(block.find(line) != std::string::npos);
    }
    size_t keyRing = block.find("\r\nKeyRing=");
    std::string keyRingName = block.substr(keyRing + 10 + 9, block.find("\r\n", keyRing + 2) - keyRing - 19);
    CHECK(text.find("Name=" + keyRingName + " Ptr=") != std::string::npos);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}
