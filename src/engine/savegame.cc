#include "engine/savegame.h"

#include "core/config.h"
#include "core/objects.h"
#include "core/package_writer.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/map_writer.h"
#include "engine/mesh.h"
#include "engine/props_deusex.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <functional>
#include <unordered_set>

namespace dx {
    namespace savegame {
        namespace {
            struct State {
                std::filesystem::path root;
                std::filesystem::path pending;
            };

            State& state()
            {
                static State instance;
                return instance;
            }

            std::string lower(std::string text)
            {
                for (char& c : text) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return text;
            }

            std::filesystem::path defaultRoot()
            {
                return config::userDirectory() / "Save";
            }

            bool classNamed(const UClass* cls, std::initializer_list<const char*> names)
            {
                for (const UClass* current = cls; current; current = current->superClass()) {
                    for (const char* candidate : names) {
                        if (current->name == Name(candidate)) {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool transientObject(UObject* object)
            {
                for (const UClass* current = object->cls; current; current = current->superClass()) {
                    if (current->classFlags & CLASS_Transient) {
                        return true;
                    }
                }
                return classNamed(object->cls, { "Window", "GC", "Player", "Canvas", "Console", "Client", "GameDirectory" });
            }

            bool nativeResource(UObject* object)
            {
                return cast<UField>(object) || cast<UTexture>(object) || cast<UPalette>(object) || cast<USound>(object) || cast<UMusic>(object) || cast<UPolys>(object) || cast<UPrimitive>(object) || cast<UFont>(object) || cast<UMesh>(object) || cast<ULevel>(object);
            }

            bool isActor(UObject* object)
            {
                return object->cls && engineClasses().actor && object->isA(engineClasses().actor);
            }

            void forEachReference(UStruct* structType, uint8_t* data, const std::function<void(UObject*)>& visit)
            {
                if (!structType || !data) {
                    return;
                }
                for (UProperty* property : structType->properties) {
                    if (property->propertyFlags & (CPF_Transient | CPF_Native)) {
                        continue;
                    }
                    for (int32_t index = 0; index < property->arrayDim; index++) {
                        uint8_t* memory = data + property->offset + static_cast<size_t>(index) * property->elementSize();
                        if (property->type == PropType::Object || property->type == PropType::Class) {
                            visit(*reinterpret_cast<UObject**>(memory));
                        } else if (property->type == PropType::Struct) {
                            forEachReference(property->structType, memory, visit);
                        }
                    }
                }
            }

            UObject* findLevelInfo(World& world, const char* className)
            {
                for (UObject* a : world.actors) {
                    if (a && !a->destroyed && a->isA(Name(className))) {
                        return a;
                    }
                }
                return nullptr;
            }

            std::string twoDigitPrefix(const std::string& map)
            {
                if (map.size() >= 3 && std::isdigit(static_cast<unsigned char>(map[0])) && std::isdigit(static_cast<unsigned char>(map[1])) && map[2] == '_') {
                    return map.substr(0, 2);
                }
                return {};
            }

            std::string slotName(int32_t index)
            {
                if (index == QuickSaveIndex) {
                    return "QuickSave";
                }
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "Save%04d", index);
                return buffer;
            }

            void copyLevels(const std::filesystem::path& from, const std::filesystem::path& to)
            {
                std::error_code error;
                std::filesystem::create_directories(to, error);
                if (!std::filesystem::is_directory(from)) {
                    return;
                }
                for (const auto& entry : std::filesystem::directory_iterator(from)) {
                    if (entry.path().extension() == ".dxs" && entry.path().filename() != "SaveInfo.dxs") {
                        std::filesystem::copy_file(entry.path(), to / entry.path().filename(), std::filesystem::copy_options::overwrite_existing, error);
                    }
                }
            }

            void clearDirectory(const std::filesystem::path& directory)
            {
                std::error_code error;
                if (!std::filesystem::is_directory(directory)) {
                    return;
                }
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    std::filesystem::remove_all(entry.path(), error);
                }
            }
        }

        void setSaveRoot(const std::filesystem::path& root)
        {
            state().root = root;
        }

        std::filesystem::path saveRoot()
        {
            if (state().root.empty()) {
                state().root = defaultRoot();
            }
            std::error_code error;
            std::filesystem::create_directories(state().root, error);
            return state().root;
        }

        std::filesystem::path currentDirectory()
        {
            return saveRoot() / "Current";
        }

        std::filesystem::path slotDirectory(int32_t index)
        {
            return saveRoot() / slotName(index);
        }

        std::vector<int32_t> slotIndices()
        {
            std::vector<int32_t> result;
            for (const auto& entry : std::filesystem::directory_iterator(saveRoot())) {
                std::string name = entry.path().filename().string();
                if (!entry.is_directory() || name.size() != 8 || name.compare(0, 4, "Save") != 0) {
                    continue;
                }
                bool digits = std::all_of(name.begin() + 4, name.end(), [](char c) {
                    return std::isdigit(static_cast<unsigned char>(c)) != 0;
                });
                if (digits) {
                    result.push_back(std::atoi(name.c_str() + 4));
                }
            }
            std::sort(result.begin(), result.end());
            return result;
        }

        int32_t newSlotIndex()
        {
            std::vector<int32_t> used = slotIndices();
            return used.empty() ? 1 : used.back() + 1;
        }

        uint64_t slotSize(int32_t index)
        {
            uint64_t total = 0;
            std::filesystem::path directory = slotDirectory(index);
            if (!std::filesystem::is_directory(directory)) {
                return 0;
            }
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    total += static_cast<uint64_t>(entry.file_size());
                }
            }
            return total;
        }

        std::string mapBaseName(const std::string& url)
        {
            std::string map = url.substr(0, url.find('?'));
            size_t hash = map.find('#');
            if (hash != std::string::npos) {
                map = map.substr(0, hash);
            }
            map = std::filesystem::path(map).filename().string();
            size_t dot = map.rfind('.');
            if (dot != std::string::npos) {
                map = map.substr(0, dot);
            }
            return map;
        }

        bool saveLevel(World& world, const std::filesystem::path& file, bool includePlayer, std::string* error)
        {
            ULevel* level = world.level;
            if (!level) {
                if (error) {
                    *error = "no level loaded";
                }
                return false;
            }
            UObject* player = world.player();
            std::unordered_set<UObject*> excluded;
            if (!includePlayer && player) {
                for (UObject* a : world.actors) {
                    if (!a || a->destroyed) {
                        continue;
                    }
                    UObject* owner = a;
                    int guard = 0;
                    while (owner && owner != player && guard++ < 32) {
                        owner = actor::Owner(owner);
                    }
                    if (owner == player) {
                        excluded.insert(a);
                    }
                }
            }

            PackageWriter writer(Name("SaveGame"));
            UObject* mapPackage = level->outermost();
            writer.addExport(level, level->outer);
            std::deque<UObject*> queue;
            for (UObject* a : world.actors) {
                if (a && !a->destroyed && !excluded.count(a) && !writer.isExport(a) && !transientObject(a)) {
                    writer.addExport(a, a->outer);
                    queue.push_back(a);
                }
            }

            auto exportable = [&](UObject* object) {
                if (!object || object->destroyed || writer.isExport(object) || !object->cls || !object->outer) {
                    return false;
                }
                if (isActor(object) || transientObject(object) || nativeResource(object)) {
                    return false;
                }
                return !object->linker || object->linker->savedGame || object->outermost() == mapPackage;
            };
            auto visit = [&](UObject* object) {
                if (exportable(object)) {
                    writer.addExport(object, object->outer);
                    queue.push_back(object);
                }
            };
            auto addModel = [&](UModel* model) {
                if (!model || writer.isExport(model) || model->outermost() != mapPackage) {
                    return;
                }
                objects::ensureLoaded(model);
                writer.addExport(model, model->outer);
                if (model->polys && !writer.isExport(model->polys) && model->polys->outermost() == mapPackage) {
                    objects::ensureLoaded(model->polys);
                    writer.addExport(model->polys, model->polys->outer);
                }
            };
            addModel(level->model);
            for (UObject* a : world.actors) {
                if (a && !a->destroyed && writer.isExport(a)) {
                    addModel(cast<UModel>(actor::Brush(a)));
                }
            }
            while (!queue.empty()) {
                UObject* object = queue.front();
                queue.pop_front();
                if (object->outer && exportable(object->outer)) {
                    visit(object->outer);
                }
                forEachReference(object->cls, object->data, visit);
                aiReferences(object, visit);
            }
            objects::flushLoads();

            writer.setFlagSelector([&](UObject* object) {
                constexpr uint32_t loadFlags = 0x00070000;
                if (isActor(object)) {
                    return loadFlags | RF_Transactional;
                }
                if (object->outermost() == mapPackage) {
                    return loadFlags | (object->flags & (RF_Transactional | RF_Public | RF_Standalone));
                }
                return loadFlags;
            });

            writer.setFilter([&](UObject* object) {
                if (!object->outer) {
                    return true;
                }
                if (cast<UClass>(object)) {
                    return true;
                }
                if (isActor(object) || transientObject(object)) {
                    return false;
                }
                return object->linker && !object->linker->savedGame;
            });

            std::string map = mapBaseName(world.url);
            writer.setNativeSerializer([&](PackageWriter& out, UObject* object, Writer& data) {
                if (aiWriteNative(out, object, data)) {
                    return;
                }
                if (object == level) {
                    int32_t count = static_cast<int32_t>(world.actors.size());
                    data.i32(count);
                    data.i32(count);
                    for (UObject* a : world.actors) {
                        out.writeObject(data, a && !a->destroyed && out.isExport(a) ? a : nullptr);
                    }
                    std::vector<std::string> options;
                    size_t mark = world.url.find('?');
                    while (mark != std::string::npos) {
                        size_t next = world.url.find('?', mark + 1);
                        std::string option = world.url.substr(mark + 1, next == std::string::npos ? std::string::npos : next - mark - 1);
                        if (!option.empty()) {
                            options.push_back(option);
                        }
                        mark = next;
                    }
                    if (options.empty() && player) {
                        options.push_back("Name=Player");
                        options.push_back("Class=" + std::string(player->cls->outermost()->name.c_str()) + "." + player->cls->name.c_str());
                    }
                    data.string("deusex");
                    data.string("");
                    data.string(map);
                    data.string("");
                    data.index(static_cast<int32_t>(options.size()));
                    for (const std::string& option : options) {
                        data.string(option);
                    }
                    data.i32(7790);
                    data.u32(1);
                    out.writeObject(data, level->model);
                    data.index(static_cast<int32_t>(level->reachSpecs.size()));
                    for (const ReachSpec& spec : level->reachSpecs) {
                        data.i32(spec.distance);
                        out.writeObject(data, spec.start);
                        out.writeObject(data, spec.end);
                        data.i32(spec.collisionRadius);
                        data.i32(spec.collisionHeight);
                        data.i32(spec.reachFlags);
                        data.u8(spec.pruned);
                    }
                    data.f32(world.timeSeconds);
                    out.writeObject(data, nullptr);
                    for (UObject* block : level->textBlocks) {
                        out.writeObject(data, block);
                    }
                    data.index(static_cast<int32_t>(level->travelInfo.size()));
                    for (const auto& [key, value] : level->travelInfo) {
                        data.string(key);
                        data.string(value);
                    }
                    return;
                }
                if (UModel* model = cast<UModel>(object)) {
                    mapwriter::writeModel(out, *model, data);
                    return;
                }
                if (UPolys* polys = cast<UPolys>(object)) {
                    mapwriter::writePolys(out, *polys, data);
                    return;
                }
                if (classNamed(object->cls, { "ExtString" })) {
                    uint8_t* text = object->propertyAddress(Name("Text"));
                    data.string(text ? *reinterpret_cast<std::string*>(text) : std::string());
                }
            });

            return writer.save(file, error);
        }

        bool saveGame(World& world, int32_t index, const std::string& description)
        {
            std::string map = mapBaseName(world.url);
            if (!world.level || map.empty()) {
                return false;
            }
            if (index == 0) {
                index = newSlotIndex();
            }
            UObject* player = world.player();
            bool deusExPlayer = player && engineClasses().deusExPlayer && player->isA(engineClasses().deusExPlayer);
            if (deusExPlayer) {
                deusexplayer::saveCount(player) += 1;
            }
            std::string error;
            std::filesystem::path current = currentDirectory();
            if (!saveLevel(world, current / (map + ".dxs"), true, &error)) {
                std::fprintf(stderr, "save failed: %s\n", error.c_str());
                return false;
            }
            std::filesystem::path slot = slotDirectory(index);
            std::error_code ignored;
            std::filesystem::remove_all(slot, ignored);
            copyLevels(current, slot);

            UClass* infoClass = objects::findClass("DeusEx.DeusExSaveInfo");
            if (!infoClass) {
                return false;
            }
            objects::flushLoads();
            UObject* info = objects::newObject(infoClass, objects::packageObject(Name("Transient")), Name("MyDeusExSaveInfo"), 0);
            vm::Call stamp(info, Name("UpdateTimeStamp"));
            if (stamp.valid()) {
                stamp.run();
            }
            UObject* levelInfo = findLevelInfo(world, "DeusExLevelInfo");
            deusexsaveinfo::DirectoryIndex(info) = index;
            deusexsaveinfo::Description(info) = description;
            deusexsaveinfo::MissionLocation(info) = levelInfo ? deusexlevelinfo::MissionLocation(levelInfo) : std::string();
            deusexsaveinfo::MapName(info) = map;
            deusexsaveinfo::saveCount(info) = deusExPlayer ? deusexplayer::saveCount(player) : 0;
            deusexsaveinfo::saveTime(info) = deusExPlayer ? static_cast<int32_t>(deusexplayer::saveTime(player)) : 0;
            deusexsaveinfo::bCheatsEnabled(info) = player ? playerpawn::bCheatsEnabled(player) : 0;
            PackageWriter infoWriter(Name("SaveInfo"));
            infoWriter.addExport(info, nullptr);
            infoWriter.setFilter([](UObject* object) {
                return !object->outer || cast<UClass>(object) || (object->linker && !object->linker->savedGame);
            });
            infoWriter.setFlagSelector([](UObject*) {
                return 0x00070000u | RF_Public;
            });
            bool saved = infoWriter.save(slot / "SaveInfo.dxs", &error);
            objects::destroyObject(info);
            if (!saved) {
                std::fprintf(stderr, "save info failed: %s\n", error.c_str());
                return false;
            }
            return true;
        }

        bool deleteGame(int32_t index)
        {
            std::error_code error;
            return std::filesystem::remove_all(slotDirectory(index), error) > 0;
        }

        void deleteCurrent()
        {
            clearDirectory(currentDirectory());
            state().pending.clear();
        }

        UObject* readSaveInfo(int32_t index)
        {
            std::filesystem::path file = slotDirectory(index) / "SaveInfo.dxs";
            if (!std::filesystem::is_regular_file(file)) {
                return nullptr;
            }
            try {
                Linker* linker = objects::loadPackageFile(Name("SaveInfo_" + slotName(index)), file, true);
                const Package& package = linker->file();
                for (int32_t ref = 1; ref <= static_cast<int32_t>(package.exports().size()); ref++) {
                    if (package.classOf(ref) == "DeusExSaveInfo") {
                        UObject* info = linker->object(ref);
                        objects::ensureLoaded(info);
                        objects::flushLoads();
                        deusexsaveinfo::DirectoryIndex(info) = index;
                        return info;
                    }
                }
            } catch (const std::exception& failure) {
                std::fprintf(stderr, "cannot read save info %s: %s\n", file.string().c_str(), failure.what());
            }
            return nullptr;
        }

        bool loadRequested(const std::string& url, int32_t& index)
        {
            std::string options = lower(url.substr(std::min(url.find('?'), url.size())));
            size_t at = options.find("?loadgame=");
            if (at == std::string::npos) {
                return false;
            }
            index = std::atoi(options.c_str() + at + 10);
            return true;
        }

        bool prepareLoad(int32_t index, std::string& map)
        {
            UObject* info = readSaveInfo(index);
            if (!info) {
                return false;
            }
            map = deusexsaveinfo::MapName(info);
            std::filesystem::path slot = slotDirectory(index);
            if (map.empty() || !std::filesystem::is_regular_file(slot / (map + ".dxs"))) {
                return false;
            }
            std::filesystem::path current = currentDirectory();
            clearDirectory(current);
            copyLevels(slot, current);
            return true;
        }

        void storeDepartingLevel(World& world, const std::string& targetUrl)
        {
            std::string map = mapBaseName(world.url);
            std::string target = mapBaseName(targetUrl);
            std::filesystem::path current = currentDirectory();
            std::error_code error;
            UObject* levelInfo = findLevelInfo(world, "DeusExLevelInfo");
            int32_t mission = levelInfo ? deusexlevelinfo::missionNumber(levelInfo) : -1;
            if (!map.empty() && lower(map) == lower(target)) {
                std::filesystem::remove(current / (map + ".dxs"), error);
            } else if (!map.empty() && mission >= 0 && mission < 98) {
                std::string message;
                if (!saveLevel(world, current / (map + ".dxs"), false, &message)) {
                    std::fprintf(stderr, "cannot store %s: %s\n", map.c_str(), message.c_str());
                }
            }
            std::string prefix = twoDigitPrefix(target);
            if (prefix.empty() || !std::filesystem::is_directory(current)) {
                return;
            }
            for (const auto& entry : std::filesystem::directory_iterator(current)) {
                if (entry.path().extension() == ".dxs" && twoDigitPrefix(entry.path().stem().string()) != prefix) {
                    std::filesystem::remove(entry.path(), error);
                }
            }
        }

        bool selectLevel(const std::string& url)
        {
            std::filesystem::path file = currentDirectory() / (mapBaseName(url) + ".dxs");
            if (!std::filesystem::is_regular_file(file)) {
                state().pending.clear();
                return false;
            }
            state().pending = file;
            return true;
        }

        Linker* takePendingLevel(const std::string& map)
        {
            if (state().pending.empty()) {
                return nullptr;
            }
            std::filesystem::path file = state().pending;
            state().pending.clear();
            std::string base = mapBaseName(map);
            if (lower(file.stem().string()) != lower(base)) {
                return nullptr;
            }
            try {
                return objects::loadPackageFile(Name("SaveGame_" + base), file, true);
            } catch (const std::exception& failure) {
                std::fprintf(stderr, "cannot open saved level %s: %s\n", file.string().c_str(), failure.what());
                return nullptr;
            }
        }

        void levelRestored(World& world)
        {
            if (world.level) {
                world.timeSeconds = world.level->timeSeconds;
            }
            static const Name bindEvents("ConBindEvents");
            std::vector<UObject*> snapshot = world.actors;
            for (UObject* a : snapshot) {
                if (!a || a->destroyed) {
                    continue;
                }
                UFunction* function = vm::findFunction(a, bindEvents);
                if (function) {
                    vm::Call call(a, function);
                    call.run();
                }
            }
        }
    }
}
