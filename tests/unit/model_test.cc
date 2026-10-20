#include "core/objects.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <filesystem>

namespace {
    dx::ULevel* loadLevel(const std::string& map)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(map));
        if (!linker) {
            return nullptr;
        }
        int32_t ref = linker->findExport(dx::Name("MyLevel"), dx::Name("Level"), "");
        dx::ULevel* level = dx::cast<dx::ULevel>(linker->object(ref));
        dx::objects::ensureLoaded(level);
        return level;
    }
}

TEST_CASE("level actor list of UNATCO Island", "[data]")
{
    dx::ULevel* level = loadLevel("01_NYC_UNATCOIsland");
    REQUIRE(level != nullptr);
    CHECK(level->actors.size() == 3687);
    REQUIRE(level->actors[0] != nullptr);
    CHECK(level->actors[0]->cls->name == dx::Name("LevelInfo"));
    REQUIRE(level->model != nullptr);
    CHECK(level->model->cls->name == dx::Name("Model"));
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}

TEST_CASE("every map loads without native serialization mismatches", "[data]")
{
    size_t maps = 0;
    for (const auto& entry : std::filesystem::directory_iterator(DEUSEX_GAME_DATA "/Maps")) {
        dx::ULevel* level = loadLevel(entry.path().stem().string());
        INFO(entry.path().filename().string());
        REQUIRE(level != nullptr);
        dx::objects::ensureLoaded(level->model);
        CHECK_FALSE(level->model->nodes.empty());
        dx::objects::unloadPackage(dx::Name(entry.path().stem().string()));
        maps++;
    }
    CHECK(maps == 88);
    const std::vector<std::string>& mismatches = dx::objects::loadMismatches();
    INFO("first mismatch: " << (mismatches.empty() ? std::string() : mismatches.front()) << " of " << mismatches.size());
    CHECK(mismatches.empty());
}
