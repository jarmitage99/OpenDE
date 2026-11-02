#include "core/objects.h"
#include "engine/actor.h"
#include "engine/csg.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/surface_select.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <set>
#include <string>
#include <vector>

namespace {
    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    struct Rooms {
        dx::ULevel* level = nullptr;
        dx::UObject* builder = nullptr;
        dx::UObject* roomA = nullptr;
        dx::UObject* roomB = nullptr;
        dx::UObject* roomC = nullptr;
        dx::UObject* block = nullptr;
    };

    dx::UObject* addCube(Rooms& rooms, float width, float depth, float height, const dx::Vector& location, const dx::Rotator& rotation, uint8_t csgOper)
    {
        dx::UPolys& polys = *dx::cast<dx::UModel>(dx::actor::Brush(rooms.builder))->polys;
        dx::csg::buildCube(polys, width, depth, height, nullptr);
        for (size_t i = 0; i < polys.polys.size(); i++) {
            polys.polys[i].itemName = dx::Name(i < 4 ? "Side" : "Cap");
        }
        dx::actor::Location(rooms.builder) = location;
        dx::actor::Rotation(rooms.builder) = rotation;
        return dx::csg::addBrush(rooms.level, rooms.builder, csgOper);
    }

    Rooms makeRooms(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        Rooms rooms;
        rooms.level = make<dx::ULevel>("Level", package);
        rooms.level->model = make<dx::UModel>("Model", package);
        rooms.builder = make<dx::UObject>("Brush", package);
        dx::UModel* builderModel = make<dx::UModel>("Model", package);
        builderModel->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(rooms.builder) = builderModel;
        rooms.level->actors.push_back(nullptr);
        rooms.level->actors.push_back(rooms.builder);
        rooms.roomA = addCube(rooms, 512.0f, 512.0f, 256.0f, { 0, 0, 0 }, { 0, 0, 0 }, dx::csg::CsgSubtract);
        rooms.roomB = addCube(rooms, 256.0f, 512.0f, 256.0f, { 384, 0, 0 }, { 0, 0, 0 }, dx::csg::CsgSubtract);
        rooms.roomC = addCube(rooms, 512.0f, 512.0f, 256.0f, { 1536, 0, 0 }, { 0, 0, 0 }, dx::csg::CsgSubtract);
        rooms.block = addCube(rooms, 64.0f, 64.0f, 64.0f, { 0, 0, 0 }, { 8192, 0, 0 }, dx::csg::CsgAdd);
        dx::actor::Group(rooms.roomA) = dx::Name("East");
        dx::actor::Group(rooms.roomB) = dx::Name("West");
        dx::actor::Group(rooms.roomC) = dx::Name("East");
        dx::actor::Group(rooms.block) = dx::Name("Props");
        dx::csg::rebuild(rooms.level);
        return rooms;
    }

    int32_t surfOf(const dx::UModel& model, const dx::UObject* owner, const dx::Vector& normal)
    {
        for (size_t i = 0; i < model.surfs.size(); i++) {
            if (model.surfs[i].actor == owner && model.vectors[static_cast<size_t>(model.surfs[i].vNormal)].dot(normal) > 0.99f) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    std::set<const dx::UObject*> ownersOf(const dx::UModel& model, const std::set<int32_t>& selection)
    {
        std::set<const dx::UObject*> owners;
        for (int32_t index : selection) {
            owners.insert(model.surfs[static_cast<size_t>(index)].actor);
        }
        return owners;
    }

    size_t surfCount(const dx::UModel& model, const dx::UObject* owner)
    {
        size_t count = 0;
        for (const dx::BspSurf& surf : model.surfs) {
            if (surf.actor == owner) {
                count++;
            }
        }
        return count;
    }
}

TEST_CASE("adjacent wall floor and all selections follow shared vertices of the room", "[data]")
{
    Rooms rooms = makeRooms("SurfSelectAdjacentTest");
    const dx::UModel& model = *rooms.level->model;
    int32_t floorA = surfOf(model, rooms.roomA, { 0, 0, 1 });
    int32_t ceilingA = surfOf(model, rooms.roomA, { 0, 0, -1 });
    int32_t ceilingB = surfOf(model, rooms.roomB, { 0, 0, -1 });
    int32_t floorB = surfOf(model, rooms.roomB, { 0, 0, 1 });
    int32_t floorC = surfOf(model, rooms.roomC, { 0, 0, 1 });
    int32_t wallA = surfOf(model, rooms.roomA, { 1, 0, 0 });
    REQUIRE(floorA >= 0);
    REQUIRE(ceilingA >= 0);
    REQUIRE(floorB >= 0);
    REQUIRE(floorC >= 0);
    REQUIRE(wallA >= 0);

    std::set<int32_t> walls { floorA };
    dx::editorops::selectAdjacent(model, walls, dx::editorops::Adjacent::Walls);
    CHECK(walls.count(floorA) == 1);
    CHECK(walls.count(ceilingA) == 0);
    CHECK(walls.count(floorB) == 0);
    CHECK(walls.count(surfOf(model, rooms.roomA, { 0, 1, 0 })) == 1);
    CHECK(walls.count(surfOf(model, rooms.roomA, { 0, -1, 0 })) == 1);
    CHECK(walls.count(wallA) == 1);
    CHECK(surfOf(model, rooms.roomA, { -1, 0, 0 }) == -1);
    CHECK(walls.count(surfOf(model, rooms.roomB, { -1, 0, 0 })) == 1);
    CHECK(walls.count(surfOf(model, rooms.roomC, { 1, 0, 0 })) == 0);
    CHECK(ownersOf(model, walls) == std::set<const dx::UObject*> { rooms.roomA, rooms.roomB });

    std::set<int32_t> floors { floorA };
    dx::editorops::selectAdjacent(model, floors, dx::editorops::Adjacent::Floors);
    CHECK(floors == std::set<int32_t> { floorA, floorB });

    std::set<int32_t> floorsAndCeilings { floorA, surfOf(model, rooms.roomA, { 0, 1, 0 }) };
    dx::editorops::selectAdjacent(model, floorsAndCeilings, dx::editorops::Adjacent::Floors);
    CHECK(floorsAndCeilings.count(ceilingA) == 1);
    CHECK(floorsAndCeilings.count(ceilingB) == 1);
    CHECK(floorsAndCeilings.count(floorC) == 0);

    std::set<int32_t> all { floorA };
    dx::editorops::selectAdjacent(model, all, dx::editorops::Adjacent::All);
    CHECK(all.size() == surfCount(model, rooms.roomA) + surfCount(model, rooms.roomB));
    CHECK(ownersOf(model, all) == std::set<const dx::UObject*> { rooms.roomA, rooms.roomB });
}

TEST_CASE("adjacent slants and coplanars select only the matching connected surfaces", "[data]")
{
    Rooms rooms = makeRooms("SurfSelectSlantTest");
    const dx::UModel& model = *rooms.level->model;
    REQUIRE(surfCount(model, rooms.block) == 6);
    int32_t side = surfOf(model, rooms.block, { 0, 1, 0 });
    int32_t otherSide = surfOf(model, rooms.block, { 0, -1, 0 });
    REQUIRE(side >= 0);
    REQUIRE(otherSide >= 0);
    std::set<int32_t> slantStart;
    for (size_t i = 0; i < model.surfs.size(); i++) {
        float z = model.vectors[static_cast<size_t>(model.surfs[i].vNormal)].z;
        if (model.surfs[i].actor == rooms.block && z > 0.7f && z < 0.72f) {
            slantStart.insert(static_cast<int32_t>(i));
            break;
        }
    }
    REQUIRE(slantStart.size() == 1);

    std::set<int32_t> slants = slantStart;
    dx::editorops::selectAdjacent(model, slants, dx::editorops::Adjacent::Slants);
    CHECK(slants.size() == 4);
    CHECK(slants.count(side) == 0);
    CHECK(slants.count(otherSide) == 0);
    CHECK(ownersOf(model, slants) == std::set<const dx::UObject*> { rooms.block });

    std::set<int32_t> sideWalls { side };
    dx::editorops::selectAdjacent(model, sideWalls, dx::editorops::Adjacent::Walls);
    CHECK(sideWalls == std::set<int32_t> { side });

    std::set<int32_t> blockAll { side };
    dx::editorops::selectAdjacent(model, blockAll, dx::editorops::Adjacent::All);
    CHECK(blockAll.size() == 6);
    CHECK(ownersOf(model, blockAll) == std::set<const dx::UObject*> { rooms.block });

    int32_t floorA = surfOf(model, rooms.roomA, { 0, 0, 1 });
    int32_t floorB = surfOf(model, rooms.roomB, { 0, 0, 1 });
    int32_t floorC = surfOf(model, rooms.roomC, { 0, 0, 1 });
    std::set<int32_t> coplanars { floorA };
    dx::editorops::selectAdjacent(model, coplanars, dx::editorops::Adjacent::Coplanars);
    CHECK(coplanars == std::set<int32_t> { floorA, floorB });
    CHECK(coplanars.count(floorC) == 0);
}

TEST_CASE("matching group brush item and texture selections", "[data]")
{
    Rooms rooms = makeRooms("SurfSelectMatchingTest");
    dx::UModel& model = *rooms.level->model;
    int32_t floorA = surfOf(model, rooms.roomA, { 0, 0, 1 });
    int32_t ceilingA = surfOf(model, rooms.roomA, { 0, 0, -1 });
    int32_t floorB = surfOf(model, rooms.roomB, { 0, 0, 1 });
    int32_t ceilingB = surfOf(model, rooms.roomB, { 0, 0, -1 });
    REQUIRE(floorA >= 0);
    REQUIRE(ceilingB >= 0);

    std::set<int32_t> groups { floorA };
    dx::editorops::selectMatchingGroups(model, groups);
    CHECK(groups.size() == surfCount(model, rooms.roomA) + surfCount(model, rooms.roomC));
    CHECK(ownersOf(model, groups) == std::set<const dx::UObject*> { rooms.roomA, rooms.roomC });

    std::set<int32_t> brush { floorA };
    dx::editorops::selectMatchingBrush(model, brush);
    CHECK(brush.size() == surfCount(model, rooms.roomA));
    CHECK(ownersOf(model, brush) == std::set<const dx::UObject*> { rooms.roomA });

    std::set<int32_t> items { floorA };
    dx::editorops::selectMatchingItems(model, items);
    CHECK(items == std::set<int32_t> { floorA, ceilingA });

    std::set<int32_t> twoItems { floorA, floorB };
    dx::editorops::selectMatchingItems(model, twoItems);
    CHECK(twoItems == std::set<int32_t> { floorA, ceilingA, floorB, ceilingB });

    dx::UObject* brick = dx::objects::loadObject("CoreTexBrick.Brick.rough_greywall", nullptr);
    dx::objects::flushLoads();
    REQUIRE(brick != nullptr);
    model.surfs[static_cast<size_t>(floorA)].texture = brick;
    model.surfs[static_cast<size_t>(ceilingB)].texture = brick;
    std::set<int32_t> textures { floorA };
    dx::editorops::selectMatchingTexture(model, textures);
    CHECK(textures == std::set<int32_t> { floorA, ceilingB });
    CHECK(dx::editorops::surfsUsingTexture(model, brick) == std::set<int32_t> { floorA, ceilingB });
    std::set<int32_t> blank { ceilingA };
    dx::editorops::selectMatchingTexture(model, blank);
    CHECK(blank.size() == model.surfs.size() - 2);
    CHECK(blank.count(floorA) == 0);
}

TEST_CASE("select all reverse and memory combinations", "[data]")
{
    Rooms rooms = makeRooms("SurfSelectMemoryTest");
    const dx::UModel& model = *rooms.level->model;
    REQUIRE(model.surfs.size() > 6);
    std::set<int32_t> all;
    dx::editorops::selectAllSurfs(model, all);
    CHECK(all.size() == model.surfs.size());
    std::set<int32_t> reversed { 0, 2 };
    dx::editorops::selectReverse(model, reversed);
    CHECK(reversed.size() == model.surfs.size() - 2);
    CHECK(reversed.count(0) == 0);
    CHECK(reversed.count(1) == 1);
    CHECK(reversed.count(2) == 0);
    std::set<int32_t> memory { 1, 2, 3 };
    std::set<int32_t> orSet { 3, 4 };
    dx::editorops::combineWithMemory(orSet, memory, dx::editorops::MemoryOp::Or);
    CHECK(orSet == std::set<int32_t> { 1, 2, 3, 4 });
    std::set<int32_t> andSet { 3, 4 };
    dx::editorops::combineWithMemory(andSet, memory, dx::editorops::MemoryOp::And);
    CHECK(andSet == std::set<int32_t> { 3 });
    std::set<int32_t> xorSet { 3, 4 };
    dx::editorops::combineWithMemory(xorSet, memory, dx::editorops::MemoryOp::Xor);
    CHECK(xorSet == std::set<int32_t> { 1, 2, 4 });
}

TEST_CASE("textures report their group inside the package and their format", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* brick = dx::objects::loadObject("CoreTexBrick.Brick.rough_greywall", nullptr);
    dx::objects::flushLoads();
    REQUIRE(brick != nullptr);
    CHECK(dx::editorops::textureGroup(brick) == "Brick");
    dx::UTexture* texture = dx::cast<dx::UTexture>(brick);
    REQUIRE(texture != nullptr);
    CHECK(dx::editorops::textureFormatName(texture->format()) == "P8");
    CHECK(dx::editorops::textureFormatName(3) == "DXT1");
    dx::Linker* linker = dx::objects::loadPackage(dx::Name("CoreTexMetal"));
    REQUIRE(linker != nullptr);
    std::vector<dx::UObject*> textures;
    for (size_t i = 0; i < linker->file().exports().size(); i++) {
        dx::UObject* object = linker->object(static_cast<int32_t>(i + 1));
        if (object && object->cls && object->isA(dx::Name("Texture"))) {
            textures.push_back(object);
        }
    }
    REQUIRE(textures.size() > 1);
    std::vector<std::string> groups = dx::editorops::textureGroups(textures);
    CHECK(groups == std::vector<std::string> { "", "Ladder", "Metal" });
}
