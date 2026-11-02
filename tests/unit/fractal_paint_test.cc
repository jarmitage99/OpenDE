#include "compiler/asset_package.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/fractal_paint.h"
#include "engine/mesh.h"
#include "engine/props_fire.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace {
    dx::UObject* newTexture(const std::string& packageName, const std::string& name, const std::string& className)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        std::vector<std::string> log;
        REQUIRE(dx::compiler::runAssetCommand(package, {}, "TEXTURE NEW NAME=" + name + " CLASS=" + className + " PACKAGE=" + packageName + " USIZE=64 VSIZE=64", log));
        dx::UObject* texture = dx::objects::findObject(packageName + "." + name, nullptr);
        REQUIRE(texture != nullptr);
        return texture;
    }

    std::vector<int> sparkBytes(const dx::FireSpark& spark)
    {
        return { spark.type, spark.heat, spark.x, spark.y, spark.a, spark.b, spark.c, spark.d };
    }

    std::vector<int> dropBytes(dx::UObject* texture, int index)
    {
        const uint8_t* drop = dx::watertexture::Drops(texture) + index * 8;
        return std::vector<int>(drop, drop + 8);
    }
}

TEST_CASE("fire painting adds, lines, erases and closes sparks like UnrealEd", "[data]")
{
    dx::UObject* object = newTexture("PaintFireTest", "Torch", "FireTexture");
    dx::UFireTexture* fire = dx::cast<dx::UFireTexture>(object);
    REQUIRE(fire != nullptr);
    REQUIRE(dx::fractalpaint::paintable(fire));
    dx::fractalpaint::PaintState state;
    dx::firetexture::SparkType(fire) = 4;
    dx::fractalpaint::paint(fire, state, 10, 20, true, false);
    dx::fractalpaint::paint(fire, state, 11, 20, true, false);
    dx::fractalpaint::paint(fire, state, 11, 20, false, false);
    REQUIRE(dx::firetexture::NumSparks(fire) == 2);
    CHECK(sparkBytes(fire->sparks[0]) == std::vector<int>({ 4, 255, 10, 20, 0, 0, 0, 0 }));
    CHECK(sparkBytes(fire->sparks[1]) == std::vector<int>({ 4, 255, 11, 20, 0, 0, 0, 0 }));

    dx::firetexture::SparkType(fire) = 0;
    dx::fractalpaint::paint(fire, state, 0, 0, true, false);
    dx::fractalpaint::paint(fire, state, 5, 0, true, false);
    dx::fractalpaint::paint(fire, state, 5, 0, false, false);
    REQUIRE(dx::firetexture::NumSparks(fire) == 8);
    std::vector<int> xs;
    for (size_t i = 2; i < 8; i++) {
        CHECK(fire->sparks[i].y == 0);
        xs.push_back(fire->sparks[i].x);
    }
    CHECK(xs == std::vector<int>({ 0, 0, 1, 2, 3, 4 }));

    dx::fractalpaint::paint(fire, state, 10, 20, false, true);
    dx::fractalpaint::paint(fire, state, 10, 20, false, false);
    CHECK(dx::firetexture::NumSparks(fire) == 6);
    CHECK(fire->sparks.size() == 6);
    for (const dx::FireSpark& spark : fire->sparks) {
        CHECK(spark.y == 0);
    }

    dx::firetexture::SparkType(fire) = 13;
    dx::fractalpaint::addSpark(fire, state, 30, 40);
    CHECK(sparkBytes(fire->sparks[6]) == std::vector<int>({ 13, 255, 30, 40, 2, 241, 0, 2 }));

    dx::firetexture::SparkType(fire) = 23;
    dx::fractalpaint::paint(fire, state, 5, 5, true, false);
    REQUIRE(dx::firetexture::NumSparks(fire) == 8);
    CHECK(sparkBytes(fire->sparks[7]) == std::vector<int>({ 23, 0, 5, 5, 0, 0, 0, 0 }));
    dx::fractalpaint::paint(fire, state, 15, 9, true, false);
    REQUIRE(dx::firetexture::NumSparks(fire) == 8);
    CHECK(sparkBytes(fire->sparks[7]) == std::vector<int>({ 23, 255, 5, 5, 10, 4, 0, 0 }));
    dx::fractalpaint::paint(fire, state, 15, 9, false, false);
    CHECK(fire->sparks[7].d == 16);

    dx::fractalpaint::addSpark(fire, state, 64, 10);
    dx::fractalpaint::addSpark(fire, state, -1, 10);
    CHECK(dx::firetexture::NumSparks(fire) == 8);

    std::filesystem::path file = dx::config::userDirectory() / "paint-test" / "PaintFireTest.utx";
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(dx::objects::packageObject(dx::Name("PaintFireTest")), file, &error));
    for (dx::UObject* item : dx::compiler::packageContents(dx::objects::packageObject(dx::Name("PaintFireTest")))) {
        item->destroyed = true;
    }
    dx::Linker* linker = dx::objects::loadPackageFile(dx::Name("PaintFireTest"), file, false);
    dx::UFireTexture* loaded = dx::cast<dx::UFireTexture>(linker->object(linker->findExport(dx::Name("Torch"), dx::Name("FireTexture"), "*")));
    REQUIRE(loaded != nullptr);
    dx::objects::ensureLoaded(loaded);
    dx::objects::flushLoads();
    CHECK(dx::firetexture::NumSparks(loaded) == 8);
    REQUIRE(loaded->sparks.size() == 8);
    CHECK(sparkBytes(loaded->sparks[6]) == std::vector<int>({ 13, 255, 30, 40, 2, 241, 0, 2 }));
    CHECK(sparkBytes(loaded->sparks[7]) == std::vector<int>({ 23, 255, 5, 5, 10, 4, 0, 16 }));
}

TEST_CASE("water painting adds drops with their phase and erases them", "[data]")
{
    dx::UObject* water = newTexture("PaintWaterTest", "Pool", "WaveTexture");
    REQUIRE(dx::fractalpaint::paintable(water));
    dx::fractalpaint::PaintState state;
    dx::watertexture::DropType(water) = 1;
    dx::fractalpaint::paint(water, state, 20, 31, true, false);
    REQUIRE(dx::watertexture::NumDrops(water) == 1);
    CHECK(dropBytes(water, 0) == std::vector<int>({ 1, 8, 10, 15, 0, 0, 0, 8 }));
    dx::fractalpaint::paint(water, state, 22, 31, true, false);
    CHECK(dx::watertexture::NumDrops(water) == 2);
    dx::fractalpaint::paint(water, state, 22, 31, false, false);

    dx::watertexture::DropType(water) = 8;
    dx::fractalpaint::paint(water, state, 50, 50, true, false);
    dx::fractalpaint::paint(water, state, 54, 50, true, false);
    dx::fractalpaint::paint(water, state, 54, 50, false, false);
    REQUIRE(dx::watertexture::NumDrops(water) == 3);
    CHECK(dropBytes(water, 2) == std::vector<int>({ 8, 255, 25, 25, 0, 0, 0, 30 }));

    dx::fractalpaint::paint(water, state, 21, 30, false, true);
    CHECK(dx::watertexture::NumDrops(water) == 1);
    CHECK(dropBytes(water, 0)[0] == 8);
}
