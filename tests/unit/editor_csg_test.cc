#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cmath>

namespace {
    struct TestLevel {
        dx::ULevel* level = nullptr;
        dx::UObject* builder = nullptr;
    };

    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    TestLevel makeLevel(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        TestLevel result;
        result.level = make<dx::ULevel>("Level", package);
        result.level->model = make<dx::UModel>("Model", package);
        result.builder = make<dx::UObject>("Brush", package);
        dx::UModel* builderModel = make<dx::UModel>("Model", package);
        builderModel->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(result.builder) = builderModel;
        dx::brush::CsgOper(result.builder) = dx::csg::CsgActive;
        result.level->actors.push_back(nullptr);
        result.level->actors.push_back(result.builder);
        return result;
    }

    dx::UPolys& builderPolys(TestLevel& test)
    {
        return *dx::cast<dx::UModel>(dx::actor::Brush(test.builder))->polys;
    }

    const dx::BspSurf* surfFacing(const dx::UModel& model, const dx::Vector& direction)
    {
        for (const dx::BspSurf& surf : model.surfs) {
            if (model.vectors[static_cast<size_t>(surf.vNormal)].dot(direction) > 0.99f) {
                return &surf;
            }
        }
        return nullptr;
    }

    bool nodeCovers(const dx::UModel& model, const dx::BspNode& node, float x, float y)
    {
        for (int32_t i = 0; i < node.numVertices; i++) {
            const dx::Vector& a = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)];
            const dx::Vector& b = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
            float cross = (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
            if (cross < 0) {
                return false;
            }
        }
        return node.numVertices >= 3;
    }

    const dx::BspNode* floorNodeAt(const dx::UModel& model, float floorZ, float x, float y)
    {
        for (const dx::BspNode& node : model.nodes) {
            if (node.numVertices < 3 || node.plane.z < 0.99f || std::abs(node.plane.w - floorZ) > 0.5f) {
                continue;
            }
            if (nodeCovers(model, node, x, y)) {
                return &node;
            }
        }
        return nullptr;
    }
}

TEST_CASE("subtracting a cube from the solid world makes one empty room with six walls", "[data]")
{
    TestLevel test = makeLevel("CsgCubeTest");
    dx::csg::buildCube(builderPolys(test), 512.0f, 512.0f, 256.0f, nullptr);
    dx::UObject* room = dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    REQUIRE(room != nullptr);
    dx::csg::Stats stats = dx::csg::rebuild(test.level);
    const dx::UModel& model = *test.level->model;
    CHECK(stats.brushes == 1);
    CHECK(stats.nodes == 6);
    CHECK(stats.surfs == 6);
    CHECK(stats.leaves == 1);
    CHECK(stats.zones == 2);
    CHECK(model.points.size() == 8);
    CHECK_FALSE(dx::csg::pointSolid(model, { 0, 0, 0 }));
    CHECK(dx::csg::pointZone(model, { 0, 0, 0 }) == 1);
    CHECK_FALSE(dx::csg::pointSolid(model, { 250, -250, 120 }));
    CHECK(dx::csg::pointSolid(model, { 262, 0, 0 }));
    CHECK(dx::csg::pointSolid(model, { 0, 0, 134 }));
    CHECK(dx::csg::pointSolid(model, { 0, -270, -140 }));
    const dx::BspSurf* floor = surfFacing(model, { 0, 0, 1 });
    REQUIRE(floor != nullptr);
    CHECK(floor->actor == room);
    int floors = 0;
    for (const dx::BspNode& node : model.nodes) {
        CHECK(node.numVertices == 4);
        CHECK(node.zone[1] == 1);
        CHECK(node.zone[0] == 0);
        CHECK(node.zoneMask == 2);
        if (node.surf == static_cast<int32_t>(floor - model.surfs.data())) {
            floors++;
            CHECK(model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool)].vertex)].z == -128.0f);
        }
    }
    CHECK(floors == 1);
    CHECK(model.leaves[0].zone == 1);
    CHECK(model.leafHulls.size() > 0);
}

TEST_CASE("traces stop at the walls of a rebuilt room", "[data]")
{
    TestLevel test = makeLevel("CsgTraceTest");
    dx::csg::buildCube(builderPolys(test), 512.0f, 512.0f, 256.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::rebuild(test.level);
    dx::World world;
    world.level = test.level;
    dx::CheckResult hit;
    REQUIRE(world.bspLineCheck(hit, { 0, 0, 0 }, { 400, 0, 0 }, { 0, 0, 0 }));
    CHECK(std::abs(hit.time - 0.64f) < 0.002f);
    CHECK(hit.normal.x < -0.99f);
    REQUIRE(world.bspLineCheck(hit, { 0, 0, 0 }, { 400, 0, 0 }, { 16, 16, 16 }));
    CHECK(std::abs(hit.time - 0.5f) < 0.002f);
    REQUIRE(world.bspLineCheck(hit, { 0, 0, 0 }, { 0, 0, -300 }, { 0, 0, 0 }));
    CHECK(std::abs(hit.location.z + 127.5f) < 0.01f);
    CHECK_FALSE(world.bspLineCheck(hit, { -100, -100, 0 }, { 100, 100, 50 }, { 16, 16, 16 }));
}

TEST_CASE("an added block inside a subtracted room is solid and has its own surfaces", "[data]")
{
    TestLevel test = makeLevel("CsgPillarTest");
    dx::csg::buildCube(builderPolys(test), 1024.0f, 1024.0f, 512.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(builderPolys(test), 128.0f, 128.0f, 512.0f, nullptr);
    dx::UObject* pillar = dx::csg::addBrush(test.level, test.builder, dx::csg::CsgAdd);
    dx::csg::Stats stats = dx::csg::rebuild(test.level);
    const dx::UModel& model = *test.level->model;
    CHECK(stats.brushes == 2);
    CHECK(stats.surfs == 10);
    CHECK(dx::csg::pointSolid(model, { 0, 0, 0 }));
    CHECK(dx::csg::pointSolid(model, { 60, 60, 200 }));
    CHECK_FALSE(dx::csg::pointSolid(model, { 70, 0, 0 }));
    CHECK_FALSE(dx::csg::pointSolid(model, { 400, 400, 200 }));
    int pillarSurfs = 0;
    for (const dx::BspSurf& surf : model.surfs) {
        if (surf.actor == pillar) {
            pillarSurfs++;
        }
    }
    CHECK(pillarSurfs == 4);
    CHECK(floorNodeAt(model, -256.0f, 0.0f, 0.0f) == nullptr);
    CHECK(floorNodeAt(model, -256.0f, 200.0f, 0.0f) != nullptr);
    for (const dx::BspNode& node : model.nodes) {
        CHECK(node.numVertices >= 3);
    }
}

TEST_CASE("the later of two coplanar brush faces provides the surface texture", "[data]")
{
    TestLevel test = makeLevel("CsgCoplanarTest");
    dx::UPolys& polys = builderPolys(test);
    dx::UObject* first = dx::objects::loadObject("CoreTexBrick.Brick.rough_greywall", nullptr);
    dx::UObject* second = dx::objects::loadObject("CoreTexBrick.Brick.redbrckpttrn_a", nullptr);
    dx::objects::flushLoads();
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    dx::csg::buildCube(polys, 1024.0f, 1024.0f, 512.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(polys, 256.0f, 256.0f, 128.0f, first);
    dx::actor::Location(test.builder) = { 0.0f, 0.0f, -192.0f };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgAdd);
    dx::csg::buildCube(polys, 128.0f, 128.0f, 128.0f, second);
    dx::actor::Location(test.builder) = { 64.0f, 0.0f, -192.0f };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgAdd);
    dx::csg::rebuild(test.level);
    const dx::UModel& model = *test.level->model;
    const dx::BspNode* left = floorNodeAt(model, -128.0f, -100.0f, 0.0f);
    const dx::BspNode* right = floorNodeAt(model, -128.0f, 80.0f, 10.0f);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    CHECK(model.surfs[static_cast<size_t>(left->surf)].texture == first);
    CHECK(model.surfs[static_cast<size_t>(right->surf)].texture == second);
    int covering = 0;
    for (const dx::BspNode& node : model.nodes) {
        if (node.numVertices >= 3 && node.plane.z > 0.99f && std::abs(node.plane.w + 128.0f) < 0.5f && nodeCovers(model, node, 80.0f, 10.0f)) {
            covering++;
        }
    }
    CHECK(covering == 1);
}

TEST_CASE("a semisolid brush does not cut the floor it stands in but is solid for collision", "[data]")
{
    TestLevel test = makeLevel("CsgSemisolidTest");
    dx::csg::buildCube(builderPolys(test), 1024.0f, 1024.0f, 512.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(builderPolys(test), 128.0f, 128.0f, 128.0f, nullptr);
    dx::actor::Location(test.builder) = { 200.0f, 0.0f, -256.0f };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgAdd, dx::csg::PolySemisolid);
    dx::csg::rebuild(test.level);
    const dx::UModel& model = *test.level->model;
    CHECK(floorNodeAt(model, -256.0f, 200.0f, 0.0f) != nullptr);
    CHECK(dx::csg::pointSolid(model, { 200.0f, 0.0f, -230.0f }));
    CHECK_FALSE(dx::csg::pointSolid(model, { 0.0f, 0.0f, -230.0f }));

    TestLevel solid = makeLevel("CsgSolidBlockTest");
    dx::csg::buildCube(builderPolys(solid), 1024.0f, 1024.0f, 512.0f, nullptr);
    dx::csg::addBrush(solid.level, solid.builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(builderPolys(solid), 128.0f, 128.0f, 128.0f, nullptr);
    dx::actor::Location(solid.builder) = { 200.0f, 0.0f, -256.0f };
    dx::csg::addBrush(solid.level, solid.builder, dx::csg::CsgAdd);
    dx::csg::rebuild(solid.level);
    CHECK(floorNodeAt(*solid.level->model, -256.0f, 200.0f, 0.0f) == nullptr);
}

TEST_CASE("a zone portal polygon joins the zones on its two sides", "[data]")
{
    TestLevel test = makeLevel("CsgPortalZoneTest");
    dx::UPolys& polys = builderPolys(test);
    dx::csg::buildCube(polys, 512.0f, 512.0f, 256.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::actor::Location(test.builder) = { 768.0f, 0.0f, 0.0f };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(polys, 256.0f, 128.0f, 128.0f, nullptr);
    dx::actor::Location(test.builder) = { 384.0f, 0.0f, 0.0f };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::buildSheet(polys, 192.0f, 192.0f, nullptr);
    dx::actor::Rotation(test.builder) = { 16384, 0, 0 };
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgAdd, dx::csg::PolyPortal | dx::csg::PolyInvisible);
    dx::csg::Stats stats = dx::csg::rebuild(test.level);
    const dx::UModel& model = *test.level->model;
    CHECK(stats.zones == 3);
    int32_t first = dx::csg::pointZone(model, { 0, 0, 0 });
    int32_t second = dx::csg::pointZone(model, { 768, 0, 0 });
    REQUIRE(first > 0);
    REQUIRE(second > 0);
    REQUIRE(first != second);
    uint64_t both = (1ull << first) | (1ull << second);
    int portals = 0;
    for (const dx::BspNode& node : model.nodes) {
        if (node.numVertices == 0 || !(model.surfs[static_cast<size_t>(node.surf)].polyFlags & dx::csg::PolyPortal)) {
            continue;
        }
        portals++;
        CHECK(std::min(node.zone[0], node.zone[1]) == std::min(first, second));
        CHECK(std::max(node.zone[0], node.zone[1]) == std::max(first, second));
        CHECK((node.zoneMask & both) == both);
    }
    CHECK(portals >= 1);
    CHECK(model.zones[static_cast<size_t>(first)].connectivity == both);
    CHECK(model.zones[static_cast<size_t>(second)].connectivity == both);
}

TEST_CASE("brush from intersection keeps the part of the builder inside the solid world", "[data]")
{
    TestLevel test = makeLevel("CsgIntersectTest");
    dx::UPolys& polys = builderPolys(test);
    dx::csg::buildCube(polys, 512.0f, 512.0f, 256.0f, nullptr);
    dx::csg::addBrush(test.level, test.builder, dx::csg::CsgSubtract);
    dx::csg::rebuild(test.level);
    dx::csg::buildCube(polys, 256.0f, 256.0f, 256.0f, nullptr);
    dx::actor::Location(test.builder) = { 256.0f, 0.0f, 0.0f };
    REQUIRE(dx::csg::brushFromIntersection(test.level, test.builder, false));
    dx::UModel& brush = *dx::cast<dx::UModel>(dx::actor::Brush(test.builder));
    CHECK(brush.polys->polys.size() == 6);
    CHECK(brush.bounds.min.x == Catch::Approx(0.0f).margin(0.01));
    CHECK(brush.bounds.max.x == Catch::Approx(128.0f).margin(0.01));
    CHECK(brush.bounds.min.y == Catch::Approx(-128.0f).margin(0.01));
    CHECK(brush.bounds.max.z == Catch::Approx(128.0f).margin(0.01));

    dx::csg::buildCube(polys, 256.0f, 256.0f, 256.0f, nullptr);
    REQUIRE(dx::csg::brushFromIntersection(test.level, test.builder, true));
    CHECK(brush.polys->polys.size() == 6);
    CHECK(brush.bounds.min.x == Catch::Approx(-128.0f).margin(0.01));
    CHECK(brush.bounds.max.x == Catch::Approx(0.0f).margin(0.01));
}

TEST_CASE("the cube brush matches the original cube builder vertex order and texture axes", "[data]")
{
    TestLevel test = makeLevel("CsgCubeBuilderTest");
    dx::UPolys& polys = builderPolys(test);
    dx::csg::buildCube(polys, 256.0f, 128.0f, 64.0f, nullptr);
    REQUIRE(polys.polys.size() == 6);
    const dx::Poly& first = polys.polys[0];
    REQUIRE(first.vertices.size() == 4);
    CHECK(first.vertices[0].x == -128.0f);
    CHECK(first.vertices[0].y == -64.0f);
    CHECK(first.vertices[0].z == -32.0f);
    CHECK(first.vertices[1].z == 32.0f);
    CHECK(first.vertices[2].y == 64.0f);
    CHECK(first.normal.x == -1.0f);
    CHECK(first.base.x == -128.0f);
    CHECK(first.textureU.y == Catch::Approx(1.0f).margin(0.0001));
    CHECK(first.textureV.z == Catch::Approx(-1.0f).margin(0.0001));
}
