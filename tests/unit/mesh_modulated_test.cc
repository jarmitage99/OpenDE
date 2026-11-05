#define SDL_MAIN_HANDLED

#include "core/objects.h"
#include "engine/mesh.h"
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
            window = SDL_CreateWindow("mesh", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, frameWidth, frameHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
        renderer.applyBrightness(0.5f);
        std::vector<uint8_t> rgba;
        int width = 0;
        int height = 0;
        renderer.readPixels(rgba, width, height);
        CHECK(glGetError() == GL_NO_ERROR);
        return rgba;
    }

    int brightness(const std::vector<uint8_t>& rgba, size_t pixel)
    {
        return rgba[pixel * 4] + rgba[pixel * 4 + 1] + rgba[pixel * 4 + 2];
    }
}

TEST_CASE("modulated mesh faces ignore vertex lighting so a 50 percent gray skin leaves the scene unchanged", "[data]")
{
    OffscreenContext gl;
    REQUIRE(gl.context != nullptr);
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
    dx::World world;
    REQUIRE(world.loadMap("01_NYC_UNATCOHQ"));
    dx::UObject* player = world.player();
    REQUIRE(player != nullptr);
    dx::UClass* crateClass = dx::objects::findClass("DeusEx.CrateUnbreakableSmall");
    dx::objects::flushLoads();
    REQUIRE(crateClass != nullptr);
    dx::UObject* trench = dx::objects::loadObject("DeusExCharacters.GM_Trench", nullptr);
    dx::UObject* pink = dx::objects::loadObject("DeusExItems.Skins.PinkMaskTex", nullptr);
    dx::UObject* gray = dx::objects::loadObject("DeusExItems.Skins.GrayMaskTex", nullptr);
    dx::UObject* black = dx::objects::loadObject("DeusExItems.Skins.BlackMaskTex", nullptr);
    REQUIRE(trench != nullptr);
    REQUIRE(pink != nullptr);
    REQUIRE(gray != nullptr);
    REQUIRE(black != nullptr);
    dx::ULodMesh* mesh = dx::cast<dx::ULodMesh>(trench);
    REQUIRE(mesh != nullptr);
    dx::objects::ensureLoaded(mesh);
    REQUIRE(mesh->materials.size() == 7);
    CHECK(mesh->materials[5].polyFlags == 0x140);
    CHECK(mesh->materials[5].textureIndex == 6);
    CHECK(mesh->materials[6].polyFlags == 0x104);
    CHECK(mesh->materials[6].textureIndex == 7);

    dx::Vector front = dx::actor::Location(player) + dx::rotationAxes(dx::actor::Rotation(player)).xAxis * 50.0f;
    dx::Rotator facing { 0, (dx::actor::Rotation(player).yaw + 32768) & 65535, 0 };
    dx::UObject* dummy = world.spawn(crateClass, nullptr, dx::Name(), front, facing, nullptr, true);
    REQUIRE(dummy != nullptr);
    dx::actor::Mesh(dummy) = trench;
    for (int i = 0; i < 6; i++) {
        dx::actor::MultiSkins(dummy, i) = pink;
    }
    dx::actor::MultiSkins(dummy, 6) = gray;
    dx::actor::MultiSkins(dummy, 7) = black;
    dx::Vector eye = dx::actor::Location(player) + dx::Vector { 0, 0, 30 };
    dx::Rotator rotation { 0, dx::actor::Rotation(player).yaw, 0 };

    dx::SceneRenderer renderer;
    render(renderer, world, eye, rotation);
    std::vector<uint8_t> grayFrames = render(renderer, world, eye, rotation);
    dx::actor::MultiSkins(dummy, 6) = black;
    std::vector<uint8_t> blackFrames = render(renderer, world, eye, rotation);
    dx::actor::bHidden(dummy) = 1;
    std::vector<uint8_t> hidden = render(renderer, world, eye, rotation);
    REQUIRE(grayFrames.size() == hidden.size());
    REQUIRE(blackFrames.size() == hidden.size());

    long background = 0;
    long underBlack = 0;
    long underGray = 0;
    int framePixels = 0;
    for (size_t pixel = 0; pixel < hidden.size() / 4; pixel++) {
        int behind = brightness(hidden, pixel);
        if (behind < 60 || brightness(blackFrames, pixel) >= behind) {
            continue;
        }
        framePixels++;
        background += behind;
        underBlack += brightness(blackFrames, pixel);
        underGray += brightness(grayFrames, pixel);
    }
    INFO("frame pixels " << framePixels << " background " << background << " under black " << underBlack << " under gray " << underGray);
    REQUIRE(framePixels > 100);
    CHECK(underBlack * 10 < background);
    CHECK(std::abs(underGray - background) * 20 < background);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOHQ"));
}
