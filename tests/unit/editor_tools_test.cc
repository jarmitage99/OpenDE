#include "core/objects.h"
#include "engine/actor.h"
#include "engine/brush_builder.h"
#include "engine/csg.h"
#include "engine/editor_history.h"
#include "engine/editor_ops.h"
#include "engine/lightmap_builder.h"
#include "engine/map_writer.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "render/lightmap.h"

#include <catch_amalgamated.hpp>

#include <filesystem>
#include <set>
#include <string>

namespace {
    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    struct Room {
        dx::ULevel* level = nullptr;
        dx::UObject* builder = nullptr;
        dx::UObject* light = nullptr;
    };

    Room makeRoom(const char* packageName, bool pillar)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        Room room;
        room.level = make<dx::ULevel>("Level", package);
        room.level->model = make<dx::UModel>("Model", package);
        room.builder = make<dx::UObject>("Brush", package);
        dx::UModel* builderModel = make<dx::UModel>("Model", package);
        builderModel->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(room.builder) = builderModel;
        room.level->actors.push_back(nullptr);
        room.level->actors.push_back(room.builder);
        dx::csg::buildCube(*builderModel->polys, 512.0f, 512.0f, 256.0f, nullptr);
        dx::csg::addBrush(room.level, room.builder, dx::csg::CsgSubtract);
        if (pillar) {
            dx::csg::buildCube(*builderModel->polys, 64.0f, 64.0f, 256.0f, nullptr);
            dx::actor::Location(room.builder) = { 128.0f, 0.0f, 0.0f };
            dx::csg::addBrush(room.level, room.builder, dx::csg::CsgAdd);
        }
        room.light = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
        room.light->loaded = true;
        dx::actor::Location(room.light) = { 0.0f, 0.0f, 0.0f };
        dx::actor::LightType(room.light) = 1;
        dx::actor::LightBrightness(room.light) = 64;
        dx::actor::LightRadius(room.light) = 32;
        dx::actor::bStatic(room.light) = 1;
        room.level->actors.push_back(room.light);
        dx::csg::rebuild(room.level);
        return room;
    }

    const dx::BspSurf* floorSurf(const dx::UModel& model)
    {
        for (const dx::BspSurf& surf : model.surfs) {
            if (model.vectors[static_cast<size_t>(surf.vNormal)].z > 0.99f) {
                return &surf;
            }
        }
        return nullptr;
    }

    bool texelLit(const dx::UModel& model, const dx::LightMapIndex& index, int32_t x, int32_t y)
    {
        int32_t pitch = (index.uClamp + 7) / 8;
        uint8_t byte = model.lightBits[static_cast<size_t>(index.dataOffset + y * pitch + (x >> 3))];
        return (byte & (1 << (x & 7))) != 0;
    }
}

TEST_CASE("undo and redo restore an actor move", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("HistoryMoveTest"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    dx::UObject* actor = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    actor->loaded = true;
    level->actors.push_back(actor);
    dx::actor::Location(actor) = { 10.0f, 20.0f, 30.0f };
    dx::EditorHistory history;
    history.begin(level, { actor }, "Move");
    dx::actor::Location(actor) = { 99.0f, 20.0f, 30.0f };
    history.commit();
    REQUIRE(history.undo());
    CHECK(dx::actor::Location(actor).x == 10.0f);
    CHECK(history.nextRedo() == "Move");
    REQUIRE(history.redo());
    CHECK(dx::actor::Location(actor).x == 99.0f);
    CHECK_FALSE(history.redo());
}

TEST_CASE("undo brings back a deleted actor and redo removes it again", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("HistoryDeleteTest"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    dx::UObject* first = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    dx::UObject* second = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    level->actors = { first, second };
    dx::EditorHistory history;
    history.begin(level, { second }, "Delete");
    level->actors.pop_back();
    second->destroyed = true;
    history.commit();
    REQUIRE(history.undo());
    CHECK(level->actors.size() == 2);
    CHECK(level->actors[1] == second);
    CHECK_FALSE(second->destroyed);
    REQUIRE(history.redo());
    CHECK(level->actors.size() == 1);
    CHECK(second->destroyed);
}

TEST_CASE("a light in an empty room lights the floor texels and not the edge texels inside the walls", "[data]")
{
    Room room = makeRoom("LightRoomTest", false);
    dx::lightbuild::Stats stats = dx::lightbuild::rebuild(room.level);
    const dx::UModel& model = *room.level->model;
    CHECK(stats.lights == 1);
    CHECK(stats.surfaces == 6);
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    REQUIRE(floor->lightMap >= 0);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    CHECK(index.uClamp == 16);
    CHECK(index.vClamp == 16);
    CHECK(index.uScale == Catch::Approx(34.15f).margin(0.0001));
    CHECK(index.pan.x == Catch::Approx(-0.125f).margin(0.0001));
    REQUIRE(index.lightActors >= 0);
    CHECK(model.lights[static_cast<size_t>(index.lightActors)] == room.light);
    CHECK(model.lights[static_cast<size_t>(index.lightActors) + 1] == nullptr);
    CHECK(model.lightBits[static_cast<size_t>(index.dataOffset)] == 0x00);
    CHECK(model.lightBits[static_cast<size_t>(index.dataOffset) + 1] == 0x00);
    CHECK(model.lightBits[static_cast<size_t>(index.dataOffset) + 2] == 0xfe);
    CHECK(model.lightBits[static_cast<size_t>(index.dataOffset) + 3] == 0x7f);
}

TEST_CASE("a pillar casts a shadow on the floor texels behind it", "[data]")
{
    Room room = makeRoom("LightPillarTest", true);
    dx::lightbuild::rebuild(room.level);
    dx::UModel& model = *room.level->model;
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    REQUIRE(floor->lightMap >= 0);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    REQUIRE(index.uClamp == 16);
    CHECK(texelLit(model, index, 2, 8));
    CHECK_FALSE(texelLit(model, index, 13, 8));
    CHECK_FALSE(texelLit(model, index, 14, 8));
    dx::Vector ambient { 0.1f, 0.1f, 0.1f };
    dx::LightmapTexels texels = dx::computeLightmap(&model, *floor, ambient);
    REQUIRE(texels.width == 16);
    const dx::Vector& shadowed = texels.colors[static_cast<size_t>(8 * 16 + 14)];
    const dx::Vector& lit = texels.colors[static_cast<size_t>(8 * 16 + 2)];
    CHECK(shadowed.x == Catch::Approx(0.0470588f).margin(0.000001));
    CHECK(shadowed.y == Catch::Approx(0.0470588f).margin(0.000001));
    CHECK(lit.x > 0.2f);
}

TEST_CASE("a non static light that cannot be deleted is raytraced like a static one", "[data]")
{
    Room room = makeRoom("LightNoDeleteTest", false);
    dx::actor::bStatic(room.light) = 0;
    dx::actor::bNoDelete(room.light) = 1;
    dx::lightbuild::Stats stats = dx::lightbuild::rebuild(room.level);
    const dx::UModel& model = *room.level->model;
    CHECK(stats.lights == 1);
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    REQUIRE(index.lightActors >= 0);
    CHECK(model.lights[static_cast<size_t>(index.lightActors)] == room.light);
    CHECK(texelLit(model, index, 8, 8));
}

TEST_CASE("an invisible brush between a light and the floor casts no shadow", "[data]")
{
    Room room = makeRoom("LightInvisibleTest", false);
    dx::UModel* builderModel = dx::cast<dx::UModel>(dx::actor::Brush(room.builder));
    dx::csg::buildCube(*builderModel->polys, 128.0f, 128.0f, 32.0f, nullptr);
    dx::actor::Location(room.builder) = { 0.0f, 0.0f, -64.0f };
    dx::csg::addBrush(room.level, room.builder, dx::csg::CsgAdd, dx::csg::PolyInvisible);
    dx::csg::rebuild(room.level);
    dx::lightbuild::rebuild(room.level);
    const dx::UModel& model = *room.level->model;
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    REQUIRE(floor->lightMap >= 0);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    REQUIRE(index.uClamp == 16);
    REQUIRE(index.lightActors >= 0);
    CHECK(texelLit(model, index, 7, 7));
    CHECK(texelLit(model, index, 8, 8));
}

TEST_CASE("geometry rebuild lists the lights permeating each leaf and lighting keeps the lists", "[data]")
{
    Room room = makeRoom("LightPermeateTest", false);
    dx::UModel& model = *room.level->model;
    REQUIRE(model.leaves.size() == 1);
    int32_t first = model.leaves[0].permeating;
    REQUIRE(first >= 0);
    CHECK(model.lights[static_cast<size_t>(first)] == room.light);
    CHECK(model.lights[static_cast<size_t>(first) + 1] == nullptr);
    dx::lightbuild::rebuild(room.level);
    int32_t kept = model.leaves[0].permeating;
    REQUIRE(kept >= 0);
    CHECK(model.lights[static_cast<size_t>(kept)] == room.light);
    CHECK(model.lights[static_cast<size_t>(kept) + 1] == nullptr);
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    CHECK(index.lightActors > kept);
}

TEST_CASE("a light behind a wall does not permeate the leaf on the other side", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("LightTwoRoomTest"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    level->model = make<dx::UModel>("Model", package);
    dx::UObject* builder = make<dx::UObject>("Brush", package);
    dx::UModel* builderModel = make<dx::UModel>("Model", package);
    builderModel->polys = make<dx::UPolys>("Polys", package);
    dx::actor::Brush(builder) = builderModel;
    level->actors = { nullptr, builder };
    dx::csg::buildCube(*builderModel->polys, 512.0f, 512.0f, 256.0f, nullptr);
    dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
    dx::actor::Location(builder) = { 0.0f, 640.0f, 0.0f };
    dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
    dx::UObject* light = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    light->loaded = true;
    dx::actor::LightType(light) = 1;
    dx::actor::LightRadius(light) = 64;
    dx::actor::bStatic(light) = 1;
    level->actors.push_back(light);
    dx::csg::rebuild(level);
    const dx::UModel& model = *level->model;
    REQUIRE(model.leaves.size() == 2);
    int lit = 0;
    for (const dx::ConvexLeaf& leaf : model.leaves) {
        if (leaf.permeating >= 0) {
            lit++;
            CHECK(model.lights[static_cast<size_t>(leaf.permeating)] == light);
        }
    }
    CHECK(lit == 1);
}

TEST_CASE("a volumetric light in a fog zone is listed in the leaves within its volume radius", "[data]")
{
    Room room = makeRoom("LightVolumeTest", false);
    dx::UObject* zone = dx::objects::newObject(dx::objects::findClass("Engine.ZoneInfo"), room.light->outer, dx::Name(), 0);
    zone->loaded = true;
    dx::actor::Location(zone) = { 100.0f, 100.0f, 0.0f };
    dx::zoneinfo::bFogZone(zone) = 1;
    room.level->actors.push_back(zone);
    dx::actor::VolumeRadius(room.light) = 8;
    dx::actor::VolumeBrightness(room.light) = 64;
    dx::csg::rebuild(room.level);
    const dx::UModel& model = *room.level->model;
    REQUIRE(model.leaves.size() == 1);
    REQUIRE(model.leaves[0].volumetric >= 0);
    CHECK(model.lights[static_cast<size_t>(model.leaves[0].volumetric)] == room.light);
    dx::zoneinfo::bFogZone(zone) = 0;
    dx::csg::rebuild(room.level);
    CHECK(room.level->model->leaves[0].volumetric == -1);
}

TEST_CASE("a surface the light cannot see gets no light list", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("LightHiddenSurfTest"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    level->model = make<dx::UModel>("Model", package);
    dx::UObject* builder = make<dx::UObject>("Brush", package);
    dx::UModel* builderModel = make<dx::UModel>("Model", package);
    builderModel->polys = make<dx::UPolys>("Polys", package);
    dx::actor::Brush(builder) = builderModel;
    level->actors = { nullptr, builder };
    dx::csg::buildCube(*builderModel->polys, 512.0f, 512.0f, 256.0f, nullptr);
    dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
    dx::actor::Location(builder) = { 1024.0f, 0.0f, 0.0f };
    dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
    dx::UObject* light = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    light->loaded = true;
    dx::actor::LightType(light) = 1;
    dx::actor::LightRadius(light) = 64;
    dx::actor::bStatic(light) = 1;
    level->actors.push_back(light);
    dx::csg::rebuild(level);
    dx::lightbuild::rebuild(level);
    const dx::UModel& model = *level->model;
    int litFloors = 0;
    int darkFloors = 0;
    for (const dx::BspNode& node : model.nodes) {
        const dx::BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
        if (node.numVertices < 3 || node.plane.z < 0.99f || surf.lightMap < 0) {
            continue;
        }
        const dx::Vector& corner = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool)].vertex)];
        const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(surf.lightMap)];
        if (corner.x < 512.0f) {
            CHECK(index.lightActors >= 0);
            litFloors++;
        } else {
            CHECK(index.lightActors == -1);
            darkFloors++;
        }
    }
    CHECK(litFloors == 1);
    CHECK(darkFloors == 1);
}

TEST_CASE("a mover gets its own light maps and shadows the floor below it", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("LightMoverTest"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    level->model = make<dx::UModel>("Model", package);
    dx::UObject* builder = make<dx::UObject>("Brush", package);
    dx::UModel* builderModel = make<dx::UModel>("Model", package);
    builderModel->polys = make<dx::UPolys>("Polys", package);
    dx::actor::Brush(builder) = builderModel;
    level->actors = { nullptr, builder };
    dx::csg::buildCube(*builderModel->polys, 1024.0f, 1024.0f, 512.0f, nullptr);
    dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
    dx::csg::buildCube(*builderModel->polys, 128.0f, 128.0f, 128.0f, nullptr);
    dx::actor::Location(builder) = { 0.0f, 0.0f, -150.0f };
    dx::UObject* mover = dx::csg::addMover(level, builder, nullptr);
    REQUIRE(mover != nullptr);
    CHECK(dx::mover::BasePos(mover).z == -150.0f);
    dx::UObject* light = dx::objects::newObject(dx::objects::findClass("Engine.Light"), package, dx::Name(), 0);
    light->loaded = true;
    dx::actor::Location(light) = { 0.0f, 0.0f, 100.0f };
    dx::actor::LightType(light) = 1;
    dx::actor::LightRadius(light) = 64;
    dx::actor::bStatic(light) = 1;
    level->actors.push_back(light);
    dx::csg::rebuild(level);
    dx::lightbuild::rebuild(level);
    const dx::UModel& model = *level->model;
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    REQUIRE(floor->lightMap >= 0);
    const dx::LightMapIndex& index = model.lightMaps[static_cast<size_t>(floor->lightMap)];
    REQUIRE(index.uClamp == 32);
    REQUIRE(index.lightActors >= 0);
    CHECK_FALSE(texelLit(model, index, 15, 15));
    CHECK(texelLit(model, index, 2, 15));
    dx::UModel& brush = *dx::cast<dx::UModel>(dx::actor::Brush(mover));
    REQUIRE(brush.lightMaps.size() == 6);
    int lit = 0;
    int unlit = 0;
    for (const dx::Poly& poly : brush.polys->polys) {
        REQUIRE(poly.brushPoly >= 0);
        const dx::LightMapIndex& moverIndex = brush.lightMaps[static_cast<size_t>(poly.brushPoly)];
        if (poly.normal.z > 0.99f) {
            REQUIRE(moverIndex.lightActors >= 0);
            CHECK(brush.lights[static_cast<size_t>(moverIndex.lightActors)] == light);
            CHECK(texelLit(brush, moverIndex, 1, 1));
            lit++;
        } else if (poly.normal.z < -0.99f) {
            CHECK(moverIndex.lightActors == -1);
            unlit++;
        }
    }
    CHECK(lit == 1);
    CHECK(unlit == 1);
    CHECK(dx::actor::Location(mover).z == -150.0f);
}

TEST_CASE("geometry rebuild attaches zone infos to their zones and sets actor regions", "[data]")
{
    Room room = makeRoom("ZoneInfoRegionTest", false);
    dx::UObject* zone = dx::objects::newObject(dx::objects::findClass("Engine.ZoneInfo"), room.light->outer, dx::Name(), 0);
    zone->loaded = true;
    dx::actor::Location(zone) = { 100.0f, 50.0f, 0.0f };
    room.level->actors.push_back(zone);
    dx::csg::rebuild(room.level);
    const dx::UModel& model = *room.level->model;
    CHECK(model.zones[1].zoneActor == zone);
    CHECK(dx::region(room.light).zone == zone);
    CHECK(dx::region(room.light).zoneNumber == 1);
    CHECK(dx::region(room.light).iLeaf == 0);
}

TEST_CASE("surface pan scale align and flag edits update the surface and its brush polygon", "[data]")
{
    Room room = makeRoom("SurfaceOpsTest", false);
    dx::UModel& model = *room.level->model;
    const dx::BspSurf* floor = floorSurf(model);
    REQUIRE(floor != nullptr);
    int32_t index = static_cast<int32_t>(floor - model.surfs.data());
    std::set<int32_t> selection { index };
    dx::UObject* roomBrush = model.surfs[static_cast<size_t>(index)].actor;
    REQUIRE(roomBrush != nullptr);
    dx::Poly& master = dx::cast<dx::UModel>(dx::actor::Brush(roomBrush))->polys->polys[static_cast<size_t>(model.surfs[static_cast<size_t>(index)].brushPoly)];
    dx::editorops::texPan(model, selection, 16, -8, false);
    CHECK(model.surfs[static_cast<size_t>(index)].panU == 16);
    CHECK(model.surfs[static_cast<size_t>(index)].panV == -8);
    CHECK(master.panU == 16);
    dx::editorops::texPan(model, selection, 4, 4, false);
    CHECK(model.surfs[static_cast<size_t>(index)].panU == 20);
    dx::editorops::texPan(model, selection, 0, 0, true);
    CHECK(master.panU == 0);
    model.surfs[static_cast<size_t>(index)].lightMap = 3;
    dx::editorops::texScale(model, selection, 0.5f, 0.0f, 0.0f, 0.5f, true);
    const dx::BspSurf& scaled = model.surfs[static_cast<size_t>(index)];
    CHECK(model.vectors[static_cast<size_t>(scaled.vTextureU)].size() == Catch::Approx(0.5f).margin(0.0001));
    CHECK(model.vectors[static_cast<size_t>(scaled.vTextureV)].size() == Catch::Approx(0.5f).margin(0.0001));
    CHECK(scaled.lightMap == -1);
    CHECK(master.textureU.size() == Catch::Approx(0.5f).margin(0.0001));
    dx::editorops::setAndClearFlags(model, selection, 0x00800000, 0);
    CHECK((model.surfs[static_cast<size_t>(index)].polyFlags & 0x00800000) != 0);
    CHECK((master.polyFlags & 0x00800000) != 0);
    dx::editorops::setAndClearFlags(model, selection, 0, 0x00800000);
    CHECK((master.polyFlags & 0x00800000) == 0);
    dx::UObject* texture = dx::objects::loadObject("CoreTexBrick.Brick.rough_greywall", nullptr);
    dx::objects::flushLoads();
    REQUIRE(texture != nullptr);
    dx::editorops::setTexture(model, selection, texture);
    CHECK(master.texture == texture);
    dx::editorops::texAlign(model, selection, dx::editorops::TexAlign::Floor);
    const dx::BspSurf& aligned = model.surfs[static_cast<size_t>(index)];
    CHECK(model.vectors[static_cast<size_t>(aligned.vTextureU)].x == 1.0f);
    CHECK(model.vectors[static_cast<size_t>(aligned.vTextureV)].y == 1.0f);
    CHECK(model.points[static_cast<size_t>(aligned.pBase)].z == -128.0f);
    CHECK(model.points[static_cast<size_t>(aligned.pBase)].x == 0.0f);
    CHECK(master.base.x == 0.0f);
    CHECK(master.base.z == -128.0f);
}

TEST_CASE("undo restores the level model after a brush is added", "[data]")
{
    Room room = makeRoom("HistoryModelTest", false);
    dx::UModel& model = *room.level->model;
    size_t nodes = model.nodes.size();
    size_t surfs = model.surfs.size();
    dx::EditorHistory history;
    dx::UModel* builderModel = dx::cast<dx::UModel>(dx::actor::Brush(room.builder));
    dx::csg::buildCube(*builderModel->polys, 64.0f, 64.0f, 64.0f, nullptr);
    history.begin(room.level, { &model }, "Add brush");
    dx::csg::addBrush(room.level, room.builder, dx::csg::CsgAdd);
    history.commit();
    REQUIRE(model.surfs.size() == surfs + 6);
    REQUIRE(history.undo());
    CHECK(model.nodes.size() == nodes);
    CHECK(model.surfs.size() == surfs);
    REQUIRE(history.redo());
    CHECK(model.surfs.size() == surfs + 6);
}

TEST_CASE("the original cube builder script builds the builder brush through the editor natives", "[data]")
{
    Room room = makeRoom("ScriptBuilderTest", false);
    dx::UClass* cube = nullptr;
    for (dx::UClass* cls : dx::brushbuilder::builderClasses()) {
        if (cls->name == dx::Name("CubeBuilder")) {
            cube = cls;
        }
    }
    REQUIRE(cube != nullptr);
    CHECK(dx::brushbuilder::builderClasses().size() == 5);
    dx::UObject* builder = dx::brushbuilder::builderFor(cube);
    REQUIRE(builder != nullptr);
    *reinterpret_cast<float*>(builder->propertyAddress(dx::Name("Height"))) = 300.0f;
    *reinterpret_cast<float*>(builder->propertyAddress(dx::Name("Width"))) = 200.0f;
    *reinterpret_cast<float*>(builder->propertyAddress(dx::Name("Breadth"))) = 100.0f;
    dx::actor::PrePivot(room.builder) = { 10.0f, 0.0f, 0.0f };
    dx::actor::Location(room.builder) = { 50.0f, 0.0f, 0.0f };
    dx::brushbuilder::setTarget(room.builder);
    REQUIRE(dx::brushbuilder::build(builder));
    dx::UModel& brush = *dx::cast<dx::UModel>(dx::actor::Brush(room.builder));
    REQUIRE(brush.polys->polys.size() == 6);
    const dx::Poly& first = brush.polys->polys[0];
    REQUIRE(first.vertices.size() == 4);
    CHECK(first.vertices[0].x == -150.0f);
    CHECK(first.vertices[0].y == -100.0f);
    CHECK(first.vertices[0].z == -50.0f);
    CHECK(first.normal.x == -1.0f);
    CHECK(brush.bounds.max.x == 150.0f);
    CHECK(dx::actor::Location(room.builder).x == 40.0f);
    CHECK(dx::actor::PrePivot(room.builder).x == 0.0f);
}

TEST_CASE("brush polygons export to and import from the UnrealEd t3d polylist format", "[data]")
{
    Room room = makeRoom("BrushTextTest", false);
    dx::UModel& brush = *dx::cast<dx::UModel>(dx::actor::Brush(room.builder));
    dx::Poly poly;
    poly.vertices = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 128.0f, 0.0f }, { 128.0f, 128.0f, 0.0f }, { 128.0f, 0.0f, 0.0f } };
    poly.base = { 0.0f, 0.0f, 0.0f };
    poly.normal = { 0.0f, 0.0f, -1.0f };
    poly.textureU = { 1.0f, 0.0f, 0.0f };
    poly.textureV = { 0.0f, -1.0f, 0.0f };
    poly.polyFlags = 0x20;
    poly.panU = 3;
    poly.link = 0;
    brush.polys->polys = { poly };
    std::string text = dx::editorops::exportPolys(*brush.polys);
    CHECK(text == "Begin PolyList\r\n"
                  "   Begin Polygon Flags=32 Link=0\r\n"
                  "      Origin   +00000.000000,+00000.000000,+00000.000000\r\n"
                  "      Normal   +00000.000000,+00000.000000,-00001.000000\r\n"
                  "      Pan      U=3 V=0\r\n"
                  "      TextureU +00001.000000,+00000.000000,+00000.000000\r\n"
                  "      TextureV +00000.000000,-00001.000000,+00000.000000\r\n"
                  "      Vertex   +00000.000000,+00000.000000,+00000.000000\r\n"
                  "      Vertex   +00000.000000,+00128.000000,+00000.000000\r\n"
                  "      Vertex   +00128.000000,+00128.000000,+00000.000000\r\n"
                  "      Vertex   +00128.000000,+00000.000000,+00000.000000\r\n"
                  "   End Polygon\r\n"
                  "End PolyList\r\n");
    brush.polys->polys.clear();
    dx::editorops::importPolys(*brush.polys, "Begin PolyList\n   Begin Polygon Flags=33554464 Link=0\n      Pan      U=3 V=7\n      Vertex   0,0,0\n      Vertex   0,128,0\n      Vertex   128,128,0\n      Vertex   128,0,0\n   End Polygon\n   Begin Polygon\n      Vertex   0,0,0\n      Vertex   1,0,0\n   End Polygon\nEnd PolyList\n");
    REQUIRE(brush.polys->polys.size() == 1);
    const dx::Poly& imported = brush.polys->polys[0];
    CHECK(imported.polyFlags == 0x20u);
    CHECK(imported.panU == 3);
    CHECK(imported.panV == 7);
    CHECK(imported.link == 0);
    CHECK(imported.vertices.size() == 4);
    CHECK(imported.base.x == 0.0f);
    CHECK(imported.normal.z == -1.0f);
}

TEST_CASE("a map with a mover saves and reloads the mover brush with its nodes and light maps", "[data]")
{
    Room room = makeRoom("MoverSaveTest", false);
    dx::UObject* info = dx::objects::newObject(dx::objects::findClass("Engine.LevelInfo"), room.level->outermost(), dx::Name(), 0);
    info->loaded = true;
    room.level->actors[0] = info;
    dx::UModel* builderModel = dx::cast<dx::UModel>(dx::actor::Brush(room.builder));
    dx::csg::buildCube(*builderModel->polys, 64.0f, 64.0f, 32.0f, nullptr);
    dx::actor::Location(room.builder) = { 0.0f, 0.0f, -100.0f };
    dx::UObject* mover = dx::csg::addMover(room.level, room.builder, nullptr);
    REQUIRE(mover != nullptr);
    dx::lightbuild::rebuild(room.level);
    dx::UModel& brush = *dx::cast<dx::UModel>(dx::actor::Brush(mover));
    REQUIRE_FALSE(brush.nodes.empty());
    REQUIRE(brush.lightMaps.size() == 6);
    std::string brushName = brush.name.str();
    size_t nodes = brush.nodes.size();
    std::filesystem::path file = std::filesystem::temp_directory_path() / "dx-mover-save-test" / "MoverSaved.dx";
    std::filesystem::remove_all(file.parent_path());
    std::string error;
    REQUIRE(dx::mapwriter::saveMap(room.level, file, &error));
    dx::Linker* saved = dx::objects::loadPackageFile(dx::Name("MoverSaved"), file, false);
    REQUIRE(saved != nullptr);
    int32_t ref = saved->findExport(dx::Name(brushName), dx::Name("Model"), "*");
    REQUIRE(ref > 0);
    dx::UModel* loaded = dx::cast<dx::UModel>(saved->object(ref));
    dx::objects::ensureLoaded(loaded);
    dx::objects::flushLoads();
    REQUIRE(loaded != nullptr);
    CHECK(loaded->nodes.size() == nodes);
    CHECK(loaded->lightMaps.size() == 6);
    CHECK(loaded->zones[0].zoneActor == nullptr);
    dx::objects::unloadPackage(dx::Name("MoverSaved"));
}
