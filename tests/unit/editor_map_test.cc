#include "core/objects.h"
#include "engine/actor.h"
#include "engine/map_writer.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <filesystem>
#include <string>

namespace {
    dx::ULevel* loadEditorLevel(dx::Linker* linker)
    {
        int32_t ref = linker->findExport(dx::Name("MyLevel"), dx::Name("Level"), "*");
        dx::ULevel* level = dx::cast<dx::ULevel>(linker->object(ref));
        dx::objects::ensureLoaded(level);
        dx::objects::flushLoads();
        for (dx::UObject* a : level->actors) {
            dx::objects::ensureLoaded(a);
        }
        dx::objects::flushLoads();
        dx::objects::ensureLoaded(level->model);
        dx::objects::flushLoads();
        return level;
    }

    dx::UObject* findActor(dx::ULevel* level, const char* name)
    {
        for (dx::UObject* a : level->actors) {
            if (a && a->name == dx::Name(name)) {
                return a;
            }
        }
        return nullptr;
    }

    size_t liveActors(dx::ULevel* level)
    {
        size_t count = 0;
        for (dx::UObject* a : level->actors) {
            if (a) {
                count++;
            }
        }
        return count;
    }

    uint32_t exportFlags(dx::Linker* linker, const char* name)
    {
        int32_t ref = linker->findExport(dx::Name(name), dx::Name(), "*");
        if (ref <= 0) {
            return 0;
        }
        return linker->file().exportAt(ref).flags;
    }
}

TEST_CASE("saved map reloads with the same level and a moved actor", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::objects::setEditorMode(true);
    std::filesystem::path file = std::filesystem::temp_directory_path() / "dx-editor-map-test" / "EditedIsland.dx";
    std::filesystem::remove_all(file.parent_path());
    {
        dx::Linker* linker = dx::objects::loadPackage(dx::Name("01_NYC_UNATCOIsland"));
        REQUIRE(linker != nullptr);
        dx::ULevel* level = loadEditorLevel(linker);
        dx::UObject* paul = findActor(level, "PaulDenton0");
        REQUIRE(paul != nullptr);
        dx::actor::Location(paul) = { -2266.0f, 8294.0f, -256.0f };
        std::string error;
        REQUIRE(dx::mapwriter::saveMap(level, file, &error));
        CHECK(error.empty());
    }
    size_t mismatchesBefore = dx::objects::loadMismatches().size();
    dx::Linker* saved = dx::objects::loadPackageFile(dx::Name("EditedIsland"), file, false);
    REQUIRE(saved != nullptr);
    dx::ULevel* level = loadEditorLevel(saved);
    REQUIRE(level != nullptr);
    CHECK(level->actors.size() == 3658);
    CHECK(liveActors(level) == 3658);
    REQUIRE(level->model != nullptr);
    CHECK(level->model->nodes.size() == 13212);
    CHECK(level->model->surfs.size() == 6821);
    CHECK(level->model->points.size() == 22433);
    CHECK(level->model->verts.size() == 206964);
    CHECK(level->model->lightBits.size() == 4431675);
    CHECK(level->model->leaves.size() == 5597);
    CHECK(level->model->leafHulls.size() == 51104);
    CHECK(level->reachSpecs.size() == 12514);
    dx::UObject* paul = findActor(level, "PaulDenton0");
    REQUIRE(paul != nullptr);
    CHECK(paul->outermost()->name == dx::Name("EditedIsland"));
    CHECK(dx::actor::Location(paul).x == -2266.0f);
    CHECK(dx::actor::Location(paul).y == 8294.0f);
    CHECK(dx::actor::Location(paul).z == -256.0f);
    CHECK(level->actors[0]->cls->name == dx::Name("LevelInfo"));
    CHECK((exportFlags(saved, "Brush520") & (dx::RF_NotForClient | dx::RF_NotForServer)) == (dx::RF_NotForClient | dx::RF_NotForServer));
    CHECK((exportFlags(saved, "Camera11") & (dx::RF_NotForClient | dx::RF_NotForServer)) == (dx::RF_NotForClient | dx::RF_NotForServer));
    CHECK((exportFlags(saved, "PaulDenton0") & (dx::RF_NotForClient | dx::RF_NotForServer)) == 0);
    for (dx::UObject* a : level->actors) {
        dx::objects::ensureLoaded(dx::actor::Brush(a));
    }
    dx::objects::flushLoads();
    const std::vector<std::string>& mismatches = dx::objects::loadMismatches();
    std::string first = mismatches.size() > mismatchesBefore ? mismatches[mismatchesBefore] : std::string();
    INFO("first mismatch: " << first);
    CHECK(mismatches.size() == mismatchesBefore);
    dx::objects::unloadPackage(dx::Name("EditedIsland"));
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::objects::setEditorMode(false);
}
