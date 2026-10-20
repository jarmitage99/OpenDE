#include "audio/audio.h"
#include "core/config.h"
#include "core/objects.h"
#include "core/url.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/actor.h"
#include "engine/canvas_hooks.h"
#include "engine/console.h"
#include "engine/input.h"
#include "engine/resources.h"
#include "engine/savegame.h"
#include "engine/world.h"
#include "extension/extension.h"
#include "render/lighting.h"
#include "render/scene_renderer.h"

#include <SDL.h>
#include <glad/gl.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
#ifdef __EMSCRIPTEN__
    constexpr bool browser = true;
#else
    constexpr bool browser = false;
#endif

    uint8_t translateKey(SDL_Keycode key)
    {
        if (key >= SDLK_a && key <= SDLK_z) {
            return static_cast<uint8_t>(dx::IK_A + (key - SDLK_a));
        }
        if (key >= SDLK_0 && key <= SDLK_9) {
            return static_cast<uint8_t>(dx::IK_0 + (key - SDLK_0));
        }
        if (key >= SDLK_F1 && key <= SDLK_F12) {
            return static_cast<uint8_t>(dx::IK_F1 + (key - SDLK_F1));
        }
        if (key >= SDLK_KP_1 && key <= SDLK_KP_9) {
            return static_cast<uint8_t>(dx::IK_NumPad0 + 1 + (key - SDLK_KP_1));
        }
        switch (key) {
        case SDLK_KP_0:
            return dx::IK_NumPad0;
        case SDLK_BACKSPACE:
            return dx::IK_Backspace;
        case SDLK_TAB:
            return dx::IK_Tab;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return dx::IK_Enter;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            return dx::IK_Shift;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            return dx::IK_Ctrl;
        case SDLK_LALT:
        case SDLK_RALT:
            return dx::IK_Alt;
        case SDLK_PAUSE:
            return dx::IK_Pause;
        case SDLK_CAPSLOCK:
            return dx::IK_CapsLock;
        case SDLK_ESCAPE:
            return dx::IK_Escape;
        case SDLK_SPACE:
            return dx::IK_Space;
        case SDLK_PAGEUP:
            return dx::IK_PageUp;
        case SDLK_PAGEDOWN:
            return dx::IK_PageDown;
        case SDLK_END:
            return dx::IK_End;
        case SDLK_HOME:
            return dx::IK_Home;
        case SDLK_LEFT:
            return dx::IK_Left;
        case SDLK_UP:
            return dx::IK_Up;
        case SDLK_RIGHT:
            return dx::IK_Right;
        case SDLK_DOWN:
            return dx::IK_Down;
        case SDLK_INSERT:
            return dx::IK_Insert;
        case SDLK_DELETE:
            return dx::IK_Delete;
        case SDLK_KP_MULTIPLY:
            return dx::IK_GreyStar;
        case SDLK_KP_PLUS:
            return dx::IK_GreyPlus;
        case SDLK_KP_MINUS:
            return dx::IK_GreyMinus;
        case SDLK_KP_PERIOD:
            return dx::IK_NumPadPeriod;
        case SDLK_KP_DIVIDE:
            return dx::IK_GreySlash;
        case SDLK_NUMLOCKCLEAR:
            return dx::IK_NumLock;
        case SDLK_SCROLLLOCK:
            return dx::IK_ScrollLock;
        case SDLK_SEMICOLON:
            return dx::IK_Semicolon;
        case SDLK_EQUALS:
            return dx::IK_Equals;
        case SDLK_COMMA:
            return dx::IK_Comma;
        case SDLK_MINUS:
            return dx::IK_Minus;
        case SDLK_PERIOD:
            return dx::IK_Period;
        case SDLK_SLASH:
            return dx::IK_Slash;
        case SDLK_BACKQUOTE:
            return dx::IK_Tilde;
        case SDLK_LEFTBRACKET:
            return dx::IK_LeftBracket;
        case SDLK_BACKSLASH:
            return dx::IK_Backslash;
        case SDLK_RIGHTBRACKET:
            return dx::IK_RightBracket;
        case SDLK_QUOTE:
            return dx::IK_SingleQuote;
        default:
            return dx::IK_None;
        }
    }

    uint8_t translateButton(uint8_t button)
    {
        switch (button) {
        case SDL_BUTTON_LEFT:
            return dx::IK_LeftMouse;
        case SDL_BUTTON_RIGHT:
            return dx::IK_RightMouse;
        case SDL_BUTTON_MIDDLE:
            return dx::IK_MiddleMouse;
        default:
            return dx::IK_None;
        }
    }

    struct Options {
        std::string map;
        std::string screenshot;
        int frames = -1;
        int width = 1280;
        int height = 720;
        bool sizeGiven = false;
        std::vector<std::string> commands;
        std::vector<std::pair<int, std::string>> actions;
    };

    Options parseOptions(int argc, char** argv)
    {
        Options options;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--screenshot" && i + 1 < argc) {
                options.screenshot = argv[++i];
            } else if (arg == "--frames" && i + 1 < argc) {
                options.frames = std::atoi(argv[++i]);
            } else if (arg == "--width" && i + 1 < argc) {
                options.width = std::atoi(argv[++i]);
                options.sizeGiven = true;
            } else if (arg == "--height" && i + 1 < argc) {
                options.height = std::atoi(argv[++i]);
                options.sizeGiven = true;
            } else if (arg == "--exec" && i + 1 < argc) {
                options.commands.push_back(argv[++i]);
            } else if (arg == "--at" && i + 1 < argc) {
                std::string spec = argv[++i];
                size_t colon = spec.find(':');
                if (colon != std::string::npos) {
                    options.actions.emplace_back(std::atoi(spec.substr(0, colon).c_str()), spec.substr(colon + 1));
                }
            } else {
                options.map = arg;
            }
        }
        return options;
    }

    void saveBmp(const std::string& path, const std::vector<uint8_t>& rgba, int width, int height)
    {
        std::vector<uint8_t> pixels = rgba;
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(), width, height, 32, width * 4, SDL_PIXELFORMAT_ABGR8888);
        SDL_SaveBMP(surface, path.c_str());
        SDL_FreeSurface(surface);
    }

    class Game {
    public:
        Game(SDL_Window* target, const Options& settings)
            : window(target)
            , options(settings)
        {
        }

        std::string videoSection()
        {
            return dx::config::system().get("Engine.Engine", "ViewportManager").value_or("WinDrv.WindowsClient");
        }

        int videoSetting(const char* key, int fallback)
        {
            std::optional<std::string> value = dx::config::system().get(videoSection(), key);
            if (!value) {
                return fallback;
            }
            if (*value == "True" || *value == "true") {
                return 1;
            }
            if (*value == "False" || *value == "false") {
                return 0;
            }
            return std::atoi(value->c_str());
        }

        void saveVideoMode(bool fullscreen, int width, int height)
        {
            dx::Ini& ini = dx::config::system();
            std::string section = videoSection();
            ini.set(section, fullscreen ? "FullscreenViewportX" : "WindowedViewportX", std::to_string(width));
            ini.set(section, fullscreen ? "FullscreenViewportY" : "WindowedViewportY", std::to_string(height));
            ini.set(section, "StartupFullscreen", fullscreen ? "True" : "False");
            ini.save();
        }

        void applyVideoMode(bool fullscreen, int width, int height)
        {
            if (fullscreen && browser) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            } else if (fullscreen) {
                int display = SDL_GetWindowDisplayIndex(window);
                SDL_DisplayMode wanted {};
                wanted.w = width;
                wanted.h = height;
                SDL_DisplayMode mode {};
                if (SDL_GetClosestDisplayMode(display, &wanted, &mode)) {
                    SDL_SetWindowDisplayMode(window, &mode);
                }
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            } else {
                SDL_SetWindowFullscreen(window, 0);
                SDL_SetWindowSize(window, width, height);
            }
        }

        std::string availableResolutions()
        {
            int display = SDL_GetWindowDisplayIndex(window);
            std::vector<std::pair<int, int>> sizes;
            for (int i = 0; i < SDL_GetNumDisplayModes(display); i++) {
                SDL_DisplayMode mode {};
                if (SDL_GetDisplayMode(display, i, &mode) == 0 && mode.w >= 640 && mode.h >= 480) {
                    sizes.emplace_back(mode.w, mode.h);
                }
            }
            std::sort(sizes.begin(), sizes.end());
            sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
            if (sizes.size() > 16) {
                sizes.erase(sizes.begin(), sizes.end() - 16);
            }
            std::string text;
            for (const auto& [width, height] : sizes) {
                if (!text.empty()) {
                    text += " ";
                }
                text += std::to_string(width) + "x" + std::to_string(height);
            }
            return text;
        }

        float displayFov(float fov, int width, int height)
        {
            int index = std::clamp(videoSetting("FieldOfViewIndex", 3), 0, 12);
            float wanted = 60.0f + 5.0f * static_cast<float>(index);
            float adjusted = fov + (wanted - 75.0f) * std::clamp((fov - 25.0f) / 50.0f, 0.0f, 1.0f);
            float aspect = static_cast<float>(width) / static_cast<float>(height) * 0.75f;
            return std::atan(std::tan(adjusted * dx::Pi / 360.0f) * aspect) * 360.0f / dx::Pi;
        }

        void loadDeusExClasses()
        {
            dx::Linker* linker = dx::objects::loadPackage(dx::Name("DeusEx"));
            const dx::Package& file = linker->file();
            for (size_t i = 0; i < file.exports().size(); i++) {
                if (file.exports()[i].classRef == 0) {
                    dx::objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
                }
            }
            dx::objects::flushLoads();
        }

        dx::UClass* createExtraOptionsScreen()
        {
            dx::UClass* screen = dx::objects::createSubclass(dx::objects::findClass("DeusEx.MenuScreenOptions"), dx::Name("MenuScreenExtraOptions"));
            dx::UObject* defaults = screen->defaultObject;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("Title"))) = "Extra Options";
            for (int i = 0; i < screen->findProperty(dx::Name("choices"))->arrayDim; i++) {
                *reinterpret_cast<dx::UObject**>(defaults->propertyAddress(dx::Name("choices"), i)) = nullptr;
            }
            return screen;
        }

        void addChoice(dx::UClass* screen, int index, const char* parentPath, const char* name, const char* action, const char* help, const char* key, const std::vector<const char*>& values, int fallback, const std::string& seed)
        {
            dx::UClass* choice = dx::objects::createSubclass(dx::objects::findClass(parentPath), dx::Name(name));
            dx::UObject* defaults = choice->defaultObject;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("actionText"))) = action;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("helpText"))) = help;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("configSetting"))) = std::string("ini:Engine.Engine.ViewportManager ") + key;
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("defaultValue"))) = fallback;
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("defaultInfoWidth"))) = 88;
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("defaultInfoPosX"))) = 270;
            for (size_t i = 0; i < 40; i++) {
                *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("enumText"), static_cast<int32_t>(i))) = i < values.size() ? values[i] : "";
            }
            *reinterpret_cast<dx::UObject**>(screen->defaultObject->propertyAddress(dx::Name("choices"), index)) = choice;
            if (!dx::config::system().get(videoSection(), key)) {
                dx::config::system().set(videoSection(), key, seed);
            }
        }

        void addEnumChoice(dx::UClass* screen, int index, const char* name, const char* action, const char* help, const char* key, const std::vector<const char*>& values, int fallback)
        {
            addChoice(screen, index, "DeusEx.MenuUIChoiceEnum", name, action, help, key, values, fallback, std::to_string(fallback));
        }

        void addSwitchChoice(dx::UClass* screen, int index, const char* name, const char* action, const char* help, const char* key, bool fallback)
        {
            addChoice(screen, index, "DeusEx.MenuChoice_DetailTextures", name, action, help, key, { "Off", "On" }, fallback ? 1 : 0, fallback ? "True" : "False");
        }

        void addExtraOptions()
        {
            dx::UClass* screen = createExtraOptionsScreen();
            bool trilinear = dx::config::system().get("OpenGLDrv.OpenGLRenderDevice", "UseTrilinear").value_or("False") == "True";
            addEnumChoice(screen, 0, "MenuChoice_Anisotropy", "|&Anisotropic Filtering", "Sharpens textures seen at a steep angle, such as floors in the distance.", "Anisotropy", { "Off", "2x", "4x", "8x", "16x" }, 0);
            addSwitchChoice(screen, 1, "MenuChoice_Trilinear", "|&Trilinear Filtering", "Blends between texture detail levels to remove visible bands on floors and walls.", "UseTrilinear", trilinear);
            addEnumChoice(screen, 2, "MenuChoice_Multisampling", "Multi|&sample AA", "Smooths the edges of world geometry. Higher values cost more performance.", "Multisampling", { "Off", "2x", "4x", "8x" }, 0);
            addSwitchChoice(screen, 3, "MenuChoice_FXAA", "|&FXAA", "Fast edge smoothing of the 3D view that also covers masked textures such as fences.", "FXAA", false);
            addSwitchChoice(screen, 4, "MenuChoice_SmoothLighting", "Smooth |&Lighting", "Filters lightmaps with a bicubic filter to soften blocky shadow edges.", "SmoothLightmaps", false);
            addEnumChoice(screen, 5, "MenuChoice_DetailDistance", "|&Detail Distance", "How far away detail textures stay visible. Normal matches the original game.", "DetailDistance", { "Normal", "Far", "Very Far" }, 0);
            if (browser) {
                addPlayerSpeedChoice(screen, 6);
            } else {
                addEnumChoice(screen, 6, "MenuChoice_VSync", "|&Vertical Sync", "Waits for the display refresh. Adaptive skips the wait when a frame is late.", "VSync", { "Off", "On", "Adaptive" }, 1);
                addEnumChoice(screen, 7, "MenuChoice_FrameRateLimit", "Frame Rate L|&imit", "Caps the frame rate to save power and reduce heat.", "FrameRateLimit", { "Off", "30", "60", "75", "120", "144", "165", "240" }, 0);
                addPlayerSpeedChoice(screen, 8);
            }
            dx::deusex::insertMenuButton(dx::objects::findClass("DeusEx.MenuSettings")->defaultObject, 3, "Extra Options", screen);
        }

        void addPlayerSpeedChoice(dx::UClass* screen, int index)
        {
            dx::UClass* choice = dx::objects::createSubclass(dx::objects::findClass("DeusEx.MenuUIChoiceSlider"), dx::Name("MenuChoice_PlayerSpeed"));
            *reinterpret_cast<dx::UObject**>(screen->defaultObject->propertyAddress(dx::Name("choices"), index)) = choice;
            dx::UObject* defaults = choice->defaultObject;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("actionText"))) = "Player Sp|&eed";
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("helpText"))) = "Player movement speed in percent of normal speed.";
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("configSetting"))) = "ini:Engine.Engine.GameEngine PlayerSpeed";
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("numTicks"))) = 16;
            *reinterpret_cast<float*>(defaults->propertyAddress(dx::Name("startValue"))) = 50.0f;
            *reinterpret_cast<float*>(defaults->propertyAddress(dx::Name("endValue"))) = 200.0f;
            *reinterpret_cast<float*>(defaults->propertyAddress(dx::Name("defaultValue"))) = 100.0f;
            std::string section = dx::config::system().get("Engine.Engine", "GameEngine").value_or("DeusEx.DeusExGameEngine");
            if (!dx::config::system().get(section, "PlayerSpeed")) {
                dx::config::system().set(section, "PlayerSpeed", "100");
            }
        }

        void applyPlayerSpeed()
        {
            std::string section = dx::config::system().get("Engine.Engine", "GameEngine").value_or("DeusEx.DeusExGameEngine");
            float speed = static_cast<float>(std::atof(dx::config::system().get(section, "PlayerSpeed").value_or("100").c_str())) / 100.0f;
            if (speed == appliedPlayerSpeed) {
                return;
            }
            appliedPlayerSpeed = speed;
            dx::UClass* playerPawn = dx::objects::findClass("Engine.PlayerPawn");
            for (dx::UObject* object : dx::objects::all()) {
                dx::UClass* cls = dx::cast<dx::UClass>(object);
                if (!cls || !cls->defaultObject || !cls->isChildOf(playerPawn)) {
                    continue;
                }
                float* ground = reinterpret_cast<float*>(cls->defaultObject->propertyAddress(dx::Name("GroundSpeed")));
                float* water = reinterpret_cast<float*>(cls->defaultObject->propertyAddress(dx::Name("WaterSpeed")));
                const std::pair<float, float>& base = basePlayerSpeeds.try_emplace(cls, *ground, *water).first->second;
                *ground = base.first * speed;
                *water = base.second * speed;
            }
        }

        void applyRenderOptions()
        {
            dx::RenderOptions render;
            render.anisotropy = static_cast<float>(1 << std::clamp(videoSetting("Anisotropy", 0), 0, 4));
            render.trilinear = videoSetting("UseTrilinear", 0) != 0;
            int multisample = std::clamp(videoSetting("Multisampling", 0), 0, 3);
            render.multisample = multisample > 0 ? 1 << multisample : 0;
            render.fxaa = videoSetting("FXAA", 0) != 0;
            render.smoothLightmaps = videoSetting("SmoothLightmaps", 0) != 0;
            const float detailDistances[] = { 380.0f, 1024.0f, 2048.0f };
            render.detailDistance = detailDistances[std::clamp(videoSetting("DetailDistance", 0), 0, 2)];
            renderer->setOptions(render);
            if (browser || options.frames >= 0) {
                return;
            }
            int vsync = std::clamp(videoSetting("VSync", 1), 0, 2);
            if (vsync != appliedVSync) {
                appliedVSync = vsync;
                if (vsync != 2 || SDL_GL_SetSwapInterval(-1) != 0) {
                    SDL_GL_SetSwapInterval(std::min(vsync, 1));
                }
            }
        }

        void limitFrameRate()
        {
            const int limits[] = { 0, 30, 60, 75, 120, 144, 165, 240 };
            int limit = limits[std::clamp(videoSetting("FrameRateLimit", 0), 0, 7)];
            if (browser || limit == 0 || options.frames >= 0) {
                return;
            }
            uint64_t frequency = SDL_GetPerformanceFrequency();
            uint64_t deadline = frameDeadline + frequency / static_cast<uint64_t>(limit);
            uint64_t now = SDL_GetPerformanceCounter();
            if (now >= deadline) {
                frameDeadline = now;
                return;
            }
            while (now < deadline) {
                uint64_t remaining = (deadline - now) * 1000 / frequency;
                if (remaining > 1) {
                    SDL_Delay(static_cast<uint32_t>(remaining - 1));
                }
                now = SDL_GetPerformanceCounter();
            }
            frameDeadline = deadline;
        }

        void addFieldOfViewChoice()
        {
            dx::UClass* screen = dx::objects::findClass("DeusEx.MenuScreenDisplay");
            dx::UClass* choice = dx::objects::createSubclass(dx::objects::findClass("DeusEx.MenuUIChoiceEnum"), dx::Name("MenuChoice_FieldOfView"));
            *reinterpret_cast<dx::UObject**>(screen->defaultObject->propertyAddress(dx::Name("choices"), 2)) = choice;
            dx::UObject* defaults = choice->defaultObject;
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("actionText"))) = "Field of |&View";
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("helpText"))) = "Horizontal field of view on a 4:3 screen. Wider screens show more at the sides.";
            *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("configSetting"))) = "ini:Engine.Engine.ViewportManager FieldOfViewIndex";
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("defaultValue"))) = 3;
            *reinterpret_cast<int32_t*>(defaults->propertyAddress(dx::Name("defaultInfoWidth"))) = 98;
            for (int i = 0; i <= 12; i++) {
                *reinterpret_cast<std::string*>(defaults->propertyAddress(dx::Name("enumText"), i)) = std::to_string(60 + 5 * i);
            }
            if (!dx::config::system().get(videoSection(), "FieldOfViewIndex")) {
                dx::config::system().set(videoSection(), "FieldOfViewIndex", "3");
            }
        }

        bool start()
        {
            dx::engine::initialize(dx::config::findGameRoot(DEUSEX_GAME_DATA).string());
            dx::audio::initialize();
            dx::audio::setLightRandom(dx::lightRandomBase);
            renderer = std::make_unique<dx::SceneRenderer>();
            renderer->iteratorHook = [this](dx::UObject* actor, const std::function<void(dx::UObject*)>& emit) {
                return world && dx::deusex::renderIterate(*world, actor, emit);
            };
            dx::console::openMapHook = [this](const std::string& url, int32_t travelType) {
                pendingMap = url;
                pendingTravelType = travelType;
            };
            dx::console::quitHook = [this]() {
                running = false;
            };
            dx::console::currentResHook = [this]() {
                int width = 0;
                int height = 0;
                SDL_GetWindowSize(window, &width, &height);
                return std::to_string(width) + "x" + std::to_string(height);
            };
            dx::console::resolutionsHook = [this]() {
                return availableResolutions();
            };
            dx::console::setResHook = [this](int width, int height) {
                bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
                applyVideoMode(fullscreen, width, height);
                saveVideoMode(fullscreen, width, height);
            };
            dx::console::toggleFullscreenHook = [this]() {
                bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == 0;
                int width = videoSetting(fullscreen ? "FullscreenViewportX" : "WindowedViewportX", 640);
                int height = videoSetting(fullscreen ? "FullscreenViewportY" : "WindowedViewportY", 480);
                applyVideoMode(fullscreen, width, height);
                saveVideoMode(fullscreen, width, height);
            };
            dx::console::screenshotHook = [this]() {
                std::filesystem::path directory = dx::config::system().path.parent_path() / "Screenshots";
                std::filesystem::create_directories(directory);
                int index = 0;
                std::filesystem::path file;
                do {
                    char name[32];
                    std::snprintf(name, sizeof(name), "Shot%04d.bmp", index++);
                    file = directory / name;
                } while (std::filesystem::exists(file));
                pendingShot = file.string();
            };
            dx::console::applyAudioSettings();
            if (options.frames < 0 && !options.sizeGiven) {
                bool fullscreen = videoSetting("StartupFullscreen", 0) != 0;
                applyVideoMode(fullscreen, videoSetting(fullscreen ? "FullscreenViewportX" : "WindowedViewportX", options.width), videoSetting(fullscreen ? "FullscreenViewportY" : "WindowedViewportY", options.height));
            }
            dx::CanvasHooks& hooks = dx::canvasHooks();
            hooks.target = &renderer->canvas();
            hooks.drawActor = [this](dx::UObject* actor, bool) {
                renderer->drawActorOverlay(actor, viewLocation, viewRotation, viewFov, true);
            };
            hooks.clipboardCopy = [](const std::string& text) {
                SDL_SetClipboardText(text.c_str());
            };
            hooks.clipboardPaste = []() {
                char* text = SDL_GetClipboardText();
                std::string result = text ? text : "";
                SDL_free(text);
                return result;
            };
            dx::extension::setFrameCanvas(&renderer->canvas());
            dx::extension::setProjector([this](const dx::Vector& location, float& pixelX, float& pixelY) {
                return project(location, pixelX, pixelY);
            });
            dx::extension::setActorDrawer([this](dx::Canvas2D&, const dx::extension::ActorDrawRequest& request) {
                if (request.actor) {
                    renderer->drawActorOverlay(request.actor, viewLocation, viewRotation, viewFov, request.clearZ);
                }
            });
            dx::extension::setViewportDrawer([this](dx::Canvas2D&, const dx::extension::ViewportRequest& request) {
                if (!world || !world->level) {
                    return;
                }
                dx::UObject* hidden = request.showActor ? nullptr : request.originActor;
                int x = static_cast<int>(std::lround(request.x));
                int y = static_cast<int>(std::lround(request.y));
                int width = static_cast<int>(std::lround(request.width));
                int height = static_cast<int>(std::lround(request.height));
                renderer->drawSceneRect(world->level, world->actors, request.location, request.rotation, request.fov, hidden, x, y, width, height);
            });
            loadDeusExClasses();
            addFieldOfViewChoice();
            addExtraOptions();
            std::string map = options.map;
            std::filesystem::path mapFile = map;
            if (mapFile.has_parent_path() && std::filesystem::is_regular_file(mapFile)) {
                map = mapFile.stem().string();
                dx::objects::loadPackageFile(dx::Name(map), mapFile, false);
            }
            if (map.empty()) {
                auto local = dx::config::system().get("URL", "LocalMap");
                map = local ? *local : "DX.dx";
            }
            return openMap(map);
        }

        bool project(const dx::Vector& location, float& pixelX, float& pixelY)
        {
            dx::Coords axes = dx::rotationAxes(viewRotation);
            dx::Vector delta = location - viewLocation;
            float forward = delta.dot(axes.xAxis);
            if (forward <= 1.0f) {
                return false;
            }
            float right = delta.dot(axes.yAxis);
            float up = delta.dot(axes.zAxis);
            float halfWidth = static_cast<float>(frameWidth) * 0.5f;
            float halfHeight = static_cast<float>(frameHeight) * 0.5f;
            float scale = halfWidth / std::tan(viewFov * dx::Pi / 360.0f);
            pixelX = halfWidth + right / forward * scale;
            pixelY = halfHeight - up / forward * scale;
            return true;
        }

        bool openMap(const std::string& url, bool carryItems = false)
        {
            std::vector<dx::TravelActor> travel;
            std::vector<std::pair<std::string, std::string>> travelInfo;
            if (carryItems && world) {
                travel = world->collectTravel();
                dx::UObject* player = world->player();
                if (player) {
                    dx::UObject* replication = dx::pawn::PlayerReplicationInfo(player);
                    std::string key = replication ? dx::playerreplicationinfo::PlayerName(replication) : std::string("Player");
                    travelInfo.emplace_back(key, world->travelString(player));
                }
            }
            std::string target = url;
            if (world && !target.empty() && target[0] == '?') {
                target = world->url.substr(0, world->url.find('?')) + target;
            }
            int32_t loadIndex = 0;
            bool loading = dx::savegame::loadRequested(target, loadIndex);
            if (loading) {
                std::string map;
                if (!dx::savegame::prepareLoad(loadIndex, map)) {
                    std::fprintf(stderr, "cannot load saved game %d\n", loadIndex);
                    dx::World::releaseTravel(travel);
                    return false;
                }
                target = map + "?loadgame=" + std::to_string(loadIndex);
            } else if (world) {
                dx::savegame::storeDepartingLevel(*world, target);
            }
            if (loading || world) {
                dx::savegame::selectLevel(target);
            }
            dx::extension::reset();
            renderer->flushLevel();
            std::vector<std::pair<std::string, std::string>> previousTravelInfo;
            if (world && world->level) {
                previousTravelInfo = world->level->travelInfo;
            }
            if (world && world->level) {
                dx::objects::unloadPackage(world->level->outermost()->name);
            }
            world.reset();
            canvasObject = nullptr;
            world = std::make_unique<dx::World>();
            world->incomingTravel = std::move(travel);
            if (!loading) {
                bool restart = target.find("?restart") != std::string::npos || target.find("?RESTART") != std::string::npos;
                if (restart) {
                    travelInfo = previousTravelInfo;
                }
                world->arrivalTravelInfo = std::move(travelInfo);
                world->replaceTravelInfo = true;
            }
            world->inputHook = [](dx::UObject* player, float delta) {
                dx::console::readInput(player, delta);
            };
            if (!world->loadMap(target)) {
                return false;
            }
            if (loading) {
                world->url = world->level->url;
            }
            dx::console::resetInput(world->player());
            return true;
        }

        dx::UObject* canvas(int width, int height)
        {
            if (!canvasObject) {
                canvasObject = dx::objects::newObject(dx::engineClasses().canvas, dx::objects::packageObject(dx::Name("Transient")), dx::Name("Canvas0"), 0);
                dx::canvas::Viewport(canvasObject) = world->viewport;
            }
            dx::vm::event(canvasObject, dx::Name("Reset"));
            dx::canvas::SizeX(canvasObject) = width;
            dx::canvas::SizeY(canvasObject) = height;
            dx::canvas::ClipX(canvasObject) = static_cast<float>(width);
            dx::canvas::ClipY(canvasObject) = static_cast<float>(height);
            return canvasObject;
        }

        dx::UObject* scriptConsole()
        {
            if (!world || !world->viewport || !world->player()) {
                return nullptr;
            }
            return dx::player::Console(world->viewport);
        }

        bool consoleKeyEvent(uint8_t key, uint8_t action, float delta)
        {
            dx::UObject* console = scriptConsole();
            if (!console) {
                return false;
            }
            dx::vm::Call call(console, dx::Name("KeyEvent"));
            if (!call.valid()) {
                return false;
            }
            call.byte(key).byte(action).f(delta).run();
            return call.returnValue() && *reinterpret_cast<uint32_t*>(call.returnValue());
        }

        void charEvent(char c)
        {
            dx::UObject* console = scriptConsole();
            if (console) {
                dx::vm::Call call(console, dx::Name("KeyType"));
                if (call.valid()) {
                    call.byte(static_cast<uint8_t>(c)).run();
                    if (call.returnValue() && *reinterpret_cast<uint32_t*>(call.returnValue())) {
                        return;
                    }
                }
            }
            dx::extension::charEvent(c);
        }

        void handleKey(uint8_t key, bool pressed)
        {
            if (key == dx::IK_None) {
                return;
            }
            uint8_t action = pressed ? dx::IST_Press : dx::IST_Release;
            if (consoleKeyEvent(key, action, 0)) {
                return;
            }
            if (dx::extension::keyEvent(key, action, 0)) {
                if (!pressed) {
                    dx::console::key(key, action, 0);
                }
                return;
            }
            dx::console::key(key, action, 0);
        }

        void processEvents()
        {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (!event.key.repeat) {
                        handleKey(translateKey(event.key.keysym.sym), true);
                    } else if (!consoleKeyEvent(translateKey(event.key.keysym.sym), dx::IST_Hold, 0)) {
                        dx::extension::keyEvent(translateKey(event.key.keysym.sym), dx::IST_Hold, 0);
                    }
                    break;
                case SDL_KEYUP:
                    handleKey(translateKey(event.key.keysym.sym), false);
                    break;
                case SDL_TEXTINPUT:
                    for (const char* c = event.text.text; *c; c++) {
                        charEvent(*c);
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (dx::extension::wantsMouse()) {
                        int width = 0;
                        int height = 0;
                        SDL_GetWindowSize(window, &width, &height);
                        int drawWidth = 0;
                        int drawHeight = 0;
                        SDL_GL_GetDrawableSize(window, &drawWidth, &drawHeight);
                        float scaleX = width > 0 ? static_cast<float>(drawWidth) / static_cast<float>(width) : 1.0f;
                        float scaleY = height > 0 ? static_cast<float>(drawHeight) / static_cast<float>(height) : 1.0f;
                        dx::extension::mouseMove(static_cast<float>(event.motion.x) * scaleX, static_cast<float>(event.motion.y) * scaleY);
                    } else {
                        dx::console::key(dx::IK_MouseX, dx::IST_Axis, static_cast<float>(event.motion.xrel));
                        dx::console::key(dx::IK_MouseY, dx::IST_Axis, static_cast<float>(-event.motion.yrel));
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    uint8_t key = translateButton(event.button.button);
                    bool pressed = event.type == SDL_MOUSEBUTTONDOWN;
                    if (dx::extension::wantsMouse() && dx::extension::mouseButton(key, pressed)) {
                        break;
                    }
                    handleKey(key, pressed);
                    break;
                }
                case SDL_MOUSEWHEEL: {
                    uint8_t key = event.wheel.y > 0 ? dx::IK_MouseWheelUp : dx::IK_MouseWheelDown;
                    if (event.wheel.y != 0) {
                        dx::console::key(dx::IK_MouseW, dx::IST_Axis, static_cast<float>(event.wheel.y * 120));
                        handleKey(key, true);
                        handleKey(key, false);
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }

        void handleTravel()
        {
            if (!pendingMap.empty()) {
                std::string url = dx::travelUrl(world ? world->url : std::string(), pendingMap, pendingTravelType);
                pendingMap.clear();
                openMap(url);
                return;
            }
            if (world && world->travelPending) {
                std::string url = dx::travelUrl(world->url, world->travelUrl, world->travelType);
                bool items = world->travelItems;
                world->travelPending = false;
                openMap(url, items);
            }
        }

        void render(int width, int height)
        {
            frameWidth = width;
            frameHeight = height;
            dx::extension::setViewportSize(static_cast<float>(width), static_cast<float>(height));
            if (world) {
                renderer->setTime(world->timeSeconds);
            }
            renderer->setViewportTime(viewportTime);
            renderer->beginFrame(width, height);
            viewLocation = {};
            viewRotation = {};
            viewFov = 90;
            if (world && world->calcView(viewLocation, viewRotation, viewFov)) {
                viewFov = displayFov(viewFov, width, height);
                dx::UObject* player = world->player();
                dx::UObject* viewActor = nullptr;
                if (player && !dx::pawn::bBehindView(player)) {
                    viewActor = dx::playerpawn::ViewTarget(player) ? dx::playerpawn::ViewTarget(player) : player;
                }
                renderer->drawScene(world->level, world->actors, viewLocation, viewRotation, viewFov, viewActor);
                dx::UObject* canvasActor = canvas(width, height);
                if (player && !player->destroyed) {
                    dx::vm::Call(player, dx::Name("PreRender")).obj(canvasActor).run();
                    if (viewActor) {
                        dx::vm::Call(viewActor, dx::Name("RenderOverlays")).obj(canvasActor).run();
                    }
                    dx::vm::Call(player, dx::Name("PostRenderFlash")).obj(canvasActor).run();
                    dx::vm::Call(player, dx::Name("PostRender")).obj(canvasActor).run();
                    dx::UObject* console = scriptConsole();
                    if (console) {
                        dx::console::FrameX(console) = static_cast<float>(width);
                        dx::console::FrameY(console) = static_cast<float>(height);
                        dx::vm::Call(console, dx::Name("PostRender")).obj(canvasActor).run();
                    }
                }
            }
            renderer->endFrame();
        }

        void scriptedAction(const std::string& action)
        {
            size_t space = action.find(' ');
            std::string verb = action.substr(0, space);
            std::string rest = space == std::string::npos ? std::string() : action.substr(space + 1);
            if (verb == "press" || verb == "release") {
                uint8_t key = dx::console::keyFromName(rest);
                if (dx::extension::wantsMouse() && (key == dx::IK_LeftMouse || key == dx::IK_RightMouse)) {
                    dx::extension::mouseButton(key, verb == "press");
                } else {
                    handleKey(key, verb == "press");
                }
            } else if (verb == "cursor") {
                float x = 0;
                float y = 0;
                std::sscanf(rest.c_str(), "%f %f", &x, &y);
                dx::extension::mouseMove(x, y);
            } else if (verb == "type") {
                for (char c : rest) {
                    charEvent(c);
                }
            } else if (verb == "ccursor") {
                float x = 0;
                float y = 0;
                std::sscanf(rest.c_str(), "%f %f", &x, &y);
                x += std::floor(static_cast<float>(frameWidth - options.width) * 0.5f);
                y += std::floor(static_cast<float>(frameHeight - options.height) * 0.5f);
                dx::extension::mouseMove(x, y);
            } else if (verb == "mouse") {
                float dx = 0;
                float dy = 0;
                std::sscanf(rest.c_str(), "%f %f", &dx, &dy);
                dx::console::key(dx::IK_MouseX, dx::IST_Axis, dx);
                dx::console::key(dx::IK_MouseY, dx::IST_Axis, -dy);
            } else if (verb == "exec") {
                std::string output = dx::console::execute(world ? world->player() : nullptr, rest);
                if (!output.empty()) {
                    std::printf("%s\n", output.c_str());
                }
            } else if (verb == "shot") {
                pendingShot = rest;
            } else if (verb == "state") {
                dx::UObject* player = world ? world->player() : nullptr;
                if (player) {
                    dx::Vector location = dx::actor::Location(player);
                    std::printf("player at %.1f %.1f %.1f yaw %d state %s health %d\n", static_cast<double>(location.x), static_cast<double>(location.y), static_cast<double>(location.z), dx::actor::Rotation(player).yaw & 65535, dx::vm::stateName(player).c_str(), dx::pawn::Health(player));
                }
            }
        }

        void begin()
        {
            for (const std::string& command : options.commands) {
                dx::console::execute(world ? world->player() : nullptr, command);
            }
            last = SDL_GetPerformanceCounter();
        }

        int run()
        {
            begin();
            while (running) {
                step();
            }
            return 0;
        }

        void step()
        {
            processEvents();
            bool gui = dx::extension::wantsMouse();
            if (gui != guiMode) {
                guiMode = gui;
                SDL_SetRelativeMouseMode(gui ? SDL_FALSE : SDL_TRUE);
                float cursorX = 0;
                float cursorY = 0;
                if (gui && dx::extension::cursorPosition(cursorX, cursorY)) {
                    int windowWidth = 0;
                    int windowHeight = 0;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    int drawWidth = 0;
                    int drawHeight = 0;
                    SDL_GL_GetDrawableSize(window, &drawWidth, &drawHeight);
                    if (drawWidth > 0 && drawHeight > 0) {
                        SDL_WarpMouseInWindow(window, static_cast<int>(cursorX * static_cast<float>(windowWidth) / static_cast<float>(drawWidth)), static_cast<int>(cursorY * static_cast<float>(windowHeight) / static_cast<float>(drawHeight)));
                    }
                }
            }
            uint64_t now = SDL_GetPerformanceCounter();
            float delta = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
            last = now;
            if (options.frames >= 0) {
                delta = 1.0f / 30.0f;
            }
            viewportTime += static_cast<double>(delta);
            applyPlayerSpeed();
            applyRenderOptions();
            if (world) {
                world->tick(delta);
            }
            dx::extension::tick(delta);
            dx::UObject* console = scriptConsole();
            if (console) {
                dx::vm::Call(console, dx::Name("Tick")).f(delta).run();
                int height = 0;
                SDL_GL_GetDrawableSize(window, nullptr, &height);
                dx::console::ConsoleLines(console) = static_cast<int32_t>(std::min(dx::console::ConsolePos(console) * static_cast<float>(height), static_cast<float>(height)));
            }
            handleTravel();
            int width = 0;
            int height = 0;
            SDL_GL_GetDrawableSize(window, &width, &height);
            render(width, height);
            renderer->applyBrightness(static_cast<float>(std::atof(dx::config::system().get(videoSection(), "Brightness").value_or("0.5").c_str())));
            dx::audio::update(viewLocation, viewRotation, delta);
            if (!pendingShot.empty()) {
                std::vector<uint8_t> pixels;
                int shotWidth = 0;
                int shotHeight = 0;
                renderer->readPixels(pixels, shotWidth, shotHeight);
                saveBmp(pendingShot, pixels, shotWidth, shotHeight);
                pendingShot.clear();
            }
            frame++;
            for (const auto& [actionFrame, action] : options.actions) {
                if (actionFrame == frame) {
                    scriptedAction(action);
                }
            }
            if (options.frames >= 0 && frame >= options.frames) {
                if (!options.screenshot.empty()) {
                    std::vector<uint8_t> pixels;
                    int shotWidth = 0;
                    int shotHeight = 0;
                    renderer->readPixels(pixels, shotWidth, shotHeight);
                    saveBmp(options.screenshot, pixels, shotWidth, shotHeight);
                }
                running = false;
            }
            SDL_GL_SwapWindow(window);
            limitFrameRate();
        }

        bool isRunning() const
        {
            return running;
        }

    private:
        SDL_Window* window;
        Options options;
        std::unique_ptr<dx::SceneRenderer> renderer;
        std::unique_ptr<dx::World> world;
        dx::UObject* canvasObject = nullptr;
        std::string pendingMap;
        int32_t pendingTravelType = dx::TRAVEL_Partial;
        std::string pendingShot;
        dx::Vector viewLocation;
        dx::Rotator viewRotation;
        float viewFov = 90;
        int frameWidth = 1;
        double viewportTime = 0;
        int frameHeight = 1;
        bool running = true;
        uint64_t last = 0;
        int frame = 0;
        bool guiMode = true;
        float appliedPlayerSpeed = 1.0f;
        int appliedVSync = -1;
        uint64_t frameDeadline = 0;
        std::unordered_map<dx::UClass*, std::pair<float, float>> basePlayerSpeeds;
    };
}

int main(int argc, char** argv)
{
    Options options = parseOptions(argc, argv);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* window = SDL_CreateWindow("Deus Ex", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, options.width, options.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
        std::fprintf(stderr, "failed to load OpenGL\n");
        return 1;
    }
    SDL_GL_SetSwapInterval(options.frames >= 0 ? 0 : 1);
    SDL_StartTextInput();
    SDL_ShowCursor(SDL_DISABLE);
    dx::vm::setCriticalHandler([window](const std::string& message) {
        std::fprintf(stderr, "Critical Error: %s\n", message.c_str());
        std::fflush(stderr);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Critical Error", message.c_str(), window);
        std::_Exit(1);
    });
#ifdef __EMSCRIPTEN__
    try {
        Game* game = new Game(window, options);
        if (!game->start()) {
            std::fprintf(stderr, "failed to start game\n");
            return 1;
        }
        game->begin();
        emscripten_set_click_callback("#fullscreen", nullptr, true, [](int, const EmscriptenMouseEvent*, void*) -> EM_BOOL {
            dx::console::toggleFullscreenHook();
            return true;
        });
        emscripten_set_main_loop_arg(
            [](void* argument) {
                Game* running = static_cast<Game*>(argument);
                try {
                    running->step();
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "error: %s\n", error.what());
                    emscripten_cancel_main_loop();
                    return;
                }
                if (!running->isRunning()) {
                    emscripten_cancel_main_loop();
                }
            },
            game, 0, true);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "error: %s\n", error.what());
        return 1;
    }
    return 0;
#else
    int result = 0;
    try {
        Game game(window, options);
        if (!game.start()) {
            std::fprintf(stderr, "failed to start game\n");
            result = 1;
        } else {
            result = game.run();
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "error: %s\n", error.what());
        result = 1;
    }
    dx::audio::shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
#endif
}
