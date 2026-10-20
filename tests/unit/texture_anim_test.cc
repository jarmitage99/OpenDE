#include "core/objects.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "render/texture_cache.h"

#include <catch_amalgamated.hpp>

namespace {
    dx::UTexture* makeTexture(dx::UClass* cls)
    {
        dx::UObject* object = dx::objects::newObject(cls, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
        return dx::cast<dx::UTexture>(object);
    }
}

TEST_CASE("animated textures advance like UTexture::Update and Tick", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* cls = dx::objects::findClass("Engine.Texture");
    REQUIRE(cls != nullptr);
    dx::UTexture* a = makeTexture(cls);
    dx::UTexture* b = makeTexture(cls);
    dx::UTexture* c = makeTexture(cls);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    dx::texture::AnimNext(a) = b;
    dx::texture::AnimNext(b) = c;
    dx::texture::AnimNext(c) = a;

    dx::texture::MaxFrameRate(a) = 0.0f;
    dx::TextureAnimation perFrame;
    CHECK(dx::TextureCache::advance(a, perFrame, 0.01) == b);
    CHECK(dx::TextureCache::advance(a, perFrame, 0.01) == b);
    CHECK(dx::TextureCache::advance(a, perFrame, 0.02) == c);
    CHECK(dx::TextureCache::advance(a, perFrame, 0.03) == a);

    dx::texture::MaxFrameRate(a) = 20.0f;
    dx::texture::MinFrameRate(a) = 10.0f;
    dx::TextureAnimation limited;
    limited.lastUpdate = 1.0;
    CHECK(dx::TextureCache::advance(a, limited, 1.03) == a);
    CHECK(dx::TextureCache::advance(a, limited, 1.06) == b);
    CHECK(limited.accumulator == 0.0f);
    CHECK(dx::TextureCache::advance(a, limited, 1.31) == c);
    CHECK(limited.accumulator == Catch::Approx(0.1f).margin(1e-5));

    dx::texture::MaxFrameRate(a) = 0.0f;
    dx::texture::PrimeCount(a) = 2;
    dx::TextureAnimation primed;
    CHECK(dx::TextureCache::advance(a, primed, 0.5) == a);
}
