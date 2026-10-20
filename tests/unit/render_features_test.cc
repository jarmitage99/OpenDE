#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "render/lighting.h"
#include "render/scene_renderer.h"

#include <catch_amalgamated.hpp>

TEST_CASE("volumetric fog integrates the light sphere along the view ray")
{
    dx::VolumetricLight light = dx::prepareVolumetric({ 0, 0, 100 }, 3, 1.0f, 8);
    CHECK(light.radius == 100.0f);
    CHECK(light.brightness == 0.125f);
    CHECK_FALSE(light.inside);
    CHECK(dx::volumetricFog(light, { 0, 0, 300 }) == Catch::Approx(0.5f).margin(0.002));
    CHECK(dx::volumetricFog(light, { 0, 0, 100 }) == Catch::Approx(0.25f).margin(0.002));
    CHECK(dx::volumetricFog(light, { 300, 0, 0 }) == 0.0f);
    CHECK(dx::volumetricFog(light, { 0, 0, -50 }) == 0.0f);
    dx::VolumetricLight bright = dx::prepareVolumetric({ 0, 0, 100 }, 3, 1.0f, 64);
    CHECK(dx::volumetricFog(bright, { 0, 0, 300 }) == 1.0f);
}

TEST_CASE("fog maps of several volumetric lights merge like a screen blend in 7 bits")
{
    CHECK(dx::fogMerge(100, 50) == 111);
    CHECK(dx::fogMerge(0, 64) == 64);
    CHECK(dx::fogMerge(127, 127) == 127);
}

TEST_CASE("version 61 texture packages load names and mip data", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::Linker* linker = dx::objects::loadPackage(dx::Name("CoreTexDetail"));
    REQUIRE(linker != nullptr);
    CHECK(linker->file().version() == 61);
    int32_t ref = linker->findExport(dx::Name("DMetal_A"), dx::Name("Texture"), "Detail");
    REQUIRE(ref != 0);
    dx::UTexture* texture = dx::cast<dx::UTexture>(linker->object(ref));
    REQUIRE(texture != nullptr);
    dx::objects::ensureLoaded(texture);
    REQUIRE(texture->mips.size() == 1);
    CHECK(texture->mips[0].width == 128);
    CHECK(texture->mips[0].height == 128);
    CHECK(texture->mips[0].data.size() == 16384);
    std::vector<uint8_t> rgba = texture->rgba(0);
    REQUIRE(rgba.size() == 65536);
    CHECK(rgba[0] == 108);
    CHECK(rgba[3] == 255);
}

namespace {
    dx::UObject* meshDefaults(const char* className, dx::UMesh*& mesh)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UClass* cls = dx::objects::findClass(className);
        dx::objects::flushLoads();
        REQUIRE(cls != nullptr);
        mesh = dx::cast<dx::UMesh>(dx::actor::Mesh(cls->defaultObject));
        dx::objects::ensureLoaded(mesh);
        dx::objects::flushLoads();
        REQUIRE(mesh != nullptr);
        return cls->defaultObject;
    }

    std::string pathOf(dx::UObject* object)
    {
        return object ? object->pathName() : "None";
    }
}

TEST_CASE("environment mapped mesh triangles use the actor texture, not their skin slot", "[data]")
{
    dx::UMesh* mesh = nullptr;
    dx::UObject* logo = meshDefaults("DeusEx.EidosLogo", mesh);
    CHECK(pathOf(dx::meshSkin(logo, mesh, 0)) == "DeusExDeco.Skins.EidosLogoTex1");
    CHECK(pathOf(dx::meshEnvironmentMap(logo, mesh, nullptr, nullptr, nullptr)) == "DeusExDeco.Skins.DXLogoTex1");
}

TEST_CASE("mesh environment map falls back to zone, level and finally the last mesh texture", "[data]")
{
    dx::UMesh* mesh = nullptr;
    dx::UObject* helicopter = meshDefaults("DeusEx.AttackHelicopter", mesh);
    CHECK(pathOf(dx::meshEnvironmentMap(helicopter, mesh, nullptr, nullptr, nullptr)) == "DeusExDeco.Skins.ReflectionMapTex3");
    dx::UClass* zoneClass = dx::objects::findClass("Engine.ZoneInfo");
    dx::UClass* levelClass = dx::objects::findClass("Engine.LevelInfo");
    dx::UTexture* zoneMap = dx::cast<dx::UTexture>(dx::actor::Texture(zoneClass->defaultObject));
    dx::UTexture* levelMap = dx::cast<dx::UTexture>(dx::actor::Texture(dx::objects::findClass("Engine.Actor")->defaultObject));
    REQUIRE(zoneMap != nullptr);
    REQUIRE(levelMap != nullptr);
    dx::zoneinfo::EnvironmentMap(levelClass->defaultObject) = levelMap;
    CHECK(pathOf(dx::meshEnvironmentMap(helicopter, mesh, nullptr, levelClass->defaultObject, nullptr)) == "Engine.S_Actor");
    dx::zoneinfo::EnvironmentMap(zoneClass->defaultObject) = zoneMap;
    CHECK(pathOf(dx::meshEnvironmentMap(helicopter, mesh, zoneClass->defaultObject, levelClass->defaultObject, nullptr)) == "Engine.S_ZoneInfo");
    dx::zoneinfo::EnvironmentMap(zoneClass->defaultObject) = nullptr;
    dx::zoneinfo::EnvironmentMap(levelClass->defaultObject) = nullptr;
}

TEST_CASE("an empty mesh skin slot resolves to no texture like UMesh::GetTexture", "[data]")
{
    dx::UMesh* mesh = nullptr;
    dx::UObject* cop = meshDefaults("DeusEx.RiotCop", mesh);
    CHECK(dx::meshSkin(cop, mesh, 7) == nullptr);
    CHECK(pathOf(dx::meshSkin(cop, mesh, 6)) == "DeusExCharacters.Skins.RiotCopTex3");
    CHECK(pathOf(dx::meshEnvironmentMap(cop, mesh, nullptr, nullptr, nullptr)) == "DeusExCharacters.Skins.VisorTex1");
}

TEST_CASE("environment map coordinates come from the world space reflection vector")
{
    float u = -1;
    float v = -1;
    dx::environmentCoords({ 10, 0, 0 }, { -1, 0, 0 }, u, v);
    CHECK(u == Catch::Approx(0.0f).margin(1e-5));
    CHECK(v == Catch::Approx(0.5f).margin(1e-5));
    dx::environmentCoords({ 0, 0, -5 }, { 0, 0, 1 }, u, v);
    CHECK(u == Catch::Approx(0.5f).margin(1e-5));
    CHECK(v == Catch::Approx(0.5f).margin(1e-5));
    dx::environmentCoords({ 3, 3, 0 }, { 0, -1, 0 }, u, v);
    CHECK(u == Catch::Approx(0.853553f).margin(1e-5));
    CHECK(v == Catch::Approx(0.146447f).margin(1e-5));
}

TEST_CASE("the sky frame comes from the viewer zone's linked SkyZone, not the first sky actor", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("00_Intro"));
    {
        dx::World world;
        REQUIRE(world.loadMap("00_Intro"));
        world.tick(1.0f / 30.0f);
        dx::UObject* levelInfo = nullptr;
        dx::UObject* zone = nullptr;
        dx::UObject* firstSky = nullptr;
        for (dx::UObject* actor : world.actors) {
            if (!actor || actor->destroyed) {
                continue;
            }
            if (actor->name == dx::Name("LevelInfo0")) {
                levelInfo = actor;
            }
            if (actor->name == dx::Name("ZoneInfo1")) {
                zone = actor;
            }
            if (!firstSky && actor->isA(dx::Name("SkyZoneInfo"))) {
                firstSky = actor;
            }
        }
        REQUIRE(levelInfo != nullptr);
        REQUIRE(zone != nullptr);
        REQUIRE(firstSky != nullptr);
        CHECK(firstSky->name == dx::Name("SkyZoneInfo0"));
        CHECK(pathOf(dx::frameSkyZone(levelInfo)) == "00_Intro.SkyZoneInfo3");
        CHECK(pathOf(dx::frameSkyZone(zone)) == "00_Intro.SkyZoneInfo3");
        CHECK(dx::frameSkyZone(nullptr) == nullptr);
    }
    dx::objects::unloadPackage(dx::Name("00_Intro"));
}

TEST_CASE("only DT_Sprite and DT_SpriteAnimOnce actors are drawn as sprites")
{
    CHECK(dx::drawnAsSprite(1));
    CHECK(dx::drawnAsSprite(7));
    CHECK_FALSE(dx::drawnAsSprite(0));
    CHECK_FALSE(dx::drawnAsSprite(2));
    CHECK_FALSE(dx::drawnAsSprite(3));
    CHECK_FALSE(dx::drawnAsSprite(4));
    CHECK_FALSE(dx::drawnAsSprite(5));
    CHECK_FALSE(dx::drawnAsSprite(6));
}

TEST_CASE("sprite anim once frames, matrix mode textures and surface decals follow the original renderer", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOHQ"));
        world.tick(1.0f / 30.0f);

        dx::UTexture* first = dx::cast<dx::UTexture>(dx::objects::loadObject("Effects.water.WtrFall_A04", nullptr));
        REQUIRE(first != nullptr);
        dx::UObject* trail = world.spawn(dx::objects::findClass("DeusEx.SmokeTrail"), nullptr, dx::Name(), { -1985, 813, 598 }, {}, nullptr, true);
        REQUIRE(trail != nullptr);
        REQUIRE(dx::actor::LifeSpan(trail->cls->defaultObject) == 1.0f);
        auto frameAt = [&](float lifeSpan) {
            dx::actor::LifeSpan(trail) = lifeSpan;
            return dx::spriteAnimOnceFrame(trail, first)->name.str();
        };
        CHECK(frameAt(1.0f) == "WtrFall_A04");
        CHECK(frameAt(0.5f) == "WtrFall_A06");
        CHECK(frameAt(0.39f) == "WtrFall_A07");
        CHECK(frameAt(0.4f) == "WtrFall_A06");
        CHECK(frameAt(0.0f) == "WtrFall_A08");
        CHECK(frameAt(2.0f) == "WtrFall_A04");

        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        CHECK(dx::matrixTexture(player) == nullptr);
        dx::actor::Sprite(player) = first;
        CHECK(dx::matrixTexture(player) == first);
        dx::actor::Sprite(player) = nullptr;
        dx::actor::Sprite(trail) = first;
        CHECK(dx::matrixTexture(trail) == nullptr);

        dx::CheckResult floor;
        REQUIRE(world.bspLineCheck(floor, { -1985, 813, 598 }, { -1985, 813, 0 }, {}));
        REQUIRE(floor.normal.z == Catch::Approx(1.0f));
        dx::Vector spot = floor.location;
        dx::UObject* hole = world.spawn(dx::objects::findClass("DeusEx.BulletHole"), nullptr, dx::Name(), spot + dx::Vector { 0, 0, 8 }, { 16384, 0, 0 }, nullptr, true);
        REQUIRE(hole != nullptr);
        REQUIRE_FALSE(hole->destroyed);
        auto found = world.decals.pieces.find(hole);
        REQUIRE(found != world.decals.pieces.end());
        REQUIRE(found->second.size() == 1);
        const dx::DecalPiece& piece = found->second[0];
        const dx::BspSurf& surf = world.level->model->surfs[static_cast<size_t>(piece.surf)];
        CHECK(world.level->model->vectors[static_cast<size_t>(surf.vNormal)].z == Catch::Approx(1.0f));
        float radius = 2.2627417f;
        dx::Vector expected[4] = { { 0, radius, 0 }, { radius, 0, 0 }, { 0, -radius, 0 }, { -radius, 0, 0 } };
        float floorZ = world.level->model->points[static_cast<size_t>(surf.pBase)].z;
        for (int i = 0; i < 4; i++) {
            CHECK(piece.vertices[static_cast<size_t>(i)].x == Catch::Approx(spot.x + expected[i].x).margin(0.01));
            CHECK(piece.vertices[static_cast<size_t>(i)].y == Catch::Approx(spot.y + expected[i].y).margin(0.01));
            CHECK(piece.vertices[static_cast<size_t>(i)].z == Catch::Approx(floorZ).margin(0.01));
        }
        CHECK(dx::decalFindSurf(world.level->model, floor.item, spot) == piece.surf);
        std::vector<dx::Vector> clipped(piece.vertices.begin(), piece.vertices.end());
        dx::clipDecalToNode(world.level->model, floor.item, clipped);
        CHECK(clipped.size() >= 3);
        std::vector<dx::Vector> outside { spot + dx::Vector { 100000, 0, 0 }, spot + dx::Vector { 100010, 0, 0 }, spot + dx::Vector { 100010, 10, 0 } };
        dx::clipDecalToNode(world.level->model, floor.item, outside);
        CHECK(outside.empty());
        dx::detachDecal(world, hole);
        CHECK(world.decals.pieces.count(hole) == 0);
    }
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
}

TEST_CASE("pawn shadows attach to the floor surfaces through the AttachDecal native", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOHQ"));
        for (int i = 0; i < 60; i++) {
            world.tick(1.0f / 30.0f);
        }
        dx::UObject* playerShadow = nullptr;
        for (dx::UObject* actor : world.actors) {
            if (actor && !actor->destroyed && actor->isA(dx::Name("Shadow")) && dx::actor::Owner(actor) == world.player()) {
                playerShadow = actor;
            }
        }
        REQUIRE(playerShadow != nullptr);
        auto found = world.decals.pieces.find(playerShadow);
        REQUIRE(found != world.decals.pieces.end());
        REQUIRE(found->second.size() == 1);
        const dx::BspSurf& surf = world.level->model->surfs[static_cast<size_t>(found->second[0].surf)];
        CHECK(world.level->model->vectors[static_cast<size_t>(surf.vNormal)].z == Catch::Approx(1.0f));
        dx::Vector feet = dx::actor::Location(world.player());
        dx::Vector center = (found->second[0].vertices[0] + found->second[0].vertices[2]) * 0.5f;
        CHECK(center.x == Catch::Approx(feet.x).margin(0.01));
        CHECK(center.y == Catch::Approx(feet.y).margin(0.01));
    }
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
}
