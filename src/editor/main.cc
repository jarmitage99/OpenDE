#include "core/config.h"
#include "core/objects.h"
#include "engine/actor.h"
#include "engine/brush_builder.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/editor_history.h"
#include "engine/editor_ops.h"
#include "engine/extra_builders.h"
#include "engine/lightmap_builder.h"
#include "engine/map_writer.h"
#include "engine/path_builder.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "render/scene_renderer.h"
#include "render/texture_cache.h"

#include <SDL.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef _WIN32
extern char** environ;
#endif

namespace {
    constexpr uint32_t EditorActorFlags = dx::RF_Transactional | 0x00070000;

    struct Options {
        std::string map = "01_NYC_UNATCOIsland";
        std::string screenshot;
        std::string select;
        std::vector<std::pair<int, std::string>> actions;
        int frames = -1;
        int width = 1600;
        int height = 900;
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

    void saveBmp(const std::string& path, std::vector<uint8_t>& rgba, int width, int height)
    {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(rgba.data(), width, height, 32, width * 4, SDL_PIXELFORMAT_ABGR8888);
        SDL_SaveBMP(surface, path.c_str());
        SDL_FreeSurface(surface);
    }

    std::string lower(const std::string& text)
    {
        std::string result = text;
        for (char& c : result) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    std::filesystem::path userMapsDirectory()
    {
        return dx::config::userDirectory() / "Maps";
    }

    ImGuiKey keyNamed(const std::string& name)
    {
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
            if (lower(ImGui::GetKeyName(static_cast<ImGuiKey>(key))) == lower(name)) {
                return static_cast<ImGuiKey>(key);
            }
        }
        return ImGuiKey_None;
    }

    ImGuiKey modifierFor(ImGuiKey key)
    {
        if (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift) {
            return ImGuiMod_Shift;
        }
        if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl) {
            return ImGuiMod_Ctrl;
        }
        if (key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt) {
            return ImGuiMod_Alt;
        }
        return ImGuiKey_None;
    }

    struct Edge {
        dx::Vector a;
        dx::Vector b;
    };

    struct OrthoView {
        const char* title;
        int horizontal;
        int vertical;
        float verticalSign;
        float centerH = 0;
        float centerV = 0;
        float zoom = 0.08f;
    };

    struct SpecialPreset {
        std::string name;
        uint32_t flags = 0;
    };

    struct InputEvent {
        int frame = 0;
        int type = 0;
        float x = 0;
        float y = 0;
        int button = 0;
        bool down = false;
        ImGuiKey key = ImGuiKey_None;
        std::string view;
    };

    float component(const dx::Vector& v, int axis)
    {
        if (axis == 0) {
            return v.x;
        }
        if (axis == 1) {
            return v.y;
        }
        return v.z;
    }

    float& componentRef(dx::Vector& v, int axis)
    {
        if (axis == 0) {
            return v.x;
        }
        if (axis == 1) {
            return v.y;
        }
        return v.z;
    }

    class Editor {
    public:
        Editor(SDL_Window* target, const Options& settings, const std::filesystem::path& gameExecutable)
            : window(target)
            , options(settings)
            , gamePath(gameExecutable)
        {
            orthoViews[0] = { "Top", 0, 1, 1.0f };
            orthoViews[1] = { "Front", 0, 2, -1.0f };
            orthoViews[2] = { "Side", 1, 2, -1.0f };
        }

        ~Editor()
        {
            releaseFramebuffer();
        }

        Editor(const Editor&) = delete;
        Editor& operator=(const Editor&) = delete;

        void start()
        {
            dx::objects::setEditorMode(true);
            std::filesystem::path gameRoot = dx::config::findGameRoot(DEUSEX_GAME_DATA);
            dx::engine::initialize(gameRoot.string());
            renderer = std::make_unique<dx::SceneRenderer>();
            for (const auto& entry : std::filesystem::directory_iterator(gameRoot / "Maps")) {
                if (entry.path().extension() == ".dx") {
                    mapNames.push_back(entry.path().stem().string());
                }
            }
            std::sort(mapNames.begin(), mapNames.end());
            for (const auto& entry : std::filesystem::directory_iterator(gameRoot / "Textures")) {
                if (lower(entry.path().extension().string()) == ".utx") {
                    texturePackages.push_back(entry.path().stem().string());
                }
            }
            std::sort(texturePackages.begin(), texturePackages.end());
            buildClassTree();
            std::filesystem::path mapFile = options.map;
            if (mapFile.has_parent_path() && std::filesystem::is_regular_file(mapFile)) {
                openMapFile(mapFile);
            } else {
                openMap(options.map);
            }
            if (!options.select.empty()) {
                command("select " + options.select);
            }
            for (const auto& [frame, action] : options.actions) {
                scheduleAction(frame, action);
            }
        }

        bool openMapFile(const std::filesystem::path& file)
        {
            std::string name = file.stem().string();
            if (!dx::objects::loadPackageFile(dx::Name(name), file, false)) {
                status = "cannot open " + file.string();
                return false;
            }
            if (!openMap(name)) {
                return false;
            }
            currentFile = file;
            return true;
        }

        bool openMap(const std::string& mapName)
        {
            dx::Linker* linker = dx::objects::loadPackage(dx::Name(mapName));
            if (!linker) {
                status = "map not found: " + mapName;
                std::fprintf(stderr, "%s\n", status.c_str());
                return false;
            }
            int32_t ref = linker->findExport(dx::Name("MyLevel"), dx::Name("Level"), "*");
            dx::ULevel* newLevel = dx::cast<dx::ULevel>(linker->object(ref));
            if (!newLevel) {
                status = "no level in " + mapName;
                std::fprintf(stderr, "%s\n", status.c_str());
                return false;
            }
            return useLevel(newLevel, mapName);
        }

        bool useLevel(dx::ULevel* newLevel, const std::string& mapName)
        {
            renderer->flushLevel();
            if (level && level != newLevel) {
                dx::objects::unloadPackage(level->outermost()->name);
            }
            level = newLevel;
            currentMap = mapName;
            currentFile.clear();
            selected = nullptr;
            selectedSurfs.clear();
            history.clear();
            savedRegions.clear();
            dx::objects::ensureLoaded(level);
            dx::objects::flushLoads();
            for (dx::UObject* actor : level->actors) {
                if (actor) {
                    dx::objects::ensureLoaded(actor);
                }
            }
            dx::objects::flushLoads();
            for (dx::UObject* actor : level->actors) {
                if (actor && actor->data && actor->isA(dx::Name("Actor"))) {
                    std::array<uint8_t, sizeof(dx::PointRegion)> region {};
                    std::memcpy(region.data(), dx::actor::Region(actor), sizeof(dx::PointRegion));
                    savedRegions.emplace(actor, region);
                    *reinterpret_cast<dx::UObject**>(dx::actor::Region(actor)) = nullptr;
                }
            }
            refreshActors();
            dx::objects::ensureLoaded(level->model);
            dx::objects::flushLoads();
            buildEdges();
            brushCache.clear();
            placeCamera();
            status = "opened " + mapName;
            return true;
        }

        void refreshActors()
        {
            actors.clear();
            for (dx::UObject* actor : level->actors) {
                if (actor && !actor->destroyed && actor->data && actor->isA(dx::Name("Actor"))) {
                    actors.push_back(actor);
                }
            }
            if (selected && (selected->destroyed || std::find(actors.begin(), actors.end(), selected) == actors.end())) {
                selected = nullptr;
            }
        }

        void placeCamera()
        {
            cameraLocation = {};
            cameraRotation = {};
            for (dx::UObject* actor : actors) {
                if (actor->isA(dx::Name("PlayerStart"))) {
                    cameraLocation = dx::actor::Location(actor) + dx::Vector { 0, 0, 40 };
                    cameraRotation = dx::actor::Rotation(actor);
                    cameraRotation.roll = 0;
                    break;
                }
            }
            for (OrthoView& view : orthoViews) {
                view.centerH = component(cameraLocation, view.horizontal);
                view.centerV = component(cameraLocation, view.vertical);
                view.zoom = 0.08f;
            }
        }

        void focusSelected()
        {
            if (!selected) {
                return;
            }
            dx::Vector target = dx::actor::Location(selected);
            cameraLocation = target - dx::rotationVector(cameraRotation) * 256.0f;
            for (OrthoView& view : orthoViews) {
                view.centerH = component(target, view.horizontal);
                view.centerV = component(target, view.vertical);
            }
        }

        void select(dx::UObject* actor)
        {
            if (selected != actor) {
                editingKey = nullptr;
            }
            selected = actor;
            selectionChanged = true;
        }

        void buildEdges()
        {
            edges.clear();
            dx::UModel* model = level->model;
            if (!model) {
                return;
            }
            std::unordered_set<uint64_t> seen;
            for (const dx::BspNode& node : model->nodes) {
                int32_t count = node.numVertices;
                for (int32_t i = 0; i < count; i++) {
                    size_t first = static_cast<size_t>(node.vertPool + i);
                    size_t second = static_cast<size_t>(node.vertPool + (i + 1) % count);
                    if (first >= model->verts.size() || second >= model->verts.size()) {
                        continue;
                    }
                    uint32_t a = static_cast<uint32_t>(model->verts[first].vertex);
                    uint32_t b = static_cast<uint32_t>(model->verts[second].vertex);
                    if (a >= model->points.size() || b >= model->points.size()) {
                        continue;
                    }
                    uint64_t key = a < b ? (static_cast<uint64_t>(a) << 32) | b : (static_cast<uint64_t>(b) << 32) | a;
                    if (seen.insert(key).second) {
                        edges.push_back({ model->points[a], model->points[b] });
                    }
                }
            }
        }

        void buildClassTree()
        {
            for (dx::UObject* object : dx::objects::all()) {
                dx::UClass* cls = dx::cast<dx::UClass>(object);
                if (!cls || object->destroyed) {
                    continue;
                }
                dx::UClass* parent = cls->superClass();
                if (parent) {
                    classChildren[parent].push_back(cls);
                }
            }
            for (auto& entry : classChildren) {
                std::sort(entry.second.begin(), entry.second.end(), [](dx::UClass* a, dx::UClass* b) {
                    return a->name.str() < b->name.str();
                });
            }
            actorClass = dx::objects::findClass("Engine.Actor");
        }

        std::vector<Edge> brushEdges(dx::UObject* actor)
        {
            std::vector<Edge> result;
            dx::UModel* model = dx::cast<dx::UModel>(dx::actor::Brush(actor));
            if (!model) {
                return result;
            }
            dx::objects::ensureLoaded(model);
            if (model->polys) {
                dx::objects::ensureLoaded(model->polys);
            }
            dx::objects::flushLoads();
            if (!model->polys) {
                return result;
            }
            dx::Coords axes = dx::rotationAxes(dx::actor::Rotation(actor));
            dx::Vector location = dx::actor::Location(actor);
            dx::Vector pivot = dx::actor::PrePivot(actor);
            dx::Vector mainScale = *reinterpret_cast<dx::Vector*>(dx::brush::MainScale(actor));
            dx::Vector postScale = *reinterpret_cast<dx::Vector*>(dx::brush::PostScale(actor));
            auto transform = [&](const dx::Vector& v) {
                dx::Vector local = (v - pivot) * mainScale;
                dx::Vector rotated = axes.xAxis * local.x + axes.yAxis * local.y + axes.zAxis * local.z;
                return location + rotated * postScale;
            };
            for (const dx::Poly& poly : model->polys->polys) {
                size_t count = poly.vertices.size();
                for (size_t i = 0; i < count; i++) {
                    result.push_back({ transform(poly.vertices[i]), transform(poly.vertices[(i + 1) % count]) });
                }
            }
            return result;
        }

        const std::vector<Edge>& cachedBrushEdges(dx::UObject* actor)
        {
            auto found = brushCache.find(actor);
            if (found == brushCache.end() || actor == selected) {
                found = brushCache.insert_or_assign(actor, brushEdges(actor)).first;
            }
            return found->second;
        }

        ImU32 brushColor(dx::UObject* actor)
        {
            if (actor->isA(dx::Name("Mover"))) {
                return IM_COL32(200, 80, 255, 255);
            }
            if (actor == builderBrush()) {
                return IM_COL32(255, 40, 40, 255);
            }
            if (dx::brush::CsgOper(actor) == dx::csg::CsgSubtract) {
                return IM_COL32(255, 160, 40, 255);
            }
            return IM_COL32(80, 140, 255, 255);
        }

        dx::UObject* builderBrush()
        {
            for (dx::UObject* actor : actors) {
                if (actor->cls->name == dx::Name("Brush") && dx::brush::CsgOper(actor) == dx::csg::CsgActive) {
                    return actor;
                }
            }
            for (dx::UObject* actor : actors) {
                if (actor->cls->name == dx::Name("Brush")) {
                    return actor;
                }
            }
            return nullptr;
        }

        bool isBrush(dx::UObject* actor)
        {
            return actor->isA(dx::Name("Brush")) && dx::actor::Brush(actor);
        }

        void ensureFramebuffer(int width, int height)
        {
            if (framebuffer && width == fbWidth && height == fbHeight) {
                return;
            }
            releaseFramebuffer();
            fbWidth = width;
            fbHeight = height;
            glGenFramebuffers(1, &framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
            glGenRenderbuffers(1, &depthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void releaseFramebuffer()
        {
            if (framebuffer) {
                glDeleteFramebuffers(1, &framebuffer);
                glDeleteTextures(1, &colorTexture);
                glDeleteRenderbuffers(1, &depthBuffer);
                framebuffer = 0;
                colorTexture = 0;
                depthBuffer = 0;
            }
        }

        void renderPerspective(int width, int height)
        {
            ensureFramebuffer(width, height);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            std::vector<dx::UObject*> shown;
            for (dx::UObject* actor : actors) {
                if (dx::actor::bHidden(actor) && dx::actor::DrawType(actor) == dx::DT_Sprite && dx::actor::Texture(actor)) {
                    shown.push_back(actor);
                    dx::actor::bHidden(actor) = 0;
                }
            }
            renderer->beginFrame(width, height);
            renderer->drawScene(level, actors, cameraLocation, cameraRotation, fov, nullptr);
            renderer->endFrame();
            for (dx::UObject* actor : shown) {
                dx::actor::bHidden(actor) = 1;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        bool projectCamera(const dx::Vector& point, dx::Vector& out)
        {
            dx::Coords axes = dx::rotationAxes(cameraRotation);
            dx::Vector delta = point - cameraLocation;
            out = { delta.dot(axes.yAxis), delta.dot(axes.zAxis), delta.dot(axes.xAxis) };
            return out.z > 1.0f;
        }

        ImVec2 cameraToScreen(const dx::Vector& camera, const ImVec2& origin, const ImVec2& size)
        {
            float scale = size.x * 0.5f / std::tan(fov * dx::Pi / 360.0f);
            return ImVec2(origin.x + size.x * 0.5f + camera.x / camera.z * scale, origin.y + size.y * 0.5f - camera.y / camera.z * scale);
        }

        void drawPerspectiveLine(ImDrawList* list, const dx::Vector& a, const dx::Vector& b, const ImVec2& origin, const ImVec2& size, ImU32 color)
        {
            dx::Vector ca;
            dx::Vector cb;
            bool va = projectCamera(a, ca);
            bool vb = projectCamera(b, cb);
            if (!va && !vb) {
                return;
            }
            if (!va || !vb) {
                float t = (1.0f - ca.z) / (cb.z - ca.z);
                dx::Vector clipped = ca + (cb - ca) * t;
                if (!va) {
                    ca = clipped;
                } else {
                    cb = clipped;
                }
            }
            list->AddLine(cameraToScreen(ca, origin, size), cameraToScreen(cb, origin, size), color, 1.5f);
        }

        void drawCylinder(ImDrawList* list, dx::UObject* actor, const ImVec2& origin, const ImVec2& size)
        {
            dx::Vector center = dx::actor::Location(actor);
            float radius = std::max(dx::actor::CollisionRadius(actor), 8.0f);
            float height = std::max(dx::actor::CollisionHeight(actor), 8.0f);
            ImU32 color = IM_COL32(255, 255, 0, 255);
            const int segments = 16;
            for (int i = 0; i < segments; i++) {
                float a0 = static_cast<float>(i) * 2.0f * dx::Pi / segments;
                float a1 = static_cast<float>(i + 1) * 2.0f * dx::Pi / segments;
                dx::Vector p0 { center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius, 0 };
                dx::Vector p1 { center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius, 0 };
                dx::Vector top0 { p0.x, p0.y, center.z + height };
                dx::Vector top1 { p1.x, p1.y, center.z + height };
                dx::Vector bottom0 { p0.x, p0.y, center.z - height };
                dx::Vector bottom1 { p1.x, p1.y, center.z - height };
                drawPerspectiveLine(list, top0, top1, origin, size, color);
                drawPerspectiveLine(list, bottom0, bottom1, origin, size, color);
                if (i % 4 == 0) {
                    drawPerspectiveLine(list, top0, bottom0, origin, size, color);
                }
            }
        }

        dx::Vector rayDirection(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size)
        {
            float scale = size.x * 0.5f / std::tan(fov * dx::Pi / 360.0f);
            float right = (mouse.x - origin.x - size.x * 0.5f) / scale;
            float up = -(mouse.y - origin.y - size.y * 0.5f) / scale;
            dx::Coords axes = dx::rotationAxes(cameraRotation);
            return (axes.xAxis + axes.yAxis * right + axes.zAxis * up).normal();
        }

        dx::UObject* pickPerspective(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size)
        {
            dx::Vector direction = rayDirection(mouse, origin, size);
            dx::UObject* best = nullptr;
            float bestT = 1e9f;
            for (dx::UObject* actor : actors) {
                if (actor->isA(dx::Name("Brush")) || actor->isA(dx::Name("ZoneInfo"))) {
                    continue;
                }
                dx::Vector center = dx::actor::Location(actor);
                float radius = std::max(dx::actor::CollisionRadius(actor), 8.0f);
                float height = std::max(dx::actor::CollisionHeight(actor), 8.0f);
                dx::Vector delta = center - cameraLocation;
                float dx2 = direction.x * direction.x + direction.y * direction.y;
                if (dx2 < 1e-6f) {
                    continue;
                }
                float b = delta.x * direction.x + delta.y * direction.y;
                float c = delta.x * delta.x + delta.y * delta.y - radius * radius;
                float disc = b * b - dx2 * c;
                if (disc < 0) {
                    continue;
                }
                float t = (b - std::sqrt(disc)) / dx2;
                if (t < 0) {
                    t = (b + std::sqrt(disc)) / dx2;
                }
                if (t < 0 || t >= bestT) {
                    continue;
                }
                float z = cameraLocation.z + direction.z * t;
                if (z < center.z - height || z > center.z + height) {
                    continue;
                }
                bestT = t;
                best = actor;
            }
            return best;
        }

        int32_t pickSurface(const dx::Vector& direction)
        {
            dx::UModel* model = level ? level->model : nullptr;
            if (!model) {
                return -1;
            }
            int32_t best = -1;
            float bestT = 1e9f;
            for (const dx::BspNode& node : model->nodes) {
                if (node.numVertices < 3 || node.surf < 0) {
                    continue;
                }
                dx::Vector normal { node.plane.x, node.plane.y, node.plane.z };
                float facing = normal.dot(direction);
                if (facing >= 0) {
                    continue;
                }
                float t = (node.plane.w - normal.dot(cameraLocation)) / facing;
                if (t <= 0 || t >= bestT) {
                    continue;
                }
                dx::Vector hit = cameraLocation + direction * t;
                bool inside = true;
                for (int32_t i = 0; i < node.numVertices; i++) {
                    const dx::Vector& a = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                    const dx::Vector& b = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
                    if ((b - a).cross(hit - a).dot(normal) < -0.01f) {
                        inside = false;
                        break;
                    }
                }
                if (inside) {
                    bestT = t;
                    best = node.surf;
                }
            }
            return best;
        }

        void drawSelectedSurfaces(ImDrawList* list, const ImVec2& origin, const ImVec2& size)
        {
            dx::UModel* model = level ? level->model : nullptr;
            if (!model || selectedSurfs.empty()) {
                return;
            }
            for (const dx::BspNode& node : model->nodes) {
                if (node.numVertices < 3 || !selectedSurfs.count(node.surf)) {
                    continue;
                }
                for (int32_t i = 0; i < node.numVertices; i++) {
                    const dx::Vector& a = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                    const dx::Vector& b = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
                    drawPerspectiveLine(list, a, b, origin, size, IM_COL32(80, 255, 80, 255));
                }
            }
        }

        void perspectiveWindow(const ImVec2& position, const ImVec2& size)
        {
            ImGui::SetNextWindowPos(position);
            ImGui::SetNextWindowSize(size);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Perspective", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImVec2 area = ImGui::GetContentRegionAvail();
            perspectiveOrigin = origin;
            perspectiveSize = area;
            viewRects["perspective"] = { origin, area };
            int width = std::max(static_cast<int>(area.x), 1);
            int height = std::max(static_cast<int>(area.y), 1);
            renderPerspective(width, height);
            ImGui::Image(static_cast<ImTextureID>(colorTexture), ImVec2(static_cast<float>(width), static_cast<float>(height)), ImVec2(0, 1), ImVec2(1, 0));
            bool hovered = ImGui::IsItemHovered();
            ImDrawList* list = ImGui::GetWindowDrawList();
            if (selected) {
                if (isBrush(selected)) {
                    for (const Edge& edge : brushEdges(selected)) {
                        drawPerspectiveLine(list, edge.a, edge.b, origin, area, IM_COL32(255, 255, 0, 255));
                    }
                } else {
                    drawCylinder(list, selected, origin, area);
                }
            }
            drawSelectedSurfaces(list, origin, area);
            char text[160];
            std::snprintf(text, sizeof(text), "%.0f %.0f %.0f  speed %.0f%s", static_cast<double>(cameraLocation.x), static_cast<double>(cameraLocation.y), static_cast<double>(cameraLocation.z), static_cast<double>(moveSpeed), surfaceMode ? "  surface mode" : "");
            list->AddText(ImVec2(origin.x + 6, origin.y + 4), IM_COL32(255, 255, 255, 220), text);
            ImGuiIO& io = ImGui::GetIO();
            if (hovered) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (surfaceMode || io.KeyShift) {
                        int32_t surf = pickSurface(rayDirection(io.MousePos, origin, area));
                        if (!io.KeyCtrl) {
                            selectedSurfs.clear();
                        }
                        if (surf >= 0) {
                            selectedSurfs.insert(surf);
                        }
                        status = "selected " + std::to_string(selectedSurfs.size()) + " surfaces";
                    } else {
                        select(pickPerspective(io.MousePos, origin, area));
                    }
                }
                if (io.MouseWheel != 0) {
                    moveSpeed = std::clamp(moveSpeed * (io.MouseWheel > 0 ? 1.25f : 0.8f), 32.0f, 8192.0f);
                }
                if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                    cameraRotation.yaw += static_cast<int32_t>(io.MouseDelta.x * 40.0f);
                    cameraRotation.pitch = std::clamp(cameraRotation.pitch - static_cast<int32_t>(io.MouseDelta.y * 40.0f), -16000, 16000);
                }
                if (!io.KeyCtrl) {
                    dx::Coords axes = dx::rotationAxes(cameraRotation);
                    dx::Vector move;
                    if (ImGui::IsKeyDown(ImGuiKey_W)) {
                        move += axes.xAxis;
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_S)) {
                        move -= axes.xAxis;
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_D)) {
                        move += axes.yAxis;
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_A)) {
                        move -= axes.yAxis;
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_E)) {
                        move += dx::Vector { 0, 0, 1 };
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
                        move -= dx::Vector { 0, 0, 1 };
                    }
                    cameraLocation += move * (moveSpeed * io.DeltaTime);
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        ImVec2 orthoToScreen(const OrthoView& view, const dx::Vector& point, const ImVec2& origin, const ImVec2& size)
        {
            float h = (component(point, view.horizontal) - view.centerH) * view.zoom;
            float v = (component(point, view.vertical) - view.centerV) * view.zoom * view.verticalSign;
            return ImVec2(origin.x + size.x * 0.5f + h, origin.y + size.y * 0.5f + v);
        }

        void orthoWindow(OrthoView& view, const ImVec2& position, const ImVec2& size)
        {
            ImGui::SetNextWindowPos(position);
            ImGui::SetNextWindowSize(size);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin(view.title, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImVec2 area = ImGui::GetContentRegionAvail();
            area.x = std::max(area.x, 1.0f);
            area.y = std::max(area.y, 1.0f);
            viewRects[lower(view.title)] = { origin, area };
            ImGui::InvisibleButton("canvas", area, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            bool hovered = ImGui::IsItemHovered();
            ImDrawList* list = ImGui::GetWindowDrawList();
            ImVec2 end(origin.x + area.x, origin.y + area.y);
            list->PushClipRect(origin, end, true);
            list->AddRectFilled(origin, end, IM_COL32(20, 20, 24, 255));
            float grid = 128.0f;
            while (grid * view.zoom < 12.0f) {
                grid *= 2.0f;
            }
            float halfH = area.x * 0.5f / view.zoom;
            float halfV = area.y * 0.5f / view.zoom;
            for (float g = std::floor((view.centerH - halfH) / grid) * grid; g <= view.centerH + halfH; g += grid) {
                float sx = origin.x + area.x * 0.5f + (g - view.centerH) * view.zoom;
                list->AddLine(ImVec2(sx, origin.y), ImVec2(sx, end.y), std::fmod(std::fabs(g), grid * 8.0f) < 0.5f ? IM_COL32(70, 70, 80, 255) : IM_COL32(40, 40, 48, 255));
            }
            for (float g = std::floor((view.centerV - halfV) / grid) * grid; g <= view.centerV + halfV; g += grid) {
                float sy = origin.y + area.y * 0.5f + (g - view.centerV) * view.zoom * view.verticalSign;
                list->AddLine(ImVec2(origin.x, sy), ImVec2(end.x, sy), std::fmod(std::fabs(g), grid * 8.0f) < 0.5f ? IM_COL32(70, 70, 80, 255) : IM_COL32(40, 40, 48, 255));
            }
            for (const Edge& edge : edges) {
                ImVec2 a = orthoToScreen(view, edge.a, origin, area);
                ImVec2 b = orthoToScreen(view, edge.b, origin, area);
                if ((a.x < origin.x && b.x < origin.x) || (a.x > end.x && b.x > end.x) || (a.y < origin.y && b.y < origin.y) || (a.y > end.y && b.y > end.y)) {
                    continue;
                }
                list->AddLine(a, b, IM_COL32(150, 150, 150, 255));
            }
            for (dx::UObject* actor : actors) {
                if (!isBrush(actor)) {
                    continue;
                }
                ImU32 color = actor == selected ? IM_COL32(255, 255, 0, 255) : brushColor(actor);
                for (const Edge& edge : cachedBrushEdges(actor)) {
                    list->AddLine(orthoToScreen(view, edge.a, origin, area), orthoToScreen(view, edge.b, origin, area), color);
                }
            }
            for (dx::UObject* actor : actors) {
                if (isBrush(actor)) {
                    continue;
                }
                ImVec2 p = orthoToScreen(view, dx::actor::Location(actor), origin, area);
                if (p.x < origin.x || p.x > end.x || p.y < origin.y || p.y > end.y) {
                    continue;
                }
                if (actor == selected) {
                    list->AddRectFilled(ImVec2(p.x - 4, p.y - 4), ImVec2(p.x + 4, p.y + 4), IM_COL32(255, 255, 0, 255));
                    float radius = dx::actor::CollisionRadius(actor) * view.zoom;
                    if (radius > 3.0f) {
                        list->AddCircle(p, radius, IM_COL32(255, 255, 0, 255));
                    }
                } else {
                    list->AddRectFilled(ImVec2(p.x - 2, p.y - 2), ImVec2(p.x + 2, p.y + 2), IM_COL32(255, 90, 90, 255));
                }
            }
            char label[96];
            std::snprintf(label, sizeof(label), "%s  %.0f %.0f", view.title, static_cast<double>(view.centerH), static_cast<double>(view.centerV));
            list->AddText(ImVec2(origin.x + 6, origin.y + 4), IM_COL32(255, 255, 255, 220), label);
            list->PopClipRect();
            ImGuiIO& io = ImGui::GetIO();
            if (hovered) {
                if (io.MouseWheel != 0) {
                    view.zoom = std::clamp(view.zoom * (io.MouseWheel > 0 ? 1.25f : 0.8f), 0.002f, 8.0f);
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    dx::UObject* best = nullptr;
                    float bestDistance = 64.0f;
                    for (dx::UObject* actor : actors) {
                        ImVec2 p = orthoToScreen(view, dx::actor::Location(actor), origin, area);
                        float distance = (p.x - io.MousePos.x) * (p.x - io.MousePos.x) + (p.y - io.MousePos.y) * (p.y - io.MousePos.y);
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            best = actor;
                        }
                    }
                    select(best);
                    dragging = best != nullptr;
                    if (dragging) {
                        dragStart = dx::actor::Location(best);
                        dragRaw = dragStart;
                        history.begin(level, { best }, "Move " + best->name.str());
                    }
                }
            }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
                view.centerH -= io.MouseDelta.x / view.zoom;
                view.centerV -= io.MouseDelta.y / view.zoom * view.verticalSign;
            }
            if (ImGui::IsItemActive() && dragging && selected && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
                componentRef(dragRaw, view.horizontal) += io.MouseDelta.x / view.zoom;
                componentRef(dragRaw, view.vertical) += io.MouseDelta.y / view.zoom * view.verticalSign;
                dx::actor::Location(selected) = snapped(dragRaw);
                brushCache.erase(selected);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        void finishDrag()
        {
            if (dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                dragging = false;
                if (history.recording()) {
                    if (selected && !(dx::actor::Location(selected) == dragStart)) {
                        history.commit();
                    } else {
                        history.cancel();
                    }
                }
            }
        }

        void moveSelectionWithKeys()
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantTextInput) {
                return;
            }
            if (io.KeyCtrl) {
                if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                    command("undo");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                    command("redo");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                    command("save");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                    command("duplicate");
                }
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
                command("delete");
            }
            if (!selected) {
                return;
            }
            dx::Vector step;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                step.x -= 16.0f;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                step.x += 16.0f;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                step.y -= 16.0f;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                step.y += 16.0f;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
                step.z += 16.0f;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
                step.z -= 16.0f;
            }
            if (!step.zero()) {
                char text[96];
                std::snprintf(text, sizeof(text), "move %g %g %g", static_cast<double>(step.x), static_cast<double>(step.y), static_cast<double>(step.z));
                command(text);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
                command("rotate 0 -2048 0");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
                command("rotate 0 2048 0");
            }
        }

        void actorBrowser()
        {
            ImGui::InputText("Filter", filter.data(), filter.size());
            std::string needle = lower(filter.data());
            std::map<std::string, std::vector<dx::UObject*>> groups;
            for (dx::UObject* actor : actors) {
                std::string className = actor->cls->name.str();
                if (!needle.empty() && lower(className).find(needle) == std::string::npos && lower(actor->name.str()).find(needle) == std::string::npos) {
                    continue;
                }
                groups[className].push_back(actor);
            }
            ImGui::BeginChild("actors");
            for (auto& group : groups) {
                std::string header = group.first + " (" + std::to_string(group.second.size()) + ")";
                bool containsSelection = selected && std::find(group.second.begin(), group.second.end(), selected) != group.second.end();
                if (containsSelection && selectionChanged) {
                    ImGui::SetNextItemOpen(true);
                }
                if (!ImGui::TreeNode(group.first.c_str(), "%s", header.c_str())) {
                    continue;
                }
                for (dx::UObject* actor : group.second) {
                    bool isSelected = actor == selected;
                    if (ImGui::Selectable(actor->name.str().c_str(), isSelected)) {
                        select(actor);
                        focusSelected();
                    }
                    if (isSelected && selectionChanged) {
                        ImGui::SetScrollHereY();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::EndChild();
        }

        void classNode(dx::UClass* cls)
        {
            auto found = classChildren.find(cls);
            bool leaf = found == classChildren.end();
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            if (leaf) {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            if (cls == actorClass) {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            if (cls == placeClass) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            bool open = ImGui::TreeNodeEx(cls, flags, "%s", cls->name.str().c_str());
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                placeClass = cls;
            }
            if (open && !leaf) {
                for (dx::UClass* child : found->second) {
                    classNode(child);
                }
                ImGui::TreePop();
            }
        }

        void classBrowser()
        {
            if (placeClass) {
                std::string label = "Add " + placeClass->name.str() + " here";
                if (ImGui::Button(label.c_str())) {
                    command("add " + placeClass->pathName());
                }
                ImGui::SameLine();
                if (ImGui::Button("Show")) {
                    std::snprintf(filter.data(), filter.size(), "%s", placeClass->name.str().c_str());
                    pendingActorsTab = true;
                }
            }
            ImGui::BeginChild("classes");
            if (actorClass) {
                classNode(actorClass);
            }
            ImGui::EndChild();
        }

        void loadTexturePackage(const std::string& packageName)
        {
            dx::Linker* linker = dx::objects::loadPackage(dx::Name(packageName));
            textureList.clear();
            if (!linker) {
                return;
            }
            texturePackage = packageName;
            size_t count = linker->file().exports().size();
            for (size_t i = 0; i < count; i++) {
                dx::UObject* object = linker->object(static_cast<int32_t>(i + 1));
                if (object && object->cls && object->isA(dx::Name("Texture"))) {
                    textureList.push_back(object);
                }
            }
            std::sort(textureList.begin(), textureList.end(), [](dx::UObject* a, dx::UObject* b) {
                return a->name.str() < b->name.str();
            });
        }

        void textureBrowser()
        {
            if (ImGui::BeginCombo("Package", texturePackage.empty() ? "(choose)" : texturePackage.c_str())) {
                for (const std::string& name : texturePackages) {
                    if (ImGui::Selectable(name.c_str(), name == texturePackage)) {
                        loadTexturePackage(name);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Checkbox("Surface mode", &surfaceMode);
            ImGui::Text("%zu surfaces selected", selectedSurfs.size());
            if (currentTexture) {
                ImGui::Text("Texture: %s", currentTexture->name.str().c_str());
                if (ImGui::Button("Apply to selected surfaces")) {
                    command("applytexture " + currentTexture->pathName());
                }
            }
            ImGui::SliderInt("Thumbnail", &thumbnailSize, 32, 128);
            ImGui::BeginChild("textures");
            float width = ImGui::GetContentRegionAvail().x;
            int columns = std::max(1, static_cast<int>(width / (static_cast<float>(thumbnailSize) + 8.0f)));
            int column = 0;
            for (dx::UObject* texture : textureList) {
                dx::UTexture* image = dx::cast<dx::UTexture>(texture);
                const dx::GLTextureInfo* info = image ? thumbnails.get(image) : nullptr;
                ImGui::PushID(texture);
                ImGui::BeginGroup();
                bool chosen = texture == currentTexture;
                if (chosen) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));
                }
                float size = static_cast<float>(thumbnailSize);
                bool clicked = info ? ImGui::ImageButton("thumb", static_cast<ImTextureID>(info->id), ImVec2(size, size)) : ImGui::Button("?", ImVec2(size, size));
                if (chosen) {
                    ImGui::PopStyleColor();
                }
                if (clicked) {
                    currentTexture = texture;
                    dx::bsp::setCurrentTexture(texture);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s %ux%u", texture->name.str().c_str(), info ? info->width : 0u, info ? info->height : 0u);
                }
                ImGui::EndGroup();
                ImGui::PopID();
                column++;
                if (column < columns) {
                    ImGui::SameLine();
                } else {
                    column = 0;
                }
            }
            ImGui::EndChild();
        }

        bool editableType(const dx::UProperty* property)
        {
            switch (property->type) {
            case dx::PropType::Byte:
            case dx::PropType::Int:
            case dx::PropType::Bool:
            case dx::PropType::Float:
            case dx::PropType::Name:
            case dx::PropType::Str:
            case dx::PropType::String:
            case dx::PropType::Struct:
            case dx::PropType::Object:
            case dx::PropType::Class:
                return true;
            case dx::PropType::Array:
            case dx::PropType::Map:
            case dx::PropType::FixedArray:
            case dx::PropType::Pointer:
                return false;
            }
            return false;
        }

        void transformPanel()
        {
            dx::Vector& location = dx::actor::Location(selected);
            dx::Rotator& rotation = dx::actor::Rotation(selected);
            float position[3] = { location.x, location.y, location.z };
            int angles[3] = { rotation.pitch, rotation.yaw, rotation.roll };
            if (ImGui::DragFloat3("Location", position, 4.0f)) {
                location = { position[0], position[1], position[2] };
                brushCache.erase(selected);
            }
            trackEdit("Move " + selected->name.str());
            if (ImGui::DragInt3("Rotation", angles, 64.0f)) {
                rotation = { angles[0], angles[1], angles[2] };
                brushCache.erase(selected);
            }
            trackEdit("Rotate " + selected->name.str());
        }

        void trackEdit(const std::string& label)
        {
            if (ImGui::IsItemActivated() && !history.recording()) {
                history.begin(level, { selected }, label);
            }
            if (ImGui::IsItemDeactivated() && history.recording()) {
                history.commit();
            }
        }

        void propertiesPanel()
        {
            if (!selected) {
                ImGui::TextUnformatted("No actor selected");
                return;
            }
            ImGui::Text("%s", selected->name.str().c_str());
            ImGui::Text("Class %s", selected->cls->pathName().c_str());
            transformPanel();
            ImGui::Checkbox("Editable only", &editableOnly);
            std::map<std::string, std::vector<dx::UProperty*>> categories;
            for (dx::UProperty* property : selected->cls->properties) {
                if (editableOnly && !(property->propertyFlags & dx::CPF_Edit)) {
                    continue;
                }
                std::string category = property->category.none() ? std::string("None") : property->category.str();
                categories[category].push_back(property);
            }
            ImGui::BeginChild("properties");
            for (auto& category : categories) {
                if (!ImGui::CollapsingHeader(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    continue;
                }
                if (!ImGui::BeginTable(category.first.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
                    continue;
                }
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                for (dx::UProperty* property : category.second) {
                    for (int32_t index = 0; index < property->arrayDim; index++) {
                        propertyRow(property, index);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();
        }

        bool setProperty(dx::UObject* actor, dx::UProperty* property, int32_t index, const std::string& value)
        {
            uint8_t* memory = actor->data + property->offset + static_cast<size_t>(index) * property->elementSize();
            history.begin(level, { actor }, "Set " + property->name.str());
            if (!property->fromText(memory, value)) {
                history.cancel();
                status = "cannot set " + property->name.str() + " to " + value;
                return false;
            }
            history.commit();
            brushCache.erase(actor);
            status = "set " + property->name.str();
            return true;
        }

        void propertyRow(dx::UProperty* property, int32_t index)
        {
            objectPropertyRow(selected, property, index, true);
        }

        bool plainElement(const dx::UProperty* inner)
        {
            return inner && (inner->type == dx::PropType::Byte || inner->type == dx::PropType::Int || inner->type == dx::PropType::Float || inner->type == dx::PropType::Name || inner->type == dx::PropType::Object || inner->type == dx::PropType::Class || inner->type == dx::PropType::Bool);
        }

        void valueEditor(dx::UObject* object, dx::UProperty* property, uint8_t* memory, const void* key, bool tracked, const std::function<bool(const std::string&)>& apply)
        {
            ImGui::PushID(key);
            if (editingKey == key && editingObject == object) {
                ImGui::SetNextItemWidth(-1);
                if (focusEdit) {
                    ImGui::SetKeyboardFocusHere();
                    focusEdit = false;
                }
                if (ImGui::InputText("##edit", editBuffer.data(), editBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    apply(editBuffer.data());
                    editingKey = nullptr;
                } else if (ImGui::IsItemDeactivated()) {
                    editingKey = nullptr;
                }
            } else {
                std::string value = property->toText(memory);
                bool editable = editableType(property);
                if (!editable) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                }
                if (ImGui::Selectable(value.empty() ? " " : value.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) && editable) {
                    editingKey = key;
                    editingObject = object;
                    focusEdit = true;
                    std::snprintf(editBuffer.data(), editBuffer.size(), "%s", value.c_str());
                }
                if (!editable) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::PopID();
            static_cast<void>(tracked);
        }

        void objectPropertyRow(dx::UObject* object, dx::UProperty* property, int32_t index, bool tracked)
        {
            uint8_t* memory = object->data + property->offset + static_cast<size_t>(index) * property->elementSize();
            std::string label = property->name.str();
            if (property->arrayDim > 1) {
                label += "[" + std::to_string(index) + "]";
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label.c_str());
            ImGui::TableSetColumnIndex(1);
            if (property->type == dx::PropType::Array && plainElement(property->inner)) {
                auto& elements = *reinterpret_cast<std::vector<uint8_t>*>(memory);
                size_t size = property->inner->elementSize();
                size_t count = size ? elements.size() / size : 0;
                ImGui::PushID(memory);
                bool open = ImGui::TreeNode("elements", "%zu elements", count);
                ImGui::SameLine();
                if (ImGui::SmallButton("+")) {
                    if (tracked) {
                        history.begin(level, { object }, "Add element " + property->name.str());
                    }
                    elements.resize(elements.size() + size, 0);
                    if (tracked) {
                        history.commit();
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("-") && count > 0) {
                    if (tracked) {
                        history.begin(level, { object }, "Remove element " + property->name.str());
                    }
                    elements.resize(elements.size() - size);
                    if (tracked) {
                        history.commit();
                    }
                }
                if (open) {
                    for (size_t i = 0; i < count; i++) {
                        uint8_t* element = elements.data() + i * size;
                        ImGui::Text("[%zu]", i);
                        ImGui::SameLine();
                        valueEditor(object, property->inner, element, element, tracked, [&, i](const std::string& text) {
                            if (tracked) {
                                history.begin(level, { object }, "Set " + property->name.str());
                            }
                            auto& live = *reinterpret_cast<std::vector<uint8_t>*>(memory);
                            bool done = property->inner->fromText(live.data() + i * size, text);
                            if (tracked) {
                                if (done) {
                                    history.commit();
                                } else {
                                    history.cancel();
                                }
                            }
                            return done;
                        });
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
                return;
            }
            if (property->type == dx::PropType::FixedArray && plainElement(property->inner)) {
                ImGui::PushID(memory);
                if (ImGui::TreeNode("elements", "%d elements", property->fixedCount)) {
                    size_t size = property->inner->elementSize();
                    for (int32_t i = 0; i < property->fixedCount; i++) {
                        uint8_t* element = memory + static_cast<size_t>(i) * size;
                        ImGui::Text("[%d]", i);
                        ImGui::SameLine();
                        valueEditor(object, property->inner, element, element, tracked, [&, element](const std::string& text) {
                            return property->inner->fromText(element, text);
                        });
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
                return;
            }
            valueEditor(object, property, memory, memory, tracked, [&](const std::string& text) {
                if (tracked) {
                    return setProperty(object, property, index, text);
                }
                return property->fromText(memory, text);
            });
        }

        void builderPanel()
        {
            std::vector<dx::UClass*> classes = dx::brushbuilder::builderClasses();
            if (extraBuilderList.empty()) {
                extraBuilderList = dx::brushbuilder::extraBuilders();
            }
            std::string current = "(none)";
            if (builderChoice < classes.size()) {
                current = classes[builderChoice]->name.str();
            } else if (builderChoice - classes.size() < extraBuilderList.size()) {
                current = extraBuilderList[builderChoice - classes.size()].name;
            }
            if (ImGui::BeginCombo("Builder", current.c_str())) {
                for (size_t i = 0; i < classes.size(); i++) {
                    if (ImGui::Selectable(classes[i]->name.str().c_str(), i == builderChoice)) {
                        builderChoice = i;
                    }
                }
                for (size_t i = 0; i < extraBuilderList.size(); i++) {
                    if (ImGui::Selectable(extraBuilderList[i].name.c_str(), classes.size() + i == builderChoice)) {
                        builderChoice = classes.size() + i;
                    }
                }
                ImGui::EndCombo();
            }
            if (builderChoice >= classes.size() && builderChoice - classes.size() < extraBuilderList.size()) {
                dx::brushbuilder::ExtraBuilder& extra = extraBuilderList[builderChoice - classes.size()];
                for (dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                    if (parameter.boolean) {
                        bool value = parameter.value != 0.0;
                        if (ImGui::Checkbox(parameter.name.c_str(), &value)) {
                            parameter.value = value ? 1.0 : 0.0;
                        }
                    } else if (parameter.integer) {
                        int value = static_cast<int>(parameter.value);
                        if (ImGui::InputInt(parameter.name.c_str(), &value)) {
                            parameter.value = value;
                        }
                    } else {
                        ImGui::InputDouble(parameter.name.c_str(), &parameter.value);
                    }
                }
                if (ImGui::Button("Build brush")) {
                    std::string line = "brushbuild " + extra.name;
                    for (const dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                        line += " " + parameter.name + "=" + std::to_string(parameter.value);
                    }
                    command(line);
                }
            }
            if (builderChoice < classes.size()) {
                dx::UObject* object = dx::brushbuilder::builderFor(classes[builderChoice]);
                if (object && ImGui::BeginTable("builderProperties", 2, ImGuiTableFlags_BordersInnerV)) {
                    for (dx::UProperty* property : classes[builderChoice]->properties) {
                        if (!(property->propertyFlags & dx::CPF_Edit)) {
                            continue;
                        }
                        objectPropertyRow(object, property, 0, false);
                    }
                    ImGui::EndTable();
                }
                if (ImGui::Button("Build brush")) {
                    command("brushbuild " + classes[builderChoice]->name.str());
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Add")) {
                command("csgadd");
            }
            ImGui::SameLine();
            if (ImGui::Button("Subtract")) {
                command("csgsubtract");
            }
            ImGui::SameLine();
            if (ImGui::Button("Intersect")) {
                command("intersect");
            }
            ImGui::SameLine();
            if (ImGui::Button("Deintersect")) {
                command("deintersect");
            }
            if (ImGui::BeginCombo("Special", specialPresets[specialChoice].name.c_str())) {
                for (size_t i = 0; i < specialPresets.size(); i++) {
                    if (ImGui::Selectable(specialPresets[i].name.c_str(), i == specialChoice)) {
                        specialChoice = i;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Button("Add special")) {
                command("addspecial " + specialPresets[specialChoice].name);
            }
            std::vector<dx::UClass*> movers = moverClasses();
            std::string moverName = moverChoice < movers.size() ? movers[moverChoice]->name.str() : std::string("Mover");
            if (ImGui::BeginCombo("Mover class", moverName.c_str())) {
                for (size_t i = 0; i < movers.size(); i++) {
                    if (ImGui::Selectable(movers[i]->name.str().c_str(), i == moverChoice)) {
                        moverChoice = i;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Button("Add mover")) {
                command("addmover " + (moverChoice < movers.size() ? movers[moverChoice]->pathName() : std::string()));
            }
            ImGui::Separator();
            if (ImGui::Button("Rebuild...")) {
                showRebuild = true;
            }
        }

        std::vector<dx::UClass*> moverClasses()
        {
            std::vector<dx::UClass*> result;
            dx::UClass* base = dx::objects::findClass("Engine.Mover");
            if (!base) {
                return result;
            }
            std::vector<dx::UClass*> pending { base };
            while (!pending.empty()) {
                dx::UClass* cls = pending.back();
                pending.pop_back();
                if (!(cls->classFlags & dx::CLASS_Abstract)) {
                    result.push_back(cls);
                }
                auto found = classChildren.find(cls);
                if (found != classChildren.end()) {
                    pending.insert(pending.end(), found->second.begin(), found->second.end());
                }
            }
            std::sort(result.begin(), result.end(), [](dx::UClass* a, dx::UClass* b) {
                return a->name.str() < b->name.str();
            });
            return result;
        }

        void surfacePanel()
        {
            dx::UModel* model = level ? level->model : nullptr;
            if (!model || selectedSurfs.empty()) {
                ImGui::TextUnformatted("No surfaces selected (shift+click in the 3D view)");
                return;
            }
            const dx::BspSurf& surf = model->surfs[static_cast<size_t>(*selectedSurfs.begin())];
            ImGui::Text("%zu surfaces, texture %s", selectedSurfs.size(), surf.texture ? surf.texture->name.str().c_str() : "None");
            ImGui::Text("Pan U %d V %d", surf.panU, surf.panV);
            if (ImGui::Button("Apply current texture") && currentTexture) {
                command("applytexture");
            }
            ImGui::InputInt2("Pan", panStep);
            if (ImGui::Button("Pan")) {
                command("surfpan " + std::to_string(panStep[0]) + " " + std::to_string(panStep[1]));
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset pan")) {
                command("surfpanabs 0 0");
            }
            ImGui::InputFloat2("Scale U V", scaleValues);
            if (ImGui::Button("Scale")) {
                command("surfscale " + std::to_string(1.0f / scaleValues[0]) + " 0 0 " + std::to_string(1.0f / scaleValues[1]));
            }
            ImGui::SameLine();
            if (ImGui::Button("Rotate 90")) {
                command("surfrotate 90");
            }
            ImGui::SameLine();
            if (ImGui::Button("Rotate 45")) {
                command("surfrotate 45");
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip U")) {
                command("surfscale -1 0 0 1 relative");
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip V")) {
                command("surfscale 1 0 0 -1 relative");
            }
            const char* aligns[] = { "default", "floor", "wall", "wallpan", "onetile" };
            for (const char* align : aligns) {
                if (ImGui::Button((std::string("Align ") + align).c_str())) {
                    command(std::string("surfalign ") + align);
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
            const char* details[] = { "high", "normal", "low", "lowest" };
            for (const char* detail : details) {
                if (ImGui::Button((std::string("Light map ") + detail).c_str())) {
                    command(std::string("surfdetail ") + detail);
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
            static const std::pair<uint32_t, const char*> flags[] = {
                { 0x00000001, "Invisible" },
                { 0x00000002, "Masked" },
                { 0x00000004, "Translucent" },
                { 0x00000008, "Not solid" },
                { 0x00000010, "Environment" },
                { 0x00000020, "Semisolid" },
                { 0x00000040, "Modulated" },
                { 0x00000080, "Fake backdrop" },
                { 0x00000100, "Two sided" },
                { 0x00000200, "U pan" },
                { 0x00000400, "V pan" },
                { 0x00000800, "No smooth" },
                { 0x00001000, "Big wavy" },
                { 0x00002000, "Small wavy" },
                { 0x00004000, "Flat" },
                { 0x00010000, "No merge" },
                { 0x00020000, "Cloud wavy" },
                { 0x00040000, "Dirty shadows" },
                { 0x00080000, "Bright corners" },
                { 0x00100000, "Special lit" },
                { 0x00200000, "Gouraud" },
                { 0x00400000, "Unlit" },
                { 0x04000000, "Zone portal" },
                { 0x08000000, "Mirrored" },
            };
            for (const auto& [bit, label] : flags) {
                bool value = (surf.polyFlags & bit) != 0;
                if (ImGui::Checkbox(label, &value)) {
                    char text[64];
                    std::snprintf(text, sizeof(text), "surfflags 0x%x 0x%x", value ? bit : 0u, value ? 0u : bit);
                    command(text);
                }
            }
        }

        void rebuildWindow()
        {
            if (!showRebuild) {
                return;
            }
            ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Appearing);
            if (!ImGui::Begin("Rebuild", &showRebuild)) {
                ImGui::End();
                return;
            }
            ImGui::TextUnformatted("Geometry");
            ImGui::Checkbox("Auto BSP", &rebuildOptions.bsp);
            if (ImGui::Button("Rebuild geometry")) {
                command("rebuild");
            }
            ImGui::Separator();
            ImGui::TextUnformatted("BSP");
            int optimization = static_cast<int>(rebuildOptions.optimization);
            ImGui::RadioButton("Lame", &optimization, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Good", &optimization, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Optimal", &optimization, 2);
            rebuildOptions.optimization = static_cast<dx::bsp::Optimization>(optimization);
            ImGui::SliderInt("Balance", &rebuildOptions.balance, 0, 100);
            ImGui::Checkbox("Build visibility zones", &rebuildOptions.zones);
            ImGui::Checkbox("Optimize geometry", &rebuildOptions.optimizeGeometry);
            if (ImGui::Button("Rebuild BSP")) {
                command("rebuildbsp");
            }
            ImGui::Separator();
            ImGui::TextUnformatted("Lighting");
            ImGui::Checkbox("Selected lights only", &lightSelectedOnly);
            if (ImGui::Button("Rebuild lighting")) {
                command(lightSelectedOnly ? "light selected" : "light");
            }
            ImGui::Separator();
            ImGui::TextUnformatted("Paths");
            if (ImGui::Button("Define paths")) {
                command("paths define");
            }
            ImGui::SameLine();
            ImGui::RadioButton("Low", &pathOptimization, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Medium", &pathOptimization, 1);
            ImGui::SameLine();
            ImGui::RadioButton("High", &pathOptimization, 2);
            if (ImGui::Button("Build paths")) {
                command("paths build " + std::to_string(pathOptimization));
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove paths")) {
                command("paths remove");
            }
            ImGui::SameLine();
            if (ImGui::Button("Show")) {
                command("paths show");
            }
            ImGui::SameLine();
            if (ImGui::Button("Hide")) {
                command("paths hide");
            }
            ImGui::Separator();
            if (ImGui::Button("Rebuild all")) {
                command("rebuild");
                command(lightSelectedOnly ? "light selected" : "light");
                command("paths define");
            }
            ImGui::End();
        }

        void gridWindow()
        {
            if (!showGrid) {
                return;
            }
            if (!ImGui::Begin("Grid", &showGrid, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::End();
                return;
            }
            dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
            ImGui::Checkbox("Snap to grid", &constraints.gridEnabled);
            float size = constraints.gridSize.x;
            if (ImGui::InputFloat("Grid size", &size, 1.0f, 16.0f, "%.0f")) {
                size = std::max(size, 1.0f);
                constraints.gridSize = { size, size, size };
            }
            ImGui::Checkbox("Snap rotation", &constraints.rotGridEnabled);
            int rotation = constraints.rotGridSize.yaw;
            if (ImGui::InputInt("Rotation grid", &rotation, 256, 1024)) {
                rotation = std::max(rotation, 1);
                constraints.rotGridSize = { rotation, rotation, rotation };
            }
            ImGui::End();
        }

        dx::Vector snapped(const dx::Vector& location)
        {
            const dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
            if (!constraints.gridEnabled) {
                return location;
            }
            auto snap = [](float value, float grid) {
                return grid == 0.0f ? value : std::floor((value + 0.5f * grid) / grid) * grid;
            };
            return { snap(location.x, constraints.gridSize.x), snap(location.y, constraints.gridSize.y), snap(location.z, constraints.gridSize.z) };
        }

        void menuBar()
        {
            if (!ImGui::BeginMainMenuBar()) {
                return;
            }
            if (ImGui::BeginMenu("File")) {
                if (ImGui::BeginMenu("Open Map")) {
                    for (const std::string& mapName : mapNames) {
                        if (ImGui::MenuItem(mapName.c_str(), nullptr, mapName == currentMap)) {
                            pendingCommand = "open " + mapName;
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Open User Map")) {
                    std::error_code error;
                    if (std::filesystem::is_directory(userMapsDirectory(), error)) {
                        for (const auto& entry : std::filesystem::directory_iterator(userMapsDirectory(), error)) {
                            if (entry.path().extension() == ".dx" && ImGui::MenuItem(entry.path().stem().string().c_str())) {
                                pendingCommand = "openfile " + entry.path().string();
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("New Map")) {
                    pendingCommand = "new Untitled";
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    command("save");
                }
                if (ImGui::BeginMenu("Save As")) {
                    ImGui::InputText("Name", saveName.data(), saveName.size());
                    if (ImGui::Button("Save") && saveName[0]) {
                        command(std::string("saveas ") + saveName.data());
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Play Map")) {
                    command("play");
                }
                if (ImGui::MenuItem("Quit")) {
                    running = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                std::string undoLabel = "Undo " + history.nextUndo();
                std::string redoLabel = "Redo " + history.nextRedo();
                if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, history.undoCount() > 0)) {
                    command("undo");
                }
                if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, history.redoCount() > 0)) {
                    command("redo");
                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected != nullptr)) {
                    command("duplicate");
                }
                if (ImGui::MenuItem("Delete", "Del", false, selected != nullptr)) {
                    command("delete");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Build")) {
                if (ImGui::MenuItem("Rebuild Geometry")) {
                    command("rebuild");
                }
                if (ImGui::MenuItem("Rebuild Lighting")) {
                    command("light");
                }
                if (ImGui::MenuItem("Rebuild BSP")) {
                    command("rebuildbsp");
                }
                if (ImGui::MenuItem("Define Paths")) {
                    command("paths define");
                }
                if (ImGui::MenuItem("Rebuild All")) {
                    command("rebuild");
                    command("light");
                    command("paths define");
                }
                if (ImGui::MenuItem("Rebuild Options...")) {
                    showRebuild = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Focus selection", nullptr, false, selected != nullptr)) {
                    focusSelected();
                }
                if (ImGui::MenuItem("Reset cameras")) {
                    placeCamera();
                }
                ImGui::MenuItem("Surface mode", nullptr, &surfaceMode);
                ImGui::MenuItem("Grid...", nullptr, &showGrid);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Brush")) {
                ImGui::InputText("File", brushFile.data(), brushFile.size());
                ImGui::Checkbox("Merge coplanars on import", &brushMerge);
                if (ImGui::Button("Import") && brushFile[0]) {
                    command(std::string("brushimport ") + brushFile.data() + (brushMerge ? " 1" : " 0"));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Export") && brushFile[0]) {
                    command(std::string("brushexport ") + brushFile.data());
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }
            std::string title = status + "   " + currentMap + "  " + std::to_string(actors.size()) + " actors";
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(title.c_str()).x - 16.0f);
            ImGui::TextUnformatted(title.c_str());
            ImGui::EndMainMenuBar();
        }

        void frame(int width, int height)
        {
            menuBar();
            showActorsTab = pendingActorsTab;
            pendingActorsTab = false;
            float top = ImGui::GetFrameHeight();
            float w = static_cast<float>(width);
            float h = static_cast<float>(height) - top;
            float leftWidth = std::clamp(w * 0.2f, 180.0f, 320.0f);
            float rightWidth = std::clamp(w * 0.24f, 220.0f, 400.0f);
            ImGuiWindowFlags fixed = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
            ImGui::SetNextWindowPos(ImVec2(0, top));
            ImGui::SetNextWindowSize(ImVec2(leftWidth, h));
            ImGui::Begin("Browser", nullptr, fixed);
            if (ImGui::BeginTabBar("browserTabs")) {
                if (ImGui::BeginTabItem("Actors", nullptr, showActorsTab || pendingTab == "actors" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    actorBrowser();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Classes", nullptr, pendingTab == "classes" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    classBrowser();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Textures", nullptr, pendingTab == "textures" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    textureBrowser();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Brush", nullptr, pendingTab == "brush" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    builderPanel();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Surface", nullptr, pendingTab == "surface" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    surfacePanel();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            showActorsTab = false;
            pendingTab.clear();
            ImGui::End();
            ImGui::SetNextWindowPos(ImVec2(w - rightWidth, top));
            ImGui::SetNextWindowSize(ImVec2(rightWidth, h));
            ImGui::Begin("Properties", nullptr, fixed);
            propertiesPanel();
            ImGui::End();
            float viewWidth = (w - leftWidth - rightWidth) * 0.5f;
            float viewHeight = h * 0.5f;
            float left = leftWidth;
            orthoWindow(orthoViews[0], ImVec2(left, top), ImVec2(viewWidth, viewHeight));
            orthoWindow(orthoViews[1], ImVec2(left + viewWidth, top), ImVec2(viewWidth, viewHeight));
            orthoWindow(orthoViews[2], ImVec2(left, top + viewHeight), ImVec2(viewWidth, viewHeight));
            perspectiveWindow(ImVec2(left + viewWidth, top + viewHeight), ImVec2(viewWidth, viewHeight));
            rebuildWindow();
            gridWindow();
            finishDrag();
            moveSelectionWithKeys();
            selectionChanged = false;
        }

        bool newMap(const std::string& name)
        {
            dx::UObject* package = dx::objects::packageObject(dx::Name(name));
            dx::UClass* infoClass = dx::objects::findClass("Engine.LevelInfo");
            dx::UClass* deusExInfoClass = dx::objects::findClass("DeusEx.DeusExLevelInfo");
            dx::UClass* brushClass = dx::objects::findClass("Engine.Brush");
            dx::UClass* startClass = dx::objects::findClass("Engine.PlayerStart");
            dx::objects::flushLoads();
            if (!infoClass || !deusExInfoClass || !brushClass || !startClass) {
                status = "missing engine classes";
                return false;
            }
            auto create = [&](dx::UClass* cls, const char* objectName, uint32_t flags) {
                dx::UObject* object = dx::objects::newObject(cls, package, objectName ? dx::Name(objectName) : dx::Name(), flags);
                object->loaded = true;
                return object;
            };
            uint32_t editorOnly = EditorActorFlags | dx::RF_NotForClient | dx::RF_NotForServer;
            dx::ULevel* created = dx::cast<dx::ULevel>(create(dx::csg::nativeClass("Level"), "MyLevel", EditorActorFlags));
            created->model = dx::cast<dx::UModel>(create(dx::csg::nativeClass("Model"), nullptr, EditorActorFlags));
            dx::UObject* info = create(infoClass, nullptr, EditorActorFlags);
            dx::UObject* builder = create(brushClass, nullptr, editorOnly);
            dx::UModel* builderModel = dx::cast<dx::UModel>(create(dx::csg::nativeClass("Model"), nullptr, editorOnly));
            builderModel->polys = dx::cast<dx::UPolys>(create(dx::csg::nativeClass("Polys"), nullptr, EditorActorFlags));
            dx::csg::buildCube(*builderModel->polys, 256.0f, 256.0f, 256.0f, nullptr);
            dx::csg::updateBounds(*builderModel);
            dx::actor::Brush(builder) = builderModel;
            dx::brush::CsgOper(builder) = dx::csg::CsgActive;
            dx::UObject* start = create(startClass, nullptr, EditorActorFlags);
            dx::actor::Location(start) = { 0, 0, 0 };
            dx::UObject* deusExInfo = create(deusExInfoClass, nullptr, EditorActorFlags);
            created->actors = { info, builder, deusExInfo, start };
            for (dx::UObject* actor : created->actors) {
                dx::actor::Level(actor) = info;
            }
            return useLevel(created, name);
        }

        dx::UObject* levelInfo()
        {
            return level && !level->actors.empty() ? level->actors[0] : nullptr;
        }

        dx::UClass* resolveClass(const std::string& name)
        {
            if (name.find('.') != std::string::npos) {
                return dx::objects::findClass(name);
            }
            const char* prefixes[] = { "DeusEx.", "Engine.", "" };
            for (const char* prefix : prefixes) {
                dx::UClass* cls = dx::objects::findClass(std::string(prefix) + name);
                if (cls) {
                    return cls;
                }
            }
            return nullptr;
        }

        dx::UObject* spawnActor(dx::UClass* cls, const dx::Vector& location, const dx::Rotator& rotation, dx::UObject* templateActor)
        {
            dx::objects::flushLoads();
            dx::UObject* actor = dx::objects::newObject(cls, level->outermost(), dx::Name(), templateActor ? (templateActor->flags & ~0x02000000u) : EditorActorFlags);
            actor->loaded = true;
            if (templateActor) {
                cls->copyValue(actor->data, templateActor->data);
            }
            dx::actor::Location(actor) = location;
            dx::actor::Rotation(actor) = rotation;
            dx::actor::Level(actor) = levelInfo();
            *reinterpret_cast<dx::UObject**>(dx::actor::Region(actor)) = nullptr;
            if (templateActor && isBrush(templateActor)) {
                dx::UModel* source = dx::cast<dx::UModel>(dx::actor::Brush(templateActor));
                dx::UModel* model = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), level->outermost(), dx::Name(), source ? source->flags : EditorActorFlags));
                dx::UPolys* polys = dx::cast<dx::UPolys>(dx::objects::newObject(dx::csg::nativeClass("Polys"), level->outermost(), dx::Name(), EditorActorFlags));
                model->loaded = true;
                polys->loaded = true;
                if (source && source->polys) {
                    polys->polys = source->polys->polys;
                }
                model->polys = polys;
                dx::csg::updateBounds(*model);
                dx::actor::Brush(actor) = model;
            }
            level->actors.push_back(actor);
            return actor;
        }

        void restoreRegions(bool restore)
        {
            for (auto& [actor, region] : savedRegions) {
                if (actor->destroyed || !actor->data) {
                    continue;
                }
                if (restore) {
                    std::memcpy(dx::actor::Region(actor), region.data(), sizeof(dx::PointRegion));
                } else {
                    *reinterpret_cast<dx::UObject**>(dx::actor::Region(actor)) = nullptr;
                }
            }
        }

        bool saveTo(const std::filesystem::path& file)
        {
            restoreRegions(true);
            std::string error;
            bool ok = dx::mapwriter::saveMap(level, file, &error);
            restoreRegions(false);
            status = ok ? "saved " + file.filename().string() : "save failed: " + error;
            if (ok) {
                currentFile = file;
            }
            std::fprintf(stderr, "%s\n", status.c_str());
            return ok;
        }

        void launchGame(const std::filesystem::path& file)
        {
            std::string executable = gamePath.string();
            std::string map = file.string();
#ifdef _WIN32
            std::string quotedExecutable = "\"" + executable + "\"";
            std::string quotedMap = "\"" + map + "\"";
            const char* args[] = { quotedExecutable.c_str(), quotedMap.c_str(), nullptr };
            bool started = _spawnv(_P_NOWAIT, executable.c_str(), args) != -1;
#else
            char* args[] = { executable.data(), map.data(), nullptr };
            pid_t pid = 0;
            bool started = posix_spawn(&pid, executable.c_str(), nullptr, nullptr, args, environ) == 0;
#endif
            if (started) {
                status = "playing " + file.filename().string();
            } else {
                status = "cannot start " + executable;
            }
        }

        void captureRegions()
        {
            for (dx::UObject* actor : level->actors) {
                if (actor && !actor->destroyed && actor->data && actor->isA(dx::Name("Actor"))) {
                    std::array<uint8_t, sizeof(dx::PointRegion)> region {};
                    std::memcpy(region.data(), dx::actor::Region(actor), sizeof(dx::PointRegion));
                    savedRegions.insert_or_assign(actor, region);
                    *reinterpret_cast<dx::UObject**>(dx::actor::Region(actor)) = nullptr;
                }
            }
        }

        void geometryChanged()
        {
            renderer->flushLevel();
            refreshActors();
            buildEdges();
            selectedSurfs.clear();
            brushCache.clear();
        }

        bool surfaceEdit(const std::string& label, const std::function<void(dx::UModel&)>& edit)
        {
            dx::UModel* model = level ? level->model : nullptr;
            if (!model || selectedSurfs.empty()) {
                status = "no surfaces selected";
                return false;
            }
            std::vector<dx::UObject*> touched { model };
            for (int32_t index : selectedSurfs) {
                dx::UObject* owner = model->surfs[static_cast<size_t>(index)].actor;
                dx::UModel* brush = owner && owner->data ? dx::cast<dx::UModel>(dx::actor::Brush(owner)) : nullptr;
                if (brush && brush->polys) {
                    touched.push_back(brush->polys);
                }
            }
            history.begin(level, touched, label);
            edit(*model);
            history.commit();
            renderer->flushLevel();
            status = label + " on " + std::to_string(selectedSurfs.size()) + " surfaces";
            return true;
        }

        bool runBrushBuilder(const std::string& line)
        {
            std::istringstream input(line);
            std::string name;
            input >> name;
            dx::UObject* builder = builderBrush();
            if (!builder) {
                status = "no builder brush";
                return false;
            }
            std::vector<std::pair<std::string, std::string>> values;
            std::string item;
            while (input >> item) {
                size_t equals = item.find('=');
                if (equals != std::string::npos) {
                    values.emplace_back(item.substr(0, equals), item.substr(equals + 1));
                }
            }
            for (dx::UClass* cls : dx::brushbuilder::builderClasses()) {
                if (lower(cls->name.str()) != lower(name)) {
                    continue;
                }
                dx::UObject* object = dx::brushbuilder::builderFor(cls);
                for (const auto& [key, value] : values) {
                    dx::UProperty* property = cls->findProperty(dx::Name(key));
                    if (property) {
                        property->fromText(object->data + property->offset, value);
                    }
                }
                dx::brushbuilder::setTarget(builder);
                history.begin(level, { builder, dx::actor::Brush(builder), dx::cast<dx::UModel>(dx::actor::Brush(builder))->polys }, "Build " + cls->name.str());
                bool done = dx::brushbuilder::build(object);
                if (done) {
                    history.commit();
                } else {
                    history.cancel();
                }
                brushCache.erase(builder);
                status = done ? "built " + cls->name.str() : dx::brushbuilder::lastMessage();
                return done;
            }
            for (dx::brushbuilder::ExtraBuilder extra : dx::brushbuilder::extraBuilders()) {
                if (lower(extra.name) != lower(name)) {
                    continue;
                }
                for (const auto& [key, value] : values) {
                    for (dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                        if (lower(parameter.name) == lower(key)) {
                            parameter.value = std::strtod(value.c_str(), nullptr);
                        }
                    }
                }
                dx::UModel* brush = dx::cast<dx::UModel>(dx::actor::Brush(builder));
                history.begin(level, { builder, brush, brush->polys }, "Build " + extra.name);
                std::string error;
                bool done = dx::brushbuilder::buildExtra(extra, builder, &error);
                if (done) {
                    history.commit();
                } else {
                    history.cancel();
                }
                brushCache.erase(builder);
                status = done ? "built " + extra.name : error;
                return done;
            }
            status = "no brush builder " + name;
            return false;
        }

        bool pathsCommand(const std::string& line)
        {
            std::istringstream input(line);
            std::string action;
            int optimization = 1;
            input >> action >> optimization;
            restoreRegions(true);
            int count = 0;
            if (action == "define") {
                dx::paths::undefinePaths(level);
                dx::paths::definePaths(level);
                count = static_cast<int>(level->reachSpecs.size());
            } else if (action == "undefine") {
                dx::paths::undefinePaths(level);
            } else if (action == "build") {
                dx::paths::removePaths(level);
                count = dx::paths::buildPaths(level, optimization);
            } else if (action == "remove") {
                count = dx::paths::removePaths(level);
            } else if (action == "show") {
                count = dx::paths::showPaths(level);
            } else if (action == "hide") {
                count = dx::paths::hidePaths(level);
            } else {
                captureRegions();
                status = "unknown paths command " + action;
                return false;
            }
            captureRegions();
            refreshActors();
            status = "paths " + action + ": " + std::to_string(count);
            std::fprintf(stderr, "%s\n", status.c_str());
            return true;
        }

        bool command(const std::string& line)
        {
            std::istringstream input(line);
            std::string verb;
            input >> verb;
            std::string rest;
            std::getline(input, rest);
            size_t first = rest.find_first_not_of(' ');
            rest = first == std::string::npos ? std::string() : rest.substr(first);
            if (verb == "open") {
                return openMap(rest);
            }
            if (verb == "openfile") {
                return openMapFile(rest);
            }
            if (verb == "builder") {
                select(builderBrush());
                return selected != nullptr;
            }
            if (verb == "new") {
                return newMap(rest.empty() ? std::string("Untitled") : rest);
            }
            if (verb == "save") {
                std::filesystem::path file = currentFile.empty() ? userMapsDirectory() / (currentMap + ".dx") : currentFile;
                return saveTo(file);
            }
            if (verb == "saveas") {
                std::filesystem::path file = rest.find('/') != std::string::npos ? std::filesystem::path(rest) : userMapsDirectory() / (rest + ".dx");
                if (saveTo(file)) {
                    currentMap = file.stem().string();
                    return true;
                }
                return false;
            }
            if (verb == "play") {
                if (!command("save")) {
                    return false;
                }
                launchGame(currentFile);
                return true;
            }
            if (verb == "undo" || verb == "redo") {
                bool done = verb == "undo" ? history.undo() : history.redo();
                geometryChanged();
                status = done ? verb + " done" : "nothing to " + verb;
                return done;
            }
            if (verb == "select") {
                for (dx::UObject* actor : actors) {
                    if (lower(actor->name.str()) == lower(rest)) {
                        select(actor);
                        focusSelected();
                        return true;
                    }
                }
                status = "no actor " + rest;
                return false;
            }
            if (verb == "move" || verb == "moveto" || verb == "rotate") {
                if (!selected) {
                    return false;
                }
                std::istringstream values(rest);
                float a = 0;
                float b = 0;
                float c = 0;
                values >> a >> b >> c;
                history.begin(level, { selected }, (verb == "rotate" ? "Rotate " : "Move ") + selected->name.str());
                if (verb == "move") {
                    dx::actor::Location(selected) += dx::Vector { a, b, c };
                } else if (verb == "moveto") {
                    dx::actor::Location(selected) = { a, b, c };
                } else {
                    dx::Rotator& rotation = dx::actor::Rotation(selected);
                    rotation.pitch += static_cast<int32_t>(a);
                    rotation.yaw += static_cast<int32_t>(b);
                    rotation.roll += static_cast<int32_t>(c);
                }
                history.commit();
                brushCache.erase(selected);
                return true;
            }
            if (verb == "set") {
                if (!selected) {
                    return false;
                }
                std::istringstream values(rest);
                std::string propertyName;
                values >> propertyName;
                std::string value;
                std::getline(values, value);
                size_t start = value.find_first_not_of(' ');
                value = start == std::string::npos ? std::string() : value.substr(start);
                dx::UProperty* property = selected->cls->findProperty(dx::Name(propertyName));
                return property && setProperty(selected, property, 0, value);
            }
            if (verb == "add") {
                dx::UClass* cls = resolveClass(rest);
                if (!cls || (cls->classFlags & dx::CLASS_Abstract) || !actorClass || !cls->isChildOf(actorClass)) {
                    status = "cannot add " + rest;
                    return false;
                }
                history.begin(level, {}, "Add " + cls->name.str());
                dx::Vector location = cameraLocation + dx::rotationVector(cameraRotation) * 256.0f;
                dx::UObject* actor = spawnActor(cls, location, { 0, cameraRotation.yaw, 0 }, nullptr);
                history.commit();
                refreshActors();
                select(actor);
                status = "added " + actor->name.str();
                return true;
            }
            if (verb == "duplicate") {
                if (!selected || selected == levelInfo()) {
                    return false;
                }
                history.begin(level, {}, "Duplicate " + selected->name.str());
                dx::UObject* copy = spawnActor(selected->cls, dx::actor::Location(selected) + dx::Vector { 32, 32, 0 }, dx::actor::Rotation(selected), selected);
                history.commit();
                refreshActors();
                select(copy);
                status = "duplicated as " + copy->name.str();
                return true;
            }
            if (verb == "delete") {
                if (!selected || selected == levelInfo() || selected == builderBrush()) {
                    status = "cannot delete this actor";
                    return false;
                }
                dx::UObject* victim = selected;
                history.begin(level, { victim }, "Delete " + victim->name.str());
                level->actors.erase(std::remove(level->actors.begin(), level->actors.end(), victim), level->actors.end());
                victim->destroyed = true;
                history.commit();
                refreshActors();
                status = "deleted " + victim->name.str();
                return true;
            }
            if (verb == "cube" || verb == "cylinder" || verb == "sheet") {
                dx::UObject* builder = builderBrush();
                dx::UModel* model = builder ? dx::cast<dx::UModel>(dx::actor::Brush(builder)) : nullptr;
                if (!model) {
                    status = "no builder brush";
                    return false;
                }
                dx::objects::ensureLoaded(model);
                dx::objects::flushLoads();
                if (!model->polys) {
                    dx::UPolys* polys = dx::cast<dx::UPolys>(dx::objects::newObject(dx::csg::nativeClass("Polys"), level->outermost(), dx::Name(), EditorActorFlags));
                    polys->loaded = true;
                    model->polys = polys;
                }
                dx::objects::ensureLoaded(model->polys);
                dx::objects::flushLoads();
                std::istringstream values(rest);
                float a = 256;
                float b = 256;
                float c = 256;
                values >> a >> b >> c;
                dx::UObject* texture = currentTexture;
                if (verb == "cube") {
                    dx::csg::buildCube(*model->polys, a, b, c, texture);
                } else if (verb == "cylinder") {
                    dx::csg::buildCylinder(*model->polys, a, b, static_cast<int>(c), texture);
                } else {
                    dx::csg::buildSheet(*model->polys, a, b, texture);
                }
                dx::csg::updateBounds(*model);
                brushCache.erase(builder);
                status = "builder brush is a " + verb;
                return true;
            }
            if (verb == "csgadd" || verb == "csgsubtract") {
                dx::UObject* builder = builderBrush();
                if (!builder) {
                    return false;
                }
                uint32_t flags = verb == "csgadd" && !rest.empty() ? static_cast<uint32_t>(std::strtoul(rest.c_str(), nullptr, 0)) : 0;
                dx::bsp::setCurrentTexture(currentTexture);
                history.begin(level, { level->model }, verb == "csgadd" ? "Add brush" : "Subtract brush");
                dx::UObject* brush = dx::csg::addBrush(level, builder, verb == "csgadd" ? dx::csg::CsgAdd : dx::csg::CsgSubtract, flags);
                if (!brush) {
                    history.cancel();
                    return false;
                }
                history.commit();
                geometryChanged();
                select(brush);
                status = "created " + brush->name.str();
                return true;
            }
            if (verb == "addspecial") {
                auto preset = std::find_if(specialPresets.begin(), specialPresets.end(), [&](const SpecialPreset& item) {
                    return lower(item.name) == lower(rest);
                });
                if (preset == specialPresets.end()) {
                    status = "no special brush preset " + rest;
                    return false;
                }
                return command("csgadd " + std::to_string(preset->flags));
            }
            if (verb == "intersect" || verb == "deintersect") {
                dx::UObject* builder = builderBrush();
                if (!builder) {
                    return false;
                }
                history.begin(level, { builder, dx::actor::Brush(builder) }, verb == "intersect" ? "Brush from intersection" : "Brush from deintersection");
                bool done = dx::csg::brushFromIntersection(level, builder, verb == "deintersect");
                if (done) {
                    history.commit();
                } else {
                    history.cancel();
                }
                brushCache.erase(builder);
                status = done ? verb + " done" : verb + " failed";
                return done;
            }
            if (verb == "addmover") {
                dx::UObject* builder = builderBrush();
                dx::UClass* moverClass = rest.empty() ? dx::objects::findClass("Engine.Mover") : resolveClass(rest);
                if (!builder) {
                    return false;
                }
                dx::bsp::setCurrentTexture(currentTexture);
                history.begin(level, {}, "Add mover");
                dx::UObject* mover = dx::csg::addMover(level, builder, moverClass);
                if (!mover) {
                    history.cancel();
                    status = "cannot add mover";
                    return false;
                }
                history.commit();
                refreshActors();
                select(mover);
                status = "created " + mover->name.str();
                return true;
            }
            if (verb == "brushbuild") {
                return runBrushBuilder(rest);
            }
            if (verb == "brushexport") {
                dx::UObject* builder = builderBrush();
                if (!builder || rest.empty()) {
                    status = "brushexport needs a file";
                    return false;
                }
                std::ofstream out(rest, std::ios::binary);
                out << dx::editorops::exportPolys(*dx::cast<dx::UModel>(dx::actor::Brush(builder))->polys);
                status = out ? "exported brush to " + rest : "cannot write " + rest;
                return static_cast<bool>(out);
            }
            if (verb == "brushimport") {
                dx::UObject* builder = builderBrush();
                std::istringstream arguments(rest);
                std::string file;
                int merge = 0;
                uint32_t flags = 0;
                arguments >> file >> merge >> flags;
                std::ifstream in(file, std::ios::binary);
                if (!builder || !in) {
                    status = "cannot read " + file;
                    return false;
                }
                std::stringstream text;
                text << in.rdbuf();
                dx::UModel* brush = dx::cast<dx::UModel>(dx::actor::Brush(builder));
                history.begin(level, { builder, brush, brush->polys }, "Brush import");
                brush->linked = 0;
                dx::editorops::importPolys(*brush->polys, text.str());
                for (size_t i = 0; i < brush->polys->polys.size(); i++) {
                    brush->polys->polys[i].polyFlags |= flags;
                    brush->polys->polys[i].link = static_cast<int32_t>(i);
                }
                if (merge) {
                    dx::bsp::mergeCoplanars(*brush, false, true);
                    dx::brushbuilder::validateBrush(*brush, true);
                }
                history.commit();
                brushCache.erase(builder);
                status = "imported " + std::to_string(brush->polys->polys.size()) + " brush polygons";
                return true;
            }
            if (verb == "rebuild" || verb == "rebuildgeometry" || verb == "rebuildbsp") {
                dx::csg::RebuildOptions settings = rebuildOptions;
                settings.geometry = verb != "rebuildbsp";
                settings.bsp = verb != "rebuildgeometry" && (verb == "rebuildbsp" || rebuildOptions.bsp);
                dx::bsp::setCurrentTexture(currentTexture);
                restoreRegions(true);
                dx::csg::Stats stats = dx::csg::rebuild(level, settings);
                captureRegions();
                geometryChanged();
                status = "geometry: " + std::to_string(stats.brushes) + " brushes, " + std::to_string(stats.nodes) + " nodes, " + std::to_string(stats.surfs) + " surfaces, " + std::to_string(stats.zones) + " zones";
                std::fprintf(stderr, "%s\n", status.c_str());
                return true;
            }
            if (verb == "light") {
                std::unordered_set<dx::UObject*> chosen;
                if (rest == "selected" && selected) {
                    chosen.insert(selected);
                }
                restoreRegions(true);
                dx::lightbuild::Stats stats = dx::lightbuild::rebuild(level, rest == "selected" ? &chosen : nullptr);
                captureRegions();
                renderer->flushLevel();
                status = "lighting: " + std::to_string(stats.lights) + " lights, " + std::to_string(stats.surfaces) + " lightmaps";
                std::fprintf(stderr, "%s\n", status.c_str());
                return true;
            }
            if (verb == "paths") {
                return pathsCommand(rest);
            }
            if (verb == "surfpan" || verb == "surfpanabs") {
                std::istringstream values(rest);
                int u = 0;
                int v = 0;
                values >> u >> v;
                return surfaceEdit("Pan surface", [&](dx::UModel& model) {
                    dx::editorops::texPan(model, selectedSurfs, u, v, verb == "surfpanabs");
                });
            }
            if (verb == "surfscale") {
                std::istringstream values(rest);
                float uu = 1;
                float uv = 0;
                float vu = 0;
                float vv = 1;
                std::string mode;
                values >> uu >> uv >> vu >> vv >> mode;
                return surfaceEdit("Scale surface", [&](dx::UModel& model) {
                    dx::editorops::texScale(model, selectedSurfs, uu, uv, vu, vv, mode != "relative");
                });
            }
            if (verb == "surfrotate") {
                double degrees = std::atof(rest.c_str());
                float c = static_cast<float>(std::cos(degrees * 3.14159265358979 / 180.0));
                float sn = static_cast<float>(std::sin(degrees * 3.14159265358979 / 180.0));
                return surfaceEdit("Rotate surface", [&](dx::UModel& model) {
                    dx::editorops::texScale(model, selectedSurfs, c, sn, -sn, c, false);
                });
            }
            if (verb == "surfalign") {
                dx::editorops::TexAlign align = dx::editorops::TexAlign::Default;
                if (rest == "floor") {
                    align = dx::editorops::TexAlign::Floor;
                } else if (rest == "wall") {
                    align = dx::editorops::TexAlign::WallDir;
                } else if (rest == "wallpan") {
                    align = dx::editorops::TexAlign::WallPan;
                } else if (rest == "onetile") {
                    align = dx::editorops::TexAlign::OneTile;
                }
                return surfaceEdit("Align surface", [&](dx::UModel& model) {
                    dx::editorops::texAlign(model, selectedSurfs, align);
                });
            }
            if (verb == "surfflags") {
                std::istringstream values(rest);
                std::string setText;
                std::string clearText;
                values >> setText >> clearText;
                uint32_t setBits = static_cast<uint32_t>(std::strtoul(setText.c_str(), nullptr, 0));
                uint32_t clearBits = static_cast<uint32_t>(std::strtoul(clearText.c_str(), nullptr, 0));
                return surfaceEdit("Surface flags", [&](dx::UModel& model) {
                    dx::editorops::setAndClearFlags(model, selectedSurfs, setBits, clearBits);
                });
            }
            if (verb == "surfdetail") {
                uint32_t setBits = 0;
                if (rest == "low") {
                    setBits = dx::bsp::PF_LowShadowDetail;
                } else if (rest == "high") {
                    setBits = dx::bsp::PF_HighShadowDetail;
                } else if (rest == "lowest") {
                    setBits = dx::bsp::PF_LowShadowDetail | dx::bsp::PF_HighShadowDetail;
                }
                return surfaceEdit("Light map detail", [&](dx::UModel& model) {
                    dx::editorops::setAndClearFlags(model, selectedSurfs, setBits, dx::bsp::PF_LowShadowDetail | dx::bsp::PF_HighShadowDetail);
                });
            }
            if (verb == "grid") {
                std::istringstream values(rest);
                float size = 16;
                int enabled = 1;
                values >> size >> enabled;
                dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
                constraints.gridSize = { size, size, size };
                constraints.gridEnabled = enabled != 0;
                return true;
            }
            if (verb == "rotgrid") {
                std::istringstream values(rest);
                int size = 1024;
                int enabled = 1;
                values >> size >> enabled;
                dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
                constraints.rotGridSize = { size, size, size };
                constraints.rotGridEnabled = enabled != 0;
                return true;
            }
            if (verb == "bspoptions") {
                std::istringstream values(rest);
                std::string optimization;
                int balance = 15;
                int zones = 1;
                int optGeom = 1;
                values >> optimization >> balance >> zones >> optGeom;
                rebuildOptions.optimization = optimization == "lame" ? dx::bsp::Optimization::Lame : optimization == "good" ? dx::bsp::Optimization::Good
                                                                                                                            : dx::bsp::Optimization::Optimal;
                rebuildOptions.balance = std::clamp(balance, 0, 100);
                rebuildOptions.zones = zones != 0;
                rebuildOptions.optimizeGeometry = optGeom != 0;
                return true;
            }
            if (verb == "surface") {
                std::istringstream values(rest);
                float x = 0;
                float y = 0;
                values >> x >> y;
                int32_t surf = pickSurface(rayDirection(ImVec2(x, y), perspectiveOrigin, perspectiveSize));
                if (surf < 0) {
                    return false;
                }
                selectedSurfs.insert(surf);
                return true;
            }
            if (verb == "texture") {
                dx::UObject* texture = dx::objects::loadObject(rest, nullptr);
                dx::objects::flushLoads();
                if (!texture || !texture->isA(dx::Name("Texture"))) {
                    status = "no texture " + rest;
                    return false;
                }
                currentTexture = texture;
                return true;
            }
            if (verb == "applytexture") {
                if (!rest.empty() && !command("texture " + rest)) {
                    return false;
                }
                dx::UModel* model = level->model;
                if (!currentTexture || !model) {
                    return false;
                }
                dx::editorops::setTexture(*model, selectedSurfs, currentTexture);
                renderer->flushLevel();
                status = "applied " + currentTexture->name.str() + " to " + std::to_string(selectedSurfs.size()) + " surfaces";
                return true;
            }
            if (verb == "camera") {
                std::istringstream values(rest);
                float x = 0;
                float y = 0;
                float z = 0;
                int pitch = 0;
                int yaw = 0;
                values >> x >> y >> z >> pitch >> yaw;
                cameraLocation = { x, y, z };
                cameraRotation = { pitch, yaw, 0 };
                for (OrthoView& view : orthoViews) {
                    view.centerH = component(cameraLocation, view.horizontal);
                    view.centerV = component(cameraLocation, view.vertical);
                }
                return true;
            }
            if (verb == "zoom") {
                float zoom = static_cast<float>(std::atof(rest.c_str()));
                for (OrthoView& view : orthoViews) {
                    view.zoom = zoom;
                }
                return true;
            }
            if (verb == "tab") {
                pendingTab = rest;
                return true;
            }
            status = "unknown command " + verb;
            return false;
        }

        void printState()
        {
            std::string selection = selected ? selected->name.str() : std::string("none");
            dx::Vector location = selected ? dx::actor::Location(selected) : dx::Vector {};
            dx::Rotator rotation = selected ? dx::actor::Rotation(selected) : dx::Rotator {};
            std::printf("state selected=%s location=%.1f,%.1f,%.1f rotation=%d,%d,%d actors=%zu undo=%zu redo=%zu surfaces=%zu status=%s\n", selection.c_str(), static_cast<double>(location.x), static_cast<double>(location.y), static_cast<double>(location.z), rotation.pitch, rotation.yaw, rotation.roll, actors.size(), history.undoCount(), history.redoCount(), selectedSurfs.size(), status.c_str());
            std::fflush(stdout);
        }

        void scheduleAction(int frame, const std::string& action)
        {
            std::istringstream input(action);
            std::string verb;
            input >> verb;
            std::string viewName;
            auto push = [&](int at, int type, float x, float y, int button, bool down, ImGuiKey key) {
                InputEvent event;
                event.frame = at;
                event.type = type;
                event.x = x;
                event.y = y;
                event.button = button;
                event.down = down;
                event.key = key;
                event.view = viewName;
                inputEvents.push_back(event);
            };
            if (verb == "clickview" || verb == "dragview") {
                input >> viewName;
                verb = verb == "clickview" ? "click" : "drag";
            }
            if (verb == "click" || verb == "rclick") {
                float x = 0;
                float y = 0;
                input >> x >> y;
                int button = verb == "click" ? 0 : 1;
                push(frame, 0, x, y, 0, false, ImGuiKey_None);
                push(frame + 1, 1, x, y, button, true, ImGuiKey_None);
                push(frame + 2, 1, x, y, button, false, ImGuiKey_None);
                return;
            }
            if (verb == "drag" || verb == "rdrag") {
                float x1 = 0;
                float y1 = 0;
                float x2 = 0;
                float y2 = 0;
                input >> x1 >> y1 >> x2 >> y2;
                int button = verb == "drag" ? 0 : 1;
                push(frame, 0, x1, y1, 0, false, ImGuiKey_None);
                push(frame + 1, 1, x1, y1, button, true, ImGuiKey_None);
                for (int step = 1; step <= 8; step++) {
                    float t = static_cast<float>(step) / 8.0f;
                    push(frame + 1 + step, 0, x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, 0, false, ImGuiKey_None);
                }
                push(frame + 10, 1, x2, y2, button, false, ImGuiKey_None);
                return;
            }
            if (verb == "wheel") {
                float x = 0;
                float y = 0;
                float delta = 0;
                input >> x >> y >> delta;
                push(frame, 0, x, y, 0, false, ImGuiKey_None);
                push(frame + 1, 3, 0, delta, 0, false, ImGuiKey_None);
                return;
            }
            if (verb == "key" || verb == "keydown" || verb == "keyup") {
                std::string name;
                input >> name;
                ImGuiKey key = keyNamed(name);
                if (key == ImGuiKey_None) {
                    std::fprintf(stderr, "unknown key %s\n", name.c_str());
                    return;
                }
                if (verb != "keyup") {
                    push(frame, 2, 0, 0, 0, true, key);
                }
                if (verb != "keydown") {
                    push(verb == "key" ? frame + 1 : frame, 2, 0, 0, 0, false, key);
                }
                return;
            }
            scriptActions.emplace_back(frame, action);
        }

        void applyInput(int frameNumber)
        {
            ImGuiIO& io = ImGui::GetIO();
            for (InputEvent event : inputEvents) {
                if (event.frame != frameNumber) {
                    continue;
                }
                if (!event.view.empty()) {
                    auto rect = viewRects.find(event.view);
                    if (rect == viewRects.end()) {
                        continue;
                    }
                    event.x = rect->second.first.x + rect->second.second.x * event.x;
                    event.y = rect->second.first.y + rect->second.second.y * event.y;
                }
                if (event.type == 0) {
                    io.AddMousePosEvent(event.x, event.y);
                    scriptedMouse = ImVec2(event.x, event.y);
                    hasScriptedMouse = true;
                } else if (event.type == 1) {
                    io.AddMousePosEvent(event.x, event.y);
                    io.AddMouseButtonEvent(event.button, event.down);
                } else if (event.type == 2) {
                    io.AddKeyEvent(event.key, event.down);
                    ImGuiKey modifier = modifierFor(event.key);
                    if (modifier != ImGuiKey_None) {
                        io.AddKeyEvent(modifier, event.down);
                    }
                } else if (event.type == 3) {
                    io.AddMouseWheelEvent(0, event.y);
                }
            }
            if (hasScriptedMouse) {
                io.AddMousePosEvent(scriptedMouse.x, scriptedMouse.y);
            }
        }

        void runScript(int frameNumber, int width, int height)
        {
            for (const auto& [frame, action] : scriptActions) {
                if (frame != frameNumber) {
                    continue;
                }
                std::istringstream input(action);
                std::string verb;
                input >> verb;
                std::string rest;
                std::getline(input, rest);
                size_t first = rest.find_first_not_of(' ');
                rest = first == std::string::npos ? std::string() : rest.substr(first);
                if (verb == "exec") {
                    command(rest);
                } else if (verb == "shot") {
                    screenshot(width, height, rest);
                } else if (verb == "state") {
                    printState();
                }
            }
        }

        void run()
        {
            int frameCount = 0;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                    if (event.type == SDL_QUIT) {
                        running = false;
                    }
                }
                if (!pendingCommand.empty()) {
                    std::string pending = pendingCommand;
                    pendingCommand.clear();
                    command(pending);
                }
                int width = 0;
                int height = 0;
                SDL_GL_GetDrawableSize(window, &width, &height);
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                applyInput(frameCount);
                ImGui::NewFrame();
                frame(width, height);
                ImGui::Render();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, width, height);
                glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                runScript(frameCount, width, height);
                frameCount++;
                if (options.frames > 0 && frameCount >= options.frames) {
                    if (!options.screenshot.empty()) {
                        screenshot(width, height, options.screenshot);
                    }
                    running = false;
                }
                SDL_GL_SwapWindow(window);
            }
        }

        void screenshot(int width, int height, const std::string& path)
        {
            std::vector<uint8_t> raw(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());
            std::vector<uint8_t> rgba(raw.size());
            size_t stride = static_cast<size_t>(width) * 4;
            for (size_t y = 0; y < static_cast<size_t>(height); y++) {
                std::copy_n(raw.data() + y * stride, stride, rgba.data() + (static_cast<size_t>(height) - 1 - y) * stride);
            }
            saveBmp(path, rgba, width, height);
        }

    private:
        SDL_Window* window;
        Options options;
        std::filesystem::path gamePath;
        std::unique_ptr<dx::SceneRenderer> renderer;
        dx::ULevel* level = nullptr;
        std::vector<dx::UObject*> actors;
        std::vector<Edge> edges;
        std::unordered_map<dx::UObject*, std::vector<Edge>> brushCache;
        std::unordered_map<dx::UObject*, std::array<uint8_t, sizeof(dx::PointRegion)>> savedRegions;
        std::vector<std::string> mapNames;
        std::vector<std::string> texturePackages;
        std::string texturePackage;
        std::vector<dx::UObject*> textureList;
        dx::UObject* currentTexture = nullptr;
        std::string currentMap;
        std::filesystem::path currentFile;
        std::string pendingCommand;
        std::string pendingTab;
        std::string status;
        std::map<dx::UClass*, std::vector<dx::UClass*>> classChildren;
        dx::UClass* actorClass = nullptr;
        dx::UClass* placeClass = nullptr;
        dx::UObject* selected = nullptr;
        std::set<int32_t> selectedSurfs;
        bool surfaceMode = false;
        bool selectionChanged = false;
        bool dragging = false;
        dx::Vector dragStart;
        dx::Vector dragRaw;
        const void* editingKey = nullptr;
        dx::UObject* editingObject = nullptr;
        bool focusEdit = false;
        std::array<char, 512> editBuffer {};
        std::array<char, 128> filter {};
        std::array<char, 128> saveName {};
        std::array<char, 256> brushFile {};
        bool brushMerge = true;
        bool editableOnly = false;
        bool showActorsTab = false;
        bool pendingActorsTab = false;
        dx::Vector cameraLocation;
        dx::Rotator cameraRotation;
        float fov = 90.0f;
        float moveSpeed = 800.0f;
        std::array<OrthoView, 3> orthoViews {};
        ImVec2 perspectiveOrigin;
        ImVec2 perspectiveSize;
        std::map<std::string, std::pair<ImVec2, ImVec2>> viewRects;
        uint32_t framebuffer = 0;
        uint32_t colorTexture = 0;
        uint32_t depthBuffer = 0;
        int fbWidth = 0;
        int fbHeight = 0;
        bool running = true;
        dx::EditorHistory history;
        dx::csg::RebuildOptions rebuildOptions;
        size_t builderChoice = 0;
        std::vector<dx::brushbuilder::ExtraBuilder> extraBuilderList;
        size_t specialChoice = 0;
        size_t moverChoice = 0;
        int panStep[2] = { 16, 0 };
        float scaleValues[2] = { 1.0f, 1.0f };
        bool showRebuild = false;
        bool showGrid = false;
        bool lightSelectedOnly = false;
        int thumbnailSize = 64;
        dx::TextureCache thumbnails;
        int pathOptimization = 1;
        std::vector<SpecialPreset> specialPresets {
            { "Regular Brush", 0 },
            { "Invisible collision hull", dx::bsp::PF_Invisible | dx::bsp::PF_Semisolid },
            { "Zone Portal", dx::bsp::PF_Portal | dx::bsp::PF_Invisible | dx::bsp::PF_NotSolid | dx::bsp::PF_TwoSided },
            { "Semisolid Pillar", dx::bsp::PF_Semisolid },
            { "Transparent Window", 0x00000004u | dx::bsp::PF_NotSolid },
            { "Masked Decoration", dx::bsp::PF_Masked | dx::bsp::PF_NotSolid },
            { "Masked Wall", dx::bsp::PF_Masked | dx::bsp::PF_Semisolid },
            { "Water", dx::bsp::PF_Portal | 0x00000004u | dx::bsp::PF_NotSolid | dx::bsp::PF_TwoSided },
        };
        std::vector<InputEvent> inputEvents;
        std::vector<std::pair<int, std::string>> scriptActions;
        ImVec2 scriptedMouse;
        bool hasScriptedMouse = false;
    };
}

int main(int argc, char** argv)
{
    Options options = parseOptions(argc, argv);
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
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    std::filesystem::path gamePath = "deusex";
    if (char* base = SDL_GetBasePath()) {
        gamePath = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(base))) / "deusex";
        SDL_free(base);
    }
    int result = 0;
    try {
        Editor editor(window, options, gamePath);
        editor.start();
        editor.run();
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
