#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/props_engine.h"
#include "engine/props_fire.h"
#include "engine/world.h"
#include "render/procedural.h"

#include <catch_amalgamated.hpp>

#include <vector>

namespace {
    dx::UTexture* loadEffect(const char* name, const char* className)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::Linker* linker = dx::objects::loadPackage(dx::Name("Effects"));
        REQUIRE(linker != nullptr);
        int32_t ref = linker->findExport(dx::Name(name), dx::Name(className), "*");
        REQUIRE(ref != 0);
        dx::UObject* object = linker->object(ref);
        dx::objects::ensureLoaded(object);
        dx::UTexture* texture = dx::cast<dx::UTexture>(object);
        REQUIRE(texture != nullptr);
        return texture;
    }

    struct SinglePulseFire {
        dx::UFireTexture* fire = nullptr;
        std::vector<dx::FireSpark> sparks;
        int32_t numSparks = 0;
        uint8_t renderHeat = 0;
        uint32_t rising = 0;
        uint8_t primeCount = 0;
        float maxFrameRate = 0;
        float minFrameRate = 0;

        SinglePulseFire(uint32_t risingFire, uint8_t prime, float maxRate, uint8_t sparkY)
        {
            fire = dx::cast<dx::UFireTexture>(loadEffect("LaserSpot2", "FireTexture"));
            REQUIRE(fire != nullptr);
            REQUIRE(fire->mips[0].width == 32);
            REQUIRE(fire->mips[0].height == 32);
            sparks = fire->sparks;
            numSparks = dx::firetexture::NumSparks(fire);
            renderHeat = dx::firetexture::RenderHeat(fire);
            rising = dx::firetexture::bRising(fire);
            primeCount = dx::texture::PrimeCount(fire);
            maxFrameRate = dx::texture::MaxFrameRate(fire);
            minFrameRate = dx::texture::MinFrameRate(fire);
            dx::FireSpark pulse;
            pulse.type = 2;
            pulse.heat = 200;
            pulse.x = 10;
            pulse.y = sparkY;
            fire->sparks = { pulse };
            dx::firetexture::NumSparks(fire) = 1;
            dx::firetexture::RenderHeat(fire) = 239;
            dx::firetexture::bRising(fire) = risingFire;
            dx::texture::PrimeCount(fire) = prime;
            dx::texture::MaxFrameRate(fire) = maxRate;
            dx::texture::MinFrameRate(fire) = 0;
        }

        ~SinglePulseFire()
        {
            fire->sparks = sparks;
            dx::firetexture::NumSparks(fire) = numSparks;
            dx::firetexture::RenderHeat(fire) = renderHeat;
            dx::firetexture::bRising(fire) = rising;
            dx::texture::PrimeCount(fire) = primeCount;
            dx::texture::MaxFrameRate(fire) = maxFrameRate;
            dx::texture::MinFrameRate(fire) = minFrameRate;
        }
    };

    struct ExpectedHeat {
        int32_t x;
        int32_t y;
        uint8_t index;
    };

    void checkIndices(dx::UTexture* texture, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height, const std::vector<ExpectedHeat>& expected)
    {
        REQUIRE(width == 32);
        REQUIRE(height == 32);
        REQUIRE(rgba.size() == 32 * 32 * 4);
        std::vector<uint8_t> indices(32 * 32, 0);
        for (const ExpectedHeat& heat : expected) {
            indices[static_cast<size_t>(heat.y * 32 + heat.x)] = heat.index;
        }
        const std::vector<uint32_t>& colors = texture->palette()->colors;
        for (size_t i = 0; i < indices.size(); i++) {
            uint32_t color = colors[indices[i]];
            INFO("pixel " << i % 32 << "," << i / 32 << " expected index " << static_cast<int>(indices[i]));
            CHECK(rgba[i * 4 + 0] == (color & 0xff));
            CHECK(rgba[i * 4 + 1] == ((color >> 8) & 0xff));
            CHECK(rgba[i * 4 + 2] == ((color >> 16) & 0xff));
            CHECK(rgba[i * 4 + 3] == (indices[i] == 0 ? 0 : 255));
        }
    }
}

TEST_CASE("slow fire spreads a pulse spark from the previous frame", "[data]")
{
    SinglePulseFire setup(0, 0, 0, 10);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(setup.fire, 1.0, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {
                                                      { 10, 9, 50 },
                                                      { 9, 10, 50 },
                                                      { 10, 10, 50 },
                                                      { 11, 10, 50 },
                                                  });
}

TEST_CASE("rising fire moves a pulse spark up two lines", "[data]")
{
    SinglePulseFire setup(1, 0, 0, 10);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(setup.fire, 1.0, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {
                                                      { 10, 8, 50 },
                                                      { 9, 9, 50 },
                                                      { 10, 9, 50 },
                                                      { 11, 9, 50 },
                                                  });
}

TEST_CASE("rising fire wraps the bottom lines from the previous frame", "[data]")
{
    SinglePulseFire setup(1, 0, 0, 1);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(setup.fire, 1.0, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {
                                                      { 9, 0, 50 },
                                                      { 10, 0, 50 },
                                                      { 11, 0, 50 },
                                                      { 10, 31, 50 },
                                                  });
}

TEST_CASE("prime count runs extra fire ticks on the first update", "[data]")
{
    SinglePulseFire setup(0, 1, 0, 10);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(setup.fire, 1.0, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {
                                                      { 10, 8, 12 },
                                                      { 9, 9, 25 },
                                                      { 10, 9, 62 },
                                                      { 11, 9, 25 },
                                                      { 8, 10, 12 },
                                                      { 9, 10, 62 },
                                                      { 10, 10, 75 },
                                                      { 11, 10, 62 },
                                                      { 12, 10, 12 },
                                                  });
}

TEST_CASE("max frame rate accumulates time before ticking", "[data]")
{
    SinglePulseFire setup(0, 0, 20, 10);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(setup.fire, 1.0, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {});
    CHECK_FALSE(procedural.update(setup.fire, 1.0, rgba, width, height));
    CHECK_FALSE(procedural.update(setup.fire, 1.03, rgba, width, height));
    REQUIRE(procedural.update(setup.fire, 1.06, rgba, width, height));
    checkIndices(setup.fire, rgba, width, height, {
                                                      { 10, 9, 50 },
                                                      { 9, 10, 50 },
                                                      { 10, 10, 50 },
                                                      { 11, 10, 50 },
                                                  });
}

TEST_CASE("undisturbed wave texture shades every pixel with the flat surface light", "[data]")
{
    dx::UTexture* wave = loadEffect("waterfntn_a", "WaveTexture");
    int32_t numDrops = dx::watertexture::NumDrops(wave);
    uint8_t light = dx::wavetexture::BumpMapLight(wave);
    uint8_t range = dx::wavetexture::PhongRange(wave);
    uint8_t prime = dx::texture::PrimeCount(wave);
    float maxRate = dx::texture::MaxFrameRate(wave);
    dx::watertexture::NumDrops(wave) = 0;
    dx::wavetexture::BumpMapLight(wave) = 85;
    dx::wavetexture::PhongRange(wave) = 0;
    dx::texture::PrimeCount(wave) = 0;
    dx::texture::MaxFrameRate(wave) = 0;

    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(wave, 1.0, rgba, width, height));
    REQUIRE(procedural.update(wave, 1.1, rgba, width, height));
    REQUIRE(procedural.update(wave, 1.2, rgba, width, height));

    dx::watertexture::NumDrops(wave) = numDrops;
    dx::wavetexture::BumpMapLight(wave) = light;
    dx::wavetexture::PhongRange(wave) = range;
    dx::texture::PrimeCount(wave) = prime;
    dx::texture::MaxFrameRate(wave) = maxRate;

    REQUIRE(width == 128);
    REQUIRE(height == 128);
    uint32_t color = wave->palette()->colors[221];
    size_t mismatches = 0;
    for (size_t i = 0; i < static_cast<size_t>(width) * height; i++) {
        bool same = rgba[i * 4 + 0] == (color & 0xff) && rgba[i * 4 + 1] == ((color >> 8) & 0xff) && rgba[i * 4 + 2] == ((color >> 16) & 0xff) && rgba[i * 4 + 3] == 255;
        if (!same) {
            mismatches++;
        }
    }
    CHECK(mismatches == 0);
}

namespace {
    dx::UTexture* makeTexture(const char* className, uint32_t size, const std::vector<uint8_t>& pixels)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UClass* cls = dx::objects::findClass(className);
        REQUIRE(cls != nullptr);
        dx::UObject* object = dx::objects::newObject(cls, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
        dx::UTexture* texture = dx::cast<dx::UTexture>(object);
        REQUIRE(texture != nullptr);
        dx::TextureMip mip;
        mip.width = size;
        mip.height = size;
        mip.ubits = static_cast<uint8_t>(size == 8 ? 3 : 2);
        mip.vbits = mip.ubits;
        mip.data = pixels;
        texture->mips = { mip };
        return texture;
    }

    std::vector<uint8_t> rampSource(uint32_t size)
    {
        std::vector<uint8_t> pixels;
        for (uint32_t v = 0; v < size; v++) {
            for (uint32_t u = 0; u < size; u++) {
                pixels.push_back(static_cast<uint8_t>(10 + size * v + u));
            }
        }
        return pixels;
    }

    dx::UTexture* makeIce(dx::UTexture* source, dx::UTexture* glass, uint32_t moveIce)
    {
        dx::UTexture* ice = makeTexture("Fire.IceTexture", 8, std::vector<uint8_t>(64, 0));
        dx::icetexture::SourceTexture(ice) = source;
        dx::icetexture::GlassTexture(ice) = glass;
        dx::icetexture::PanningStyle(ice) = 0;
        dx::icetexture::TimeMethod(ice) = 1;
        dx::icetexture::HorizPanSpeed(ice) = 132;
        dx::icetexture::VertPanSpeed(ice) = 129;
        dx::icetexture::MoveIce(ice) = moveIce;
        return ice;
    }

    std::vector<uint8_t> columnGlass()
    {
        std::vector<uint8_t> pixels;
        for (uint32_t v = 0; v < 8; v++) {
            for (uint32_t u = 0; u < 8; u++) {
                pixels.push_back(static_cast<uint8_t>(u));
            }
        }
        return pixels;
    }

    void checkIce(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height, const int (&rows)[8], const int (&columns)[8], int rowStride)
    {
        REQUIRE(width == 8);
        REQUIRE(height == 8);
        REQUIRE(rgba.size() == 8 * 8 * 4);
        for (int v = 0; v < 8; v++) {
            for (int u = 0; u < 8; u++) {
                INFO("pixel " << u << "," << v);
                CHECK(rgba[static_cast<size_t>((v * 8 + u) * 4)] == 10 + rowStride * rows[v] + columns[u]);
            }
        }
    }
}

TEST_CASE("ice texture pans the glass layer over the source", "[data]")
{
    dx::UTexture* source = makeTexture("Engine.Texture", 8, rampSource(8));
    dx::UTexture* glass = makeTexture("Engine.Texture", 8, columnGlass());
    dx::UTexture* ice = makeIce(source, glass, 1);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(ice, 1.0, rgba, width, height));
    checkIce(rgba, width, height, { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 2, 4, 6, 0, 2, 4, 6 }, 8);
    REQUIRE(procedural.update(ice, 1.5, rgba, width, height));
    checkIce(rgba, width, height, { 0, 1, 2, 3, 4, 5, 6, 7 }, { 4, 6, 0, 2, 4, 6, 0, 2 }, 8);
}

TEST_CASE("ice texture pans the source under a fixed glass layer", "[data]")
{
    dx::UTexture* source = makeTexture("Engine.Texture", 8, rampSource(8));
    dx::UTexture* glass = makeTexture("Engine.Texture", 8, columnGlass());
    dx::UTexture* ice = makeIce(source, glass, 0);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(ice, 1.0, rgba, width, height));
    REQUIRE(procedural.update(ice, 1.5, rgba, width, height));
    checkIce(rgba, width, height, { 1, 2, 3, 4, 5, 6, 7, 0 }, { 4, 6, 0, 2, 4, 6, 0, 2 }, 8);
}

TEST_CASE("ice texture without glass shows the upsampled source", "[data]")
{
    dx::UTexture* source = makeTexture("Engine.Texture", 4, rampSource(4));
    dx::UTexture* ice = makeIce(source, nullptr, 1);
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    REQUIRE(procedural.update(ice, 1.0, rgba, width, height));
    checkIce(rgba, width, height, { 0, 0, 1, 1, 2, 2, 3, 3 }, { 0, 0, 1, 1, 2, 2, 3, 3 }, 4);
}

TEST_CASE("a wet texture refracts the live frame of a procedural source", "[data]")
{
    dx::UTexture* fire = loadEffect("LaserBeam1", "FireTexture");
    REQUIRE(!fire->mips.empty());
    dx::UClass* cls = dx::objects::findClass("Fire.WetTexture");
    REQUIRE(cls != nullptr);
    dx::UTexture* wet = dx::cast<dx::UTexture>(dx::objects::newObject(cls, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0));
    REQUIRE(wet != nullptr);
    dx::TextureMip mip = fire->mips[0];
    mip.data.assign(mip.data.size(), 0);
    wet->mips = { mip };
    dx::wettexture::SourceTexture(wet) = fire;
    dx::watertexture::WaveAmp(wet) = 0;
    dx::watertexture::NumDrops(wet) = 0;
    dx::ProceduralTextures procedural;
    std::vector<uint8_t> fireRgba;
    std::vector<uint8_t> wetRgba;
    uint32_t width = 0;
    uint32_t height = 0;
    int differentFrames = 0;
    std::vector<uint8_t> first;
    for (int step = 1; step <= 4; step++) {
        double time = 0.1 * step;
        REQUIRE(procedural.update(fire, time, fireRgba, width, height));
        REQUIRE(procedural.update(wet, time, wetRgba, width, height));
        INFO("step " << step);
        CHECK(wetRgba == fireRgba);
        if (step == 1) {
            first = wetRgba;
        } else if (wetRgba != first) {
            differentFrames++;
        }
    }
    CHECK(differentFrames > 0);
}
