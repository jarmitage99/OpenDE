#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/reader.h"
#include "engine/world.h"
#include "render/lighting.h"
#include "render/lightmap.h"

#include <catch_amalgamated.hpp>

namespace {
    void initializeEngine()
    {
        static bool initialized = false;
        if (!initialized) {
            dx::engine::initialize(DEUSEX_GAME_DATA);
            initialized = true;
        }
    }

    dx::ULodMesh* loadMesh(const char* package, const char* name)
    {
        initializeEngine();
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(package));
        REQUIRE(linker != nullptr);
        int32_t ref = linker->findExport(dx::Name(name), dx::Name("LodMesh"), "*");
        REQUIRE(ref != 0);
        dx::UObject* object = linker->object(ref);
        dx::objects::ensureLoaded(object);
        return dx::cast<dx::ULodMesh>(object);
    }
}

TEST_CASE("LodMesh Pan1 fields match the package bytes", "[data]")
{
    dx::ULodMesh* mesh = loadMesh("DeusExDeco", "Pan1");
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->verts.size() == 25);
    CHECK(mesh->verts[0].x == 2540.0f);
    CHECK(mesh->verts[0].y == 1166.0f);
    CHECK(mesh->verts[0].z == 356.0f);
    CHECK(mesh->verts[1].x == 0.0f);
    CHECK(mesh->verts[1].y == 3706.0f);
    CHECK(mesh->tris.empty());
    REQUIRE(mesh->sequences.size() == 2);
    CHECK(mesh->sequences[0].name == dx::Name("All"));
    CHECK(mesh->sequences[0].startFrame == 0);
    CHECK(mesh->sequences[0].numFrames == 1);
    CHECK(mesh->sequences[0].rate == 30.0f);
    CHECK(mesh->sequences[1].name == dx::Name("Still"));
    CHECK(mesh->findSequence(dx::Name("Still")) == &mesh->sequences[1]);
    CHECK(mesh->findSequence(dx::Name("Missing")) == nullptr);
    CHECK(mesh->frameVerts == 25);
    CHECK(mesh->animFrames == 1);
    CHECK(mesh->scale.x == 0.00390625f);
    CHECK(mesh->meshBounds.min.x == -2540.0f);
    CHECK(mesh->meshBounds.min.y == -3706.0f);
    CHECK(mesh->meshBounds.min.z == -412.0f);
    CHECK(mesh->textures.size() == 1);
}

TEST_CASE("mesh pose applies origin and scale", "[data]")
{
    dx::ULodMesh* mesh = loadMesh("DeusExDeco", "Pan1");
    REQUIRE(mesh != nullptr);
    std::vector<dx::Vector> pose;
    mesh->framePose(dx::Name("All"), 0.0f, pose);
    REQUIRE(pose.size() == 25);
    CHECK(pose[0].x == Catch::Approx(9.921875f));
    CHECK(pose[0].y == Catch::Approx(4.5546875f));
    CHECK(pose[0].z == Catch::Approx(1.390625f));
}

TEST_CASE("every LodMesh in the character, decoration and item packages parses to its export end", "[data]")
{
    initializeEngine();
    const char* packages[] = { "DeusExCharacters", "DeusExDeco", "DeusExItems" };
    size_t meshes = 0;
    for (const char* package : packages) {
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(package));
        REQUIRE(linker != nullptr);
        const dx::Package& file = linker->file();
        for (size_t i = 0; i < file.exports().size(); i++) {
            int32_t ref = static_cast<int32_t>(i + 1);
            if (file.classOf(ref) != "LodMesh") {
                continue;
            }
            const dx::ExportEntry& entry = file.exportAt(ref);
            dx::Reader reader(file.data(), static_cast<size_t>(entry.offset));
            dx::objects::readTaggedProperties(nullptr, nullptr, reader, *linker);
            dx::ULodMesh mesh;
            mesh.serialize(reader, *linker);
            INFO(file.objectName(ref));
            CHECK(reader.tell() == static_cast<size_t>(entry.offset + entry.size));
            meshes++;
        }
    }
    CHECK(meshes == 431);
}

TEST_CASE("Unreal hue saturation brightness conversion")
{
    dx::Vector white = dx::fGetHSV(0, 255, 255);
    CHECK(white.x == Catch::Approx(0.82128f).margin(0.0005));
    CHECK(white.y == Catch::Approx(0.82128f).margin(0.0005));
    CHECK(white.z == Catch::Approx(0.82128f).margin(0.0005));
    dx::Vector red = dx::fGetHSV(0, 0, 255);
    CHECK(red.x == Catch::Approx(0.82128f).margin(0.0005));
    CHECK(red.y == Catch::Approx(0.0f).margin(0.0005));
    CHECK(red.z == Catch::Approx(0.0f).margin(0.0005));
    dx::Vector black = dx::fGetHSV(40, 0, 0);
    CHECK(black.x == 0.0f);
}

TEST_CASE("light distance falloff")
{
    CHECK(dx::lightDistanceFalloff(0.25f) == Catch::Approx(0.5f).margin(0.0005));
    CHECK(dx::lightDistanceFalloff(0.81f) == Catch::Approx(0.0281f).margin(0.0005));
    CHECK(dx::lightDistanceFalloff(0.0001f) == Catch::Approx(0.99927f).margin(0.0001));
    CHECK(dx::lightDistanceFalloff(1.0f) == Catch::Approx(0.0f).margin(0.000001));
    CHECK(dx::lightDistanceFalloff(1.001f) == 0.0f);
}

TEST_CASE("lightmap texels lie on their surface plane", "[data]")
{
    initializeEngine();
    dx::Linker* linker = dx::objects::loadPackage(dx::Name("00_Training"));
    REQUIRE(linker != nullptr);
    dx::ULevel* level = dx::cast<dx::ULevel>(linker->object(linker->findExport(dx::Name("MyLevel"), dx::Name("Level"), "*")));
    REQUIRE(level != nullptr);
    dx::objects::ensureLoaded(level);
    dx::UModel* model = level->model;
    dx::objects::ensureLoaded(model);
    REQUIRE(model != nullptr);
    size_t checked = 0;
    for (const dx::BspSurf& surf : model->surfs) {
        if (surf.lightMap < 0 || checked >= 50) {
            continue;
        }
        const dx::LightMapIndex& index = model->lightMaps[static_cast<size_t>(surf.lightMap)];
        dx::Vector normal = model->vectors[static_cast<size_t>(surf.vNormal)];
        dx::Vector base = model->points[static_cast<size_t>(surf.pBase)];
        dx::Vector corner = dx::lightmapTexelPoint(model, surf, index.uClamp - 1, index.vClamp - 1);
        CHECK(normal.dot(corner - base) == Catch::Approx(0.0f).margin(0.05));
        checked++;
    }
    CHECK(checked == 50);
}
