#define SDL_MAIN_HANDLED

#include "core/objects.h"
#include "engine/props_engine.h"
#include "engine/world.h"
#include "render/scene_renderer.h"

#include <SDL.h>
#include <catch_amalgamated.hpp>
#include <glad/gl.h>

#include <cstdlib>
#include <vector>

namespace {
    constexpr int frameWidth = 640;
    constexpr int frameHeight = 480;

    struct OffscreenContext {
        SDL_Window* window = nullptr;
        SDL_GLContext context = nullptr;

        OffscreenContext()
        {
            SDL_SetMainReady();
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
                return;
            }
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            window = SDL_CreateWindow("smuggler", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, frameWidth, frameHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
            if (!window) {
                return;
            }
            context = SDL_GL_CreateContext(window);
            if (context && !gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
                SDL_GL_DeleteContext(context);
                context = nullptr;
            }
        }

        ~OffscreenContext()
        {
            if (context) {
                SDL_GL_DeleteContext(context);
            }
            if (window) {
                SDL_DestroyWindow(window);
            }
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }

        OffscreenContext(const OffscreenContext&) = delete;
        OffscreenContext& operator=(const OffscreenContext&) = delete;
    };

    std::vector<uint8_t> render(dx::SceneRenderer& renderer, dx::World& world, const dx::Vector& eye, const dx::Rotator& rotation)
    {
        renderer.setOptions(dx::RenderOptions {});
        renderer.setTime(1.0);
        renderer.beginFrame(frameWidth, frameHeight);
        renderer.drawScene(world.level, world.actors, eye, rotation, 90.0f, world.player());
        renderer.endFrame();
        std::vector<uint8_t> rgba;
        int width = 0;
        int height = 0;
        renderer.readPixels(rgba, width, height);
        CHECK(glGetError() == GL_NO_ERROR);
        return rgba;
    }
}

TEST_CASE("the mirror on the Smuggler's stash mover reflects the chair standing behind the viewer", "[data]")
{
    OffscreenContext gl;
    REQUIRE(gl.context != nullptr);
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("02_NYC_Smug"));
    dx::World world;
    REQUIRE(world.loadMap("02_NYC_Smug"));
    dx::UObject* chair = nullptr;
    for (dx::UObject* a : world.actors) {
        if (a && !a->destroyed && a->name == dx::Name("Chair6")) {
            chair = a;
        }
    }
    REQUIRE(chair != nullptr);
    dx::Vector eye { -580, 1450, 224 };
    dx::Rotator rotation { 0, 16384, 0 };

    dx::SceneRenderer renderer;
    render(renderer, world, eye, rotation);
    std::vector<uint8_t> shown = render(renderer, world, eye, rotation);
    dx::actor::bHidden(chair) = 1;
    std::vector<uint8_t> hidden = render(renderer, world, eye, rotation);
    REQUIRE(shown.size() == hidden.size());

    int changed = 0;
    for (size_t pixel = 0; pixel < shown.size() / 4; pixel++) {
        int difference = std::abs(shown[pixel * 4] - hidden[pixel * 4]) + std::abs(shown[pixel * 4 + 1] - hidden[pixel * 4 + 1]) + std::abs(shown[pixel * 4 + 2] - hidden[pixel * 4 + 2]);
        if (difference > 30) {
            changed++;
        }
    }
    CHECK(changed > 50);
    dx::objects::unloadPackage(dx::Name("02_NYC_Smug"));
}

TEST_CASE("a burning barrel lights the far wall of the Smuggler's basement while more than eight dynamic lights are closer", "[data]")
{
    OffscreenContext gl;
    REQUIRE(gl.context != nullptr);
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("02_NYC_Smug"));
    dx::World world;
    REQUIRE(world.loadMap("02_NYC_Smug"));
    dx::UObject* barrel = nullptr;
    for (dx::UObject* a : world.actors) {
        if (a && !a->destroyed && a->name == dx::Name("BarrelFire10")) {
            barrel = a;
        }
    }
    REQUIRE(barrel != nullptr);
    dx::Vector eye { -53, 14, 70 };
    dx::Rotator rotation { 0, 21400, 0 };

    dx::SceneRenderer renderer;
    render(renderer, world, eye, rotation);
    std::vector<uint8_t> lit = render(renderer, world, eye, rotation);
    dx::actor::LightType(barrel) = 0;
    std::vector<uint8_t> dark = render(renderer, world, eye, rotation);
    REQUIRE(lit.size() == dark.size());

    int brighter = 0;
    for (size_t pixel = 0; pixel < lit.size() / 4; pixel++) {
        int difference = (lit[pixel * 4] + lit[pixel * 4 + 1] + lit[pixel * 4 + 2]) - (dark[pixel * 4] + dark[pixel * 4 + 1] + dark[pixel * 4 + 2]);
        if (difference > 15) {
            brighter++;
        }
    }
    CHECK(brighter > 1000);
    dx::objects::unloadPackage(dx::Name("02_NYC_Smug"));
}
