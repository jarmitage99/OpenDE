#include "editor/editor.h"

#include <tabler_icons.h>

namespace {
    dx::editor::Options parseOptions(int argc, char** argv)
    {
        dx::editor::Options options;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--screenshot" && i + 1 < argc) {
                options.screenshot = argv[++i];
            } else if (arg == "--frames" && i + 1 < argc) {
                options.frames = std::atoi(argv[++i]);
            } else if (arg == "--new" && i + 1 < argc) {
                options.blankMap = argv[++i];
            } else if (arg == "--select" && i + 1 < argc) {
                options.select = argv[++i];
            } else if (arg == "--width" && i + 1 < argc) {
                options.width = std::atoi(argv[++i]);
            } else if (arg == "--height" && i + 1 < argc) {
                options.height = std::atoi(argv[++i]);
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
}

int main(int argc, char** argv)
{
    dx::editor::Options options = parseOptions(argc, argv);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* window = SDL_CreateWindow("Deus Ex UnrealEd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, options.width, options.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context || !gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
        std::fprintf(stderr, "failed to create OpenGL 3.3 context\n");
        return 1;
    }
    SDL_GL_SetSwapInterval(options.frames > 0 ? 0 : 1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::GetIO().Fonts->AddFontDefault();
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.GlyphMinAdvanceX = 13.0f;
    iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(TablerIcons_compressed_data, static_cast<int>(TablerIcons_compressed_size), 13.0f, &iconConfig);
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    std::filesystem::path gamePath = "deusex";
    if (char* base = SDL_GetBasePath()) {
        gamePath = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(base))) / "deusex";
        SDL_free(base);
    }
    int result = 0;
    try {
        dx::editor::Editor editor(window, options, gamePath);
        editor.start();
        editor.run();
        if (editor.passedExpectations + editor.failedExpectations > 0) {
            std::printf("expectations: %d passed, %d failed\n", editor.passedExpectations, editor.failedExpectations);
        }
        result = editor.failedExpectations > 0 ? 1 : 0;
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "error: %s\n", failure.what());
        result = 1;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
