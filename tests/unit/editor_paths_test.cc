#include "core/objects.h"
#include "engine/actor.h"
#include "engine/csg.h"
#include "engine/path_builder.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <vector>

namespace {
    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    dx::UObject* place(dx::ULevel* level, const char* className, const dx::Vector& location)
    {
        dx::UObject* actor = dx::objects::newObject(dx::objects::findClass(className), level->outermost(), dx::Name(), 0);
        actor->loaded = true;
        dx::actor::Location(actor) = location;
        dx::actor::Level(actor) = level->actors[0];
        dx::actor::XLevel(actor) = level;
        level->actors.push_back(actor);
        return actor;
    }

    dx::ULevel* makeRoom(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        dx::ULevel* level = make<dx::ULevel>("Level", package);
        level->model = make<dx::UModel>("Model", package);
        dx::UObject* info = dx::objects::newObject(dx::objects::findClass("Engine.LevelInfo"), package, dx::Name(), 0);
        info->loaded = true;
        dx::UObject* builder = make<dx::UObject>("Brush", package);
        dx::UModel* builderModel = make<dx::UModel>("Model", package);
        builderModel->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(builder) = builderModel;
        level->actors = { info, builder };
        dx::actor::Level(info) = info;
        dx::csg::buildCube(*builderModel->polys, 2048.0f, 1024.0f, 512.0f, nullptr);
        dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
        dx::csg::rebuild(level);
        return level;
    }

    int pathCount(dx::UObject* node)
    {
        int count = 0;
        while (count < 16 && dx::navigationpoint::Paths(node, count) != -1) {
            count++;
        }
        return count;
    }
}

TEST_CASE("define paths links nearby path nodes with reach specs and prunes the long way round", "[data]")
{
    dx::ULevel* level = makeRoom("PathsDefineTest");
    dx::UObject* a = place(level, "Engine.PathNode", { -300.0f, 0.0f, -241.0f });
    dx::UObject* b = place(level, "Engine.PathNode", { 0.0f, 0.0f, -241.0f });
    dx::UObject* c = place(level, "Engine.PathNode", { 300.0f, 0.0f, -241.0f });
    dx::UObject* far = place(level, "Engine.PathNode", { 900.0f, 450.0f, -241.0f });
    dx::paths::undefinePaths(level);
    dx::paths::definePaths(level);
    CHECK(level->reachSpecs.size() == 8);
    REQUIRE(dx::levelinfo::NavigationPointList(level->actors[0]) == far);
    CHECK(dx::navigationpoint::nextNavigationPoint(far) == c);
    REQUIRE(pathCount(a) == 1);
    const dx::ReachSpec& ab = level->reachSpecs[static_cast<size_t>(dx::navigationpoint::Paths(a, 0))];
    CHECK(ab.start == a);
    CHECK(ab.end == b);
    CHECK(ab.distance == 300);
    CHECK(ab.reachFlags == 1);
    CHECK(ab.collisionRadius >= 52);
    REQUIRE(dx::navigationpoint::PrunedPaths(a, 0) != -1);
    const dx::ReachSpec& ac = level->reachSpecs[static_cast<size_t>(dx::navigationpoint::PrunedPaths(a, 0))];
    CHECK(ac.end == c);
    CHECK(ac.pruned == 1);
    CHECK(pathCount(b) == 2);
    CHECK(pathCount(c) == 2);
    CHECK(pathCount(far) == 1);
    for (dx::UObject* actor : level->actors) {
        CHECK_FALSE(actor->isA(dx::Name("Scout")));
    }
    dx::paths::undefinePaths(level);
    CHECK(level->reachSpecs.empty());
    CHECK(dx::navigationpoint::Paths(b, 0) == -1);
    CHECK(dx::levelinfo::NavigationPointList(level->actors[0]) == nullptr);
}

namespace {
    dx::ULevel* makeLRoom(const char* packageName)
    {
        dx::ULevel* level = makeRoom(packageName);
        dx::UObject* builder = level->actors[1];
        dx::actor::Location(builder) = { 512.0f, 1024.0f, 0.0f };
        dx::csg::buildCube(*dx::cast<dx::UModel>(dx::actor::Brush(builder))->polys, 1024.0f, 1024.0f, 512.0f, nullptr);
        dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
        dx::csg::rebuild(level);
        return level;
    }

    std::vector<dx::UObject*> actorsOf(dx::ULevel* level, const char* className)
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* actor : level->actors) {
            if (!actor->destroyed && actor->isA(dx::Name(className))) {
                result.push_back(actor);
            }
        }
        return result;
    }
}

TEST_CASE("build paths places no path node in a convex room", "[data]")
{
    dx::ULevel* level = makeRoom("PathsBuildConvexTest");
    place(level, "Engine.Inventory", { 400.0f, 100.0f, -200.0f });
    CHECK(dx::paths::buildPaths(level, 1) == 0);
    CHECK(actorsOf(level, "PathNode").empty());
    CHECK(actorsOf(level, "Scout").empty());
}

TEST_CASE("build paths places one path node at the inside corner of an L shaped room", "[data]")
{
    dx::ULevel* level = makeLRoom("PathsBuildCornerTest");
    place(level, "Engine.Inventory", { -600.0f, 0.0f, -200.0f });
    CHECK(dx::paths::buildPaths(level, 1) == 1);
    std::vector<dx::UObject*> nodes = actorsOf(level, "PathNode");
    REQUIRE(nodes.size() == 1);
    dx::Vector location = dx::actor::Location(nodes[0]);
    CHECK(location.x >= 115.0f);
    CHECK(location.x <= 131.0f);
    CHECK(location.y >= 381.0f);
    CHECK(location.y <= 397.0f);
    CHECK(location.z > -256.0f);
    CHECK(location.z < -192.0f);
    CHECK(dx::navigationpoint::Paths(nodes[0], 0) == -1);
    CHECK(dx::navigationpoint::upstreamPaths(nodes[0], 15) == -1);
    CHECK(actorsOf(level, "Scout").empty());
}

TEST_CASE("build paths merges the inside corner into an existing path node", "[data]")
{
    dx::ULevel* level = makeLRoom("PathsBuildMergeTest");
    place(level, "Engine.Inventory", { -600.0f, 0.0f, -200.0f });
    REQUIRE(dx::paths::buildPaths(level, 1) == 1);
    CHECK(dx::paths::removePaths(level) == 1);
    dx::UObject* existing = place(level, "Engine.PathNode", { 160.0f, 360.0f, -245.0f });
    CHECK(dx::paths::buildPaths(level, 1) == 0);
    std::vector<dx::UObject*> nodes = actorsOf(level, "PathNode");
    REQUIRE(nodes.size() == 1);
    CHECK(nodes[0] == existing);
    CHECK(dx::actor::Location(existing).x == 160.0f);
    CHECK(dx::actor::Location(existing).y == 360.0f);
}
