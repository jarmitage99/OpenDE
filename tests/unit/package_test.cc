#include "engine/package.h"
#include "core/objects.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <vector>

TEST_CASE("compact index decodes single and multi byte values")
{
    std::vector<uint8_t> bytes = { 0x05, 0x85, 0x40, 0x01, 0x7f, 0x7f, 0x40, 0x80, 0x02 };
    dx::Reader reader(bytes, 0);
    CHECK(reader.index() == 5);
    CHECK(reader.index() == -5);
    CHECK(reader.index() == 64);
    CHECK(reader.index() == 8191);
    CHECK(reader.index() == 16384);
    CHECK(reader.tell() == bytes.size());
}

TEST_CASE("reader throws when reading past the end")
{
    std::vector<uint8_t> bytes = { 0x01, 0x02 };
    dx::Reader reader(bytes, 0);
    CHECK_THROWS_AS(reader.u32(), std::runtime_error);
}

TEST_CASE("package header and tables of Engine.u", "[data]")
{
    dx::Package package(DEUSEX_GAME_DATA "/System/Engine.u");
    CHECK(package.version() == 68);
    CHECK(package.licensee() == 0);
    CHECK(package.names().size() == 3574);
    CHECK(package.exports().size() == 5622);
    CHECK(package.imports().size() == 60);
}

TEST_CASE("tagged properties of a texture", "[data]")
{
    dx::Package package(DEUSEX_GAME_DATA "/Textures/CoreTexBrick.utx");
    int32_t ref = package.findExport("Texture", "rough_greywall");
    REQUIRE(ref != 0);
    CHECK(package.groupOf(ref) == "Brick");

    dx::Object object = package.load(ref);
    CHECK(object.objectValue("Palette") == 3);
    CHECK(object.byteValue("UBits") == 7);
    CHECK(object.byteValue("VBits") == 7);
    CHECK(object.intValue("USize") == 128);
    CHECK(object.intValue("VSize") == 128);
    REQUIRE(object.find("InternalTime", 1) != nullptr);
    CHECK(object.find("InternalTime", 2) == nullptr);
    CHECK(object.find("MipZero")->structName == "Color");
    CHECK(package.objectName(3) == "Palette123");
    CHECK(package.classOf(3) == "Palette");
}

namespace {
    int32_t findImport(dx::Linker& linker, const std::string& name)
    {
        const dx::Package& file = linker.file();
        for (size_t i = 0; i < file.imports().size(); i++) {
            int32_t ref = -static_cast<int32_t>(i + 1);
            if (file.objectName(ref) == name) {
                return ref;
            }
        }
        return 0;
    }
}

TEST_CASE("imports resolve to exports of other packages", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::Linker* map = dx::objects::loadPackage(dx::Name("01_NYC_UNATCOHQ"));
    REQUIRE(map != nullptr);
    int32_t import = findImport(*map, "bluewater");
    REQUIRE(import != 0);
    CHECK(map->file().groupOf(import) == "water");

    dx::UObject* resolved = map->object(import);
    REQUIRE(resolved != nullptr);
    CHECK(resolved->outermost()->name == dx::Name("Effects"));
    CHECK(resolved->name == dx::Name("bluewater"));
    CHECK(resolved->cls->name == dx::Name("Texture"));
}

TEST_CASE("imports from version 61 packages resolve", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::Linker* map = dx::objects::loadPackage(dx::Name("03_NYC_UNATCOHQ"));
    REQUIRE(map != nullptr);
    int32_t import = findImport(*map, "bluewater");
    REQUIRE(import != 0);
    dx::UObject* resolved = map->object(import);
    REQUIRE(resolved != nullptr);
    CHECK(resolved->outermost()->name == dx::Name("CoreTexWater"));
}
