#include "editor/editor.h"

#include <ImGuizmo.h>

#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#endif

#ifndef _WIN32
extern char** environ;
#endif

namespace dx::editor {
    Editor::Editor(SDL_Window* target, const Options& settings, const std::filesystem::path& gameExecutable)
        : window(target)
        , options(settings)
        , gamePath(gameExecutable)
    {
        orthoViews[0] = { "Top", 0, 1, 1.0f };
        orthoViews[1] = { "Front", 0, 2, -1.0f };
        orthoViews[2] = { "Side", 1, 2, -1.0f };
        for (size_t i = 0; i < panes.size(); i++) {
            panes[i].kind = static_cast<dx::editorviews::PaneKind>(i);
        }
    }

    Editor::~Editor()
    {
        releasePanes();
        releaseFramebuffer();
        if (audioReady) {
            dx::audio::shutdown();
        }
    }

    void Editor::start()
    {
        ImGuizmo::GetStyle().Colors[ImGuizmo::TEXT] = ImVec4(0, 0, 0, 0);
        ImGuizmo::GetStyle().Colors[ImGuizmo::TEXT_SHADOW] = ImVec4(0, 0, 0, 0);
        log.start();
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
        if (!options.blankMap.empty()) {
            newMap(options.blankMap);
        } else if (mapFile.has_parent_path() && std::filesystem::is_regular_file(mapFile)) {
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

    bool Editor::openMapFile(const std::filesystem::path& file)
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

    bool Editor::openMap(const std::string& mapName)
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

    bool Editor::useLevel(dx::ULevel* newLevel, const std::string& mapName)
    {
        renderer->flushLevel();
        if (level && level != newLevel) {
            dx::objects::unloadPackage(level->outermost()->name);
        }
        level = newLevel;
        currentMap = mapName;
        currentFile.clear();
        selected = nullptr;
        selectedActors.clear();
        contextActor = nullptr;
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

    void Editor::refreshActors()
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
        std::erase_if(selectedActors, [&](dx::UObject* actor) {
            return actor->destroyed || std::find(actors.begin(), actors.end(), actor) == actors.end();
        });
    }

    void Editor::placeCamera()
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
        syncPanes();
    }

    void Editor::focusSelected()
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
        syncPanes();
    }

    void Editor::select(dx::UObject* actor)
    {
        if (selected != actor) {
            editingKey = nullptr;
            componentSelection.clear();
        }
        selected = actor;
        selectedActors.clear();
        if (actor) {
            selectedActors.insert(actor);
        }
        selectionChanged = true;
    }

    void Editor::buildClassTree()
    {
        classChildren.clear();
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

    void Editor::menuBar()
    {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }
        if (ImGui::BeginMenu(withIcon(icons::File, "File").c_str())) {
            if (ImGui::BeginMenu(withIcon(icons::Open, "Open Map").c_str())) {
                for (const std::string& mapName : mapNames) {
                    if (ImGui::MenuItem(mapName.c_str(), nullptr, mapName == currentMap)) {
                        pendingCommand = "open " + mapName;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(withIcon(icons::Open, "Open User Map").c_str())) {
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
            if (ImGui::MenuItem(withIcon(icons::New, "New Map").c_str())) {
                pendingCommand = "new Untitled";
            }
            if (ImGui::MenuItem(withIcon(icons::Save, "Save").c_str(), "Ctrl+S")) {
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
            contentPackagesMenu();
            t3dFileItems();
            if (ImGui::MenuItem(withIcon(icons::Play, "Play Map").c_str())) {
                command("play");
            }
            if (ImGui::MenuItem("Quit")) {
                running = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::Edit, "Edit").c_str())) {
            std::string undoLabel = withIcon(icons::Undo, ("Undo " + history.nextUndo()).c_str());
            std::string redoLabel = withIcon(icons::Redo, ("Redo " + history.nextRedo()).c_str());
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, history.undoCount() > 0)) {
                command("undo");
            }
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, history.redoCount() > 0)) {
                command("redo");
            }
            if (ImGui::MenuItem(withIcon(icons::Duplicate, "Duplicate").c_str(), "Ctrl+D", false, selected != nullptr)) {
                command("duplicate");
            }
            if (ImGui::MenuItem(withIcon(icons::Delete, "Delete").c_str(), "Del", false, selected != nullptr)) {
                command("delete");
            }
            selectionEditItems();
            ImGui::Separator();
            if (ImGui::BeginMenu("Select Surfaces")) {
                surfaceSelectMenu();
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem(withIcon(icons::Search, "Search for Actors...").c_str(), "Ctrl+F", &showFind);
            if (ImGui::MenuItem("Find Next", "F3")) {
                findNext();
            }
            ImGui::EndMenu();
        }
        actorMenu();
        if (ImGui::BeginMenu(withIcon(icons::Build, "Build").c_str())) {
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
            ImGui::Separator();
            if (ImGui::MenuItem("Validate Level")) {
                command("mapcheck");
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::View, "View").c_str())) {
            if (ImGui::MenuItem(withIcon(icons::Focus, "Focus selection").c_str(), nullptr, false, selected != nullptr)) {
                focusSelected();
            }
            if (ImGui::MenuItem(withIcon(icons::Camera, "Reset cameras").c_str())) {
                placeCamera();
            }
            ImGui::MenuItem("Surface mode", nullptr, &surfaceMode);
            ImGui::MenuItem(withIcon(icons::Grid, "Grid...").c_str(), nullptr, &showGrid);
            ImGui::MenuItem("Level Properties...", nullptr, &showLevelProperties);
            ImGui::Separator();
            ImGui::MenuItem("Show Links", nullptr, &showPaths);
            ImGui::MenuItem("Actor Icons", nullptr, &showActorIcons);
            selectionViewItems();
            if (ImGui::BeginMenu(withIcon(icons::Layout, "Layout").c_str())) {
                if (ImGui::MenuItem("2x2 Views", nullptr, maximizedPane < 0)) {
                    maximizedPane = -1;
                }
                const char* paneNames[4] = { "Maximize Top Left", "Maximize Top Right", "Maximize Bottom Left", "Maximize Bottom Right" };
                for (int i = 0; i < 4; i++) {
                    if (ImGui::MenuItem(paneNames[i], nullptr, maximizedPane == i)) {
                        maximizedPane = i;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::Script, "Script").c_str())) {
            ImGui::MenuItem("Script Editor", nullptr, &showScripts);
            if (ImGui::MenuItem("Compile Changed", "F7")) {
                pendingCommand = "compilescripts changed";
            }
            if (ImGui::MenuItem("Compile All")) {
                pendingCommand = "compilescripts all";
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Changed Scripts")) {
                pendingCommand = "exportscripts changed";
            }
            if (ImGui::MenuItem("Export All Scripts")) {
                pendingCommand = "exportscripts all";
            }
            if (ImGui::BeginMenu("Save Package")) {
                for (const std::string& packageName : scripts.scriptPackages()) {
                    if (ImGui::MenuItem(packageName.c_str())) {
                        pendingCommand = "savepackage " + packageName;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::Brush, "Brush").c_str())) {
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
            brushMenuItems();
            builderMenuItems();
            ImGui::EndMenu();
        }
        moverMenu();
        if (ImGui::BeginMenu(withIcon(icons::Tools, "Tools").c_str())) {
            ImGui::MenuItem(withIcon(icons::Log, "Log").c_str(), nullptr, &showLog);
            ImGui::MenuItem("Results", nullptr, &showResults);
            ImGui::MenuItem("Conversation Editor", nullptr, &showConEdit);
            builderToolsMenuItems();
            ImGui::EndMenu();
        }
        std::string title = status + "   " + currentMap + "  " + std::to_string(actors.size()) + " actors";
        ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 16.0f, ImGui::GetWindowWidth() - ImGui::CalcTextSize(title.c_str()).x - 16.0f));
        ImGui::TextUnformatted(title.c_str());
        ImGui::EndMainMenuBar();
    }

    void Editor::frame(int width, int height)
    {
        popupWasOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
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
            if (ImGui::BeginTabItem(withIcon(icons::Actors, "Actors").c_str(), nullptr, showActorsTab || pendingTab == "actors" ? ImGuiTabItemFlags_SetSelected : 0)) {
                actorBrowser();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Classes, "Classes").c_str(), nullptr, pendingTab == "classes" ? ImGuiTabItemFlags_SetSelected : 0)) {
                classBrowser();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Textures, "Textures").c_str(), nullptr, pendingTab == "textures" ? ImGuiTabItemFlags_SetSelected : 0)) {
                textureBrowser();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Brush, "Brush").c_str(), nullptr, pendingTab == "brush" ? ImGuiTabItemFlags_SetSelected : 0)) {
                builderPanel();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Surface, "Surface").c_str(), nullptr, pendingTab == "surface" ? ImGuiTabItemFlags_SetSelected : 0)) {
                surfacePanel();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Sounds, "Sounds").c_str(), nullptr, pendingTab == "sounds" ? ImGuiTabItemFlags_SetSelected : 0)) {
                soundBrowser();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Music, "Music").c_str(), nullptr, pendingTab == "music" ? ImGuiTabItemFlags_SetSelected : 0)) {
                musicBrowser();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(withIcon(icons::Meshes, "Meshes").c_str(), nullptr, pendingTab == "meshes" ? ImGuiTabItemFlags_SetSelected : 0)) {
                meshBrowser();
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
        viewLayout(ImVec2(leftWidth, top), ImVec2(w - leftWidth - rightWidth, h));
        rebuildWindow();
        gridWindow();
        brushScaleWindow();
        texturePropertiesWindow();
        thumbnails.beginFrame(ImGui::GetTime());
        meshViewerWindow();
        scriptWindow();
        toolsWindows();
        conversationWindow();
        builderWindows();
        if (ImGui::IsKeyPressed(ImGuiKey_F7, false)) {
            pendingCommand = "compilescripts changed";
        }
        finishDrag();
        moveSelectionWithKeys();
        editModeKeys();
        selectionChanged = false;
    }

    bool Editor::newMap(const std::string& name)
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

    dx::UObject* Editor::levelInfo()
    {
        return level && !level->actors.empty() ? level->actors[0] : nullptr;
    }

    dx::UClass* Editor::resolveClass(const std::string& name)
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

    dx::UObject* Editor::spawnActor(dx::UClass* cls, const dx::Vector& location, const dx::Rotator& rotation, dx::UObject* templateActor)
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

    void Editor::restoreRegions(bool restore)
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

    bool Editor::saveTo(const std::filesystem::path& file)
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

    void Editor::launchGame(const std::filesystem::path& file)
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
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        if (log.originalStderr() >= 0) {
            posix_spawn_file_actions_adddup2(&actions, log.originalStderr(), 2);
        }
        bool started = posix_spawn(&pid, executable.c_str(), &actions, nullptr, args, environ) == 0;
        posix_spawn_file_actions_destroy(&actions);
#endif
        if (started) {
            status = "playing " + file.filename().string();
        } else {
            status = "cannot start " + executable;
        }
    }

    void Editor::captureRegions()
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

    void Editor::geometryChanged()
    {
        renderer->flushLevel();
        refreshActors();
        buildEdges();
        selectedSurfs.clear();
        brushCache.clear();
    }

    void Editor::run()
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

    void Editor::screenshot(int width, int height, const std::string& path)
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
}
