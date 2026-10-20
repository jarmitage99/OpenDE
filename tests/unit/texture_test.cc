#include "core/objects.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

namespace {
    dx::UTexture* loadTexture(const char* path)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* object = dx::objects::loadObject(path, nullptr);
        return dx::cast<dx::UTexture>(object);
    }
}

TEST_CASE("P8 texture loads mips and palette", "[data]")
{
    dx::UTexture* texture = loadTexture("CoreTexBrick.Brick.rough_greywall");
    REQUIRE(texture != nullptr);
    CHECK(texture->format() == 0);
    REQUIRE(texture->mips.size() == 8);
    CHECK(texture->mips[0].width == 128);
    CHECK(texture->mips[0].height == 128);
    CHECK(texture->mips[0].data.size() == 16384);
    CHECK(texture->mips[0].data[0] == 0xe7);
    CHECK(texture->mips[7].width == 1);
    REQUIRE(texture->palette() != nullptr);
    REQUIRE(texture->palette()->colors.size() == 256);

    std::vector<uint8_t> rgba = texture->rgba(0);
    REQUIRE(rgba.size() == 128 * 128 * 4);
    CHECK(rgba[0] == 0xa7);
    CHECK(rgba[1] == 0xb5);
    CHECK(rgba[2] == 0xa8);
    CHECK(rgba[3] == 0xff);
}

TEST_CASE("procedural textures have no stored pixels", "[data]")
{
    dx::UTexture* texture = loadTexture("Effects.water.bluewater_a");
    REQUIRE(texture != nullptr);
    CHECK(texture->cls->name == dx::Name("WetTexture"));
    CHECK_FALSE(texture->decodable());
}
