#include "core/objects.h"

#include "core/config.h"
#include "core/vm.h"
#include "engine/reader.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

namespace dx {
    namespace {
        struct Registry {
            std::filesystem::path root;
            std::map<std::string, std::filesystem::path> paths;
            std::unordered_map<Name, std::unique_ptr<Linker>> linkers;
            std::vector<std::unique_ptr<Linker>> retired;
            std::unordered_map<Name, UObject*> packageObjects;
            std::unordered_map<Name, ObjectFactory> factories;
            std::unordered_map<Name, UClass*> intrinsics;
            std::vector<UObject*> table;
            std::deque<UObject*> pending;
            std::unordered_map<Name, uint32_t> nameCounters;
            std::unordered_set<Name> usedNames;
            std::vector<UObject*> deferredData;
            std::vector<std::string> loadMismatches;
            bool bootstrapping = false;
            bool editorMode = false;
            UProperty* nameProperty = nullptr;
            UProperty* classProperty = nullptr;
            UProperty* outerProperty = nullptr;
            UProperty* flagsProperty = nullptr;
            UClass* objectClassPtr = nullptr;
            UClass* classClassPtr = nullptr;
        };

        Registry& registry()
        {
            static Registry instance;
            return instance;
        }

        std::string lower(std::string_view text)
        {
            std::string result(text);
            for (char& c : result) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return result;
        }

        const std::unordered_map<std::string, std::string>& intrinsicSupers()
        {
            static const std::unordered_map<std::string, std::string> supers = {
                { "field", "Object" },
                { "struct", "Field" },
                { "function", "Struct" },
                { "state", "Struct" },
                { "class", "State" },
                { "const", "Field" },
                { "enum", "Field" },
                { "property", "Field" },
                { "byteproperty", "Property" },
                { "intproperty", "Property" },
                { "boolproperty", "Property" },
                { "floatproperty", "Property" },
                { "objectproperty", "Property" },
                { "classproperty", "ObjectProperty" },
                { "nameproperty", "Property" },
                { "strproperty", "Property" },
                { "stringproperty", "Property" },
                { "structproperty", "Property" },
                { "arrayproperty", "Property" },
                { "fixedarrayproperty", "Property" },
                { "mapproperty", "Property" },
                { "pointerproperty", "Property" },
                { "textbuffer", "Object" },
                { "package", "Object" },
                { "linker", "Object" },
                { "linkerload", "Linker" },
                { "linkersave", "Linker" },
                { "system", "Subsystem" },
                { "language", "Object" },
                { "viewport", "Player" },
                { "lodmesh", "Mesh" },
                { "client", "Object" },
                { "aievent", "Object" },
                { "aisenderevent", "AIEvent" },
                { "aireceiverevent", "AIEvent" },
            };
            return supers;
        }

        UObject* makeIntrinsicInstance(Name typeName)
        {
            std::string key = lower(typeName.str());
            if (key == "function") {
                return new UFunction();
            }
            if (key == "state") {
                return new UState();
            }
            if (key == "struct") {
                return new UStruct();
            }
            if (key == "class") {
                return new UClass();
            }
            if (key == "const") {
                return new UConst();
            }
            if (key == "enum") {
                return new UEnum();
            }
            if (key == "textbuffer") {
                return new UTextBuffer();
            }
            if (key == "byteproperty") {
                return new UProperty(PropType::Byte);
            }
            if (key == "intproperty") {
                return new UProperty(PropType::Int);
            }
            if (key == "boolproperty") {
                return new UProperty(PropType::Bool);
            }
            if (key == "floatproperty") {
                return new UProperty(PropType::Float);
            }
            if (key == "objectproperty") {
                return new UProperty(PropType::Object);
            }
            if (key == "classproperty") {
                return new UProperty(PropType::Class);
            }
            if (key == "nameproperty") {
                return new UProperty(PropType::Name);
            }
            if (key == "strproperty") {
                return new UProperty(PropType::Str);
            }
            if (key == "stringproperty") {
                return new UProperty(PropType::String);
            }
            if (key == "structproperty") {
                return new UProperty(PropType::Struct);
            }
            if (key == "arrayproperty") {
                return new UProperty(PropType::Array);
            }
            if (key == "fixedarrayproperty") {
                return new UProperty(PropType::FixedArray);
            }
            if (key == "mapproperty") {
                return new UProperty(PropType::Map);
            }
            if (key == "pointerproperty") {
                return new UProperty(PropType::Pointer);
            }
            return nullptr;
        }

        UObject* makeInstance(Name typeName)
        {
            if (UObject* object = makeIntrinsicInstance(typeName)) {
                return object;
            }
            auto it = registry().factories.find(typeName);
            if (it != registry().factories.end()) {
                return it->second();
            }
            return new UObject();
        }

        bool isKnownType(Name typeName)
        {
            if (intrinsicSupers().count(lower(typeName.str()))) {
                return true;
            }
            return registry().factories.count(typeName) != 0;
        }

        Name nativeTypeOf(Linker* linker, int32_t classRef)
        {
            for (int guard = 0; guard < 128 && classRef != 0; guard++) {
                const Package& file = linker->file();
                Name className(file.objectName(classRef));
                if (isKnownType(className)) {
                    return className;
                }
                if (classRef > 0) {
                    classRef = file.exportAt(classRef).superRef;
                    continue;
                }
                int32_t root = classRef;
                while (file.importAt(root).outer != 0) {
                    root = file.importAt(root).outer;
                }
                Linker* other = objects::loadPackage(Name(file.objectName(root)));
                if (!other) {
                    return Name("Object");
                }
                int32_t found = other->findExport(className, Name("Class"), file.groupOf(classRef));
                if (found == 0) {
                    return className;
                }
                linker = other;
                classRef = found;
            }
            return Name("Object");
        }
    }

    Linker::Linker(Name name, std::unique_ptr<Package> file)
        : packageName(name)
        , package(std::move(file))
    {
        static uint64_t created = 0;
        loadOrder = ++created;
        names.reserve(package->names().size());
        for (const NameEntry& entry : package->names()) {
            names.emplace_back(entry.name);
        }
        exports.resize(package->exports().size(), nullptr);
        imports.resize(package->imports().size(), nullptr);
        importResolved.resize(package->imports().size(), false);
    }

    const Package& Linker::file() const
    {
        return *package;
    }

    Name Linker::name(int32_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= names.size()) {
            throw std::runtime_error("name index out of range in " + packageName.str());
        }
        return names[static_cast<size_t>(index)];
    }

    UObject* Linker::readObject(Reader& reader)
    {
        return object(reader.index());
    }

    Name Linker::readName(Reader& reader)
    {
        return name(reader.index());
    }

    int32_t Linker::findExport(Name objectName, Name className, std::string_view group) const
    {
        const auto& table = package->exports();
        for (size_t i = 0; i < table.size(); i++) {
            if (names[static_cast<size_t>(table[i].name)] != objectName) {
                continue;
            }
            int32_t ref = static_cast<int32_t>(i + 1);
            if (!className.none()) {
                Name actualClass = table[i].classRef == 0 ? Name("Class") : Name(package->objectName(table[i].classRef));
                if (actualClass != className) {
                    continue;
                }
            }
            if (group != "*" && !iequals(package->groupOf(ref), group)) {
                continue;
            }
            return ref;
        }
        return 0;
    }

    UObject* Linker::object(int32_t ref)
    {
        if (ref == 0) {
            return nullptr;
        }
        if (ref > 0) {
            size_t index = static_cast<size_t>(ref - 1);
            if (index >= exports.size()) {
                throw std::runtime_error("export reference out of range in " + packageName.str());
            }
            if (!exports[index]) {
                uint32_t exportFlags = package->exportAt(ref).flags;
                if (!registry().editorMode && (exportFlags & (RF_NotForClient | RF_NotForServer))) {
                    return nullptr;
                }
                return createExport(ref);
            }
            return exports[index];
        }
        size_t index = static_cast<size_t>(-ref - 1);
        if (index >= imports.size()) {
            throw std::runtime_error("import reference out of range in " + packageName.str());
        }
        if (!importResolved[index]) {
            importResolved[index] = true;
            imports[index] = resolveImport(ref);
        }
        return imports[index];
    }

    UObject* Linker::createExport(int32_t ref)
    {
        const ExportEntry& entry = package->exportAt(ref);
        UObject* result;
        if (entry.classRef == 0) {
            result = new UClass();
        } else {
            result = makeInstance(nativeTypeOf(this, entry.classRef));
        }
        result->name = names[static_cast<size_t>(entry.name)];
        registry().usedNames.insert(result->name);
        result->flags = entry.flags;
        result->linker = this;
        result->exportRef = ref;
        result->loaded = false;
        exports[static_cast<size_t>(ref - 1)] = result;
        objects::registerObject(result);
        result->outer = entry.outer != 0 ? object(entry.outer) : packageObject;
        if (entry.classRef != 0) {
            result->cls = cast<UClass>(object(entry.classRef));
            if (!result->cls) {
                std::fprintf(stderr, "warning: missing class for %s\n", result->pathName().c_str());
                result->cls = objects::objectClass();
            }
        } else {
            result->cls = objects::classClass();
        }
        objects::queueLoad(result);
        return result;
    }

    UObject* Linker::resolveImport(int32_t ref)
    {
        const ImportEntry& entry = package->importAt(ref);
        Name objectName = names[static_cast<size_t>(entry.name)];
        Name className = names[static_cast<size_t>(entry.className)];
        if (entry.outer == 0) {
            objects::loadPackage(objectName);
            return objects::packageObject(objectName);
        }
        int32_t root = ref;
        while (package->importAt(root).outer != 0) {
            root = package->importAt(root).outer;
        }
        Name rootName(package->objectName(root));
        Linker* other = objects::loadPackage(rootName);
        if (!other) {
            std::fprintf(stderr, "warning: package %s not found for %s\n", rootName.c_str(), objectName.c_str());
            return nullptr;
        }
        std::string group = package->groupOf(ref);
        int32_t found = other->findExport(objectName, className, group);
        if (found == 0 && className == Name("Class")) {
            return objects::intrinsicClass(objectName);
        }
        if (found == 0) {
            found = other->findExport(objectName, className, "*");
        }
        if (found == 0) {
            std::fprintf(stderr, "warning: cannot resolve %s %s.%s.%s\n", className.c_str(), rootName.c_str(), group.c_str(), objectName.c_str());
            return nullptr;
        }
        return other->object(found);
    }

    namespace objects {
        int32_t legacyLatentAction(int32_t action)
        {
            switch (action) {
            case 256:
                return 384;
            case 261:
                return 385;
            case 301:
                return 302;
            case 500:
            case 502:
            case 504:
            case 506:
            case 508:
            case 510:
            case 527:
                return action + 1;
            default:
                return action;
            }
        }

        void allocateData(UObject* object);
        void finishBootstrap();

        void initialize(const std::filesystem::path& gameRoot)
        {
            Registry& reg = registry();
            reg.root = gameRoot;
            if (reg.table.empty()) {
                reg.table.push_back(nullptr);
            }
            const char* folders[] = { "System", "Textures", "Sounds", "Music", "Maps" };
            const char* extensions[] = { ".u", ".utx", ".uax", ".umx", ".dx", ".unr" };
            std::vector<std::filesystem::path> directories;
            for (const char* folder : folders) {
                directories.push_back(gameRoot / folder);
            }
            std::filesystem::path userSystem = config::userDirectory() / "System";
            directories.push_back(userSystem);
            for (const std::filesystem::path& directory : directories) {
                if (!std::filesystem::is_directory(directory)) {
                    continue;
                }
                for (const auto& item : std::filesystem::directory_iterator(directory)) {
                    std::string extension = lower(item.path().extension().string());
                    bool known = false;
                    for (const char* candidate : extensions) {
                        if (extension == candidate) {
                            known = true;
                        }
                    }
                    if (known && directory == userSystem) {
                        reg.paths[lower(item.path().stem().string())] = item.path();
                    } else if (known) {
                        reg.paths.emplace(lower(item.path().stem().string()), item.path());
                    }
                }
            }
            objectClass();
        }

        const std::filesystem::path& gameRoot()
        {
            return registry().root;
        }

        UObject* packageObject(Name packageName)
        {
            Registry& reg = registry();
            auto it = reg.packageObjects.find(packageName);
            if (it != reg.packageObjects.end()) {
                return it->second;
            }
            UObject* object = new UObject();
            object->name = packageName;
            object->flags = RF_Public;
            reg.packageObjects.emplace(packageName, object);
            registerObject(object);
            auto linker = reg.linkers.find(packageName);
            if (linker != reg.linkers.end()) {
                linker->second->packageObject = object;
            }
            object->cls = intrinsicClass(Name("Package"));
            if (reg.bootstrapping) {
                reg.deferredData.push_back(object);
            } else {
                allocateData(object);
            }
            return object;
        }

        void unloadPackage(Name packageName)
        {
            Registry& reg = registry();
            auto it = reg.linkers.find(packageName);
            if (it == reg.linkers.end()) {
                return;
            }
            reg.retired.push_back(std::move(it->second));
            reg.linkers.erase(it);
            reg.packageObjects.erase(packageName);
        }

        Linker* findLinker(Name packageName)
        {
            auto it = registry().linkers.find(packageName);
            if (it == registry().linkers.end()) {
                return nullptr;
            }
            return it->second.get();
        }

        Linker* loadPackage(Name packageName)
        {
            Registry& reg = registry();
            auto it = reg.linkers.find(packageName);
            if (it != reg.linkers.end()) {
                return it->second.get();
            }
            auto path = reg.paths.find(lower(packageName.str()));
            if (path == reg.paths.end()) {
                return nullptr;
            }
            auto linker = std::make_unique<Linker>(packageName, std::make_unique<Package>(path->second));
            Linker* result = linker.get();
            reg.linkers.emplace(packageName, std::move(linker));
            result->packageObject = packageObject(packageName);
            return result;
        }

        Linker* loadPackageFile(Name packageName, const std::filesystem::path& path, bool savedGame)
        {
            Registry& reg = registry();
            unloadPackage(packageName);
            auto linker = std::make_unique<Linker>(packageName, std::make_unique<Package>(path));
            Linker* result = linker.get();
            result->savedGame = savedGame;
            reg.linkers.emplace(packageName, std::move(linker));
            result->packageObject = packageObject(packageName);
            return result;
        }

        const std::vector<std::string>& loadMismatches()
        {
            return registry().loadMismatches;
        }

        void setEditorMode(bool enabled)
        {
            registry().editorMode = enabled;
        }

        bool editorMode()
        {
            return registry().editorMode;
        }

        void registerFactory(Name nativeClass, ObjectFactory factory)
        {
            registry().factories[nativeClass] = std::move(factory);
        }

        void registerObject(UObject* object)
        {
            Registry& reg = registry();
            if (reg.table.empty()) {
                reg.table.push_back(nullptr);
            }
            object->handle = static_cast<uint32_t>(reg.table.size());
            reg.table.push_back(object);
        }

        UObject* fromHandle(uint32_t handle)
        {
            Registry& reg = registry();
            if (handle == 0 || handle >= reg.table.size()) {
                return nullptr;
            }
            UObject* object = reg.table[handle];
            if (object && !object->loaded) {
                ensureLoaded(object);
            }
            return object;
        }

        const std::vector<UObject*>& all()
        {
            return registry().table;
        }

        void queueLoad(UObject* object)
        {
            registry().pending.push_back(object);
        }

        void flushLoads()
        {
            Registry& reg = registry();
            while (!reg.pending.empty()) {
                UObject* object = reg.pending.front();
                reg.pending.pop_front();
                ensureLoaded(object);
            }
        }

        void ensureLoaded(UObject* object)
        {
            if (!object || object->loaded) {
                return;
            }
            object->loaded = true;
            Linker& linker = *object->linker;
            const ExportEntry& entry = linker.file().exportAt(object->exportRef);
            UClass* asClass = cast<UClass>(object);
            UClass* cls = object->cls;
            if (cls && !registry().bootstrapping) {
                ensureLoaded(cls);
                cls->linkClass();
                object->data = new uint8_t[std::max<size_t>(cls->size, 1)];
                cls->initValue(object->data);
                if (cls->defaultObject) {
                    cls->copyValue(object->data, cls->defaultObject->data);
                }
            } else if (!asClass) {
                registry().deferredData.push_back(object);
            }
            if (entry.size > 0) {
                Reader reader(linker.file().data(), static_cast<size_t>(entry.offset));
                int32_t stackNode = 0;
                int32_t stackState = 0;
                int32_t stackOffset = -1;
                uint64_t probeMask = 0;
                int32_t latentAction = 0;
                if (entry.flags & RF_HasStackFlag) {
                    stackNode = reader.index();
                    stackState = reader.index();
                    probeMask = reader.u64();
                    latentAction = reader.i32();
                    if (stackNode != 0) {
                        stackOffset = reader.index();
                    }
                }
                try {
                    if (!asClass) {
                        readTaggedProperties(object->data ? cls : nullptr, object->data, reader, linker);
                    }
                    object->serialize(reader, linker);
                    size_t end = static_cast<size_t>(entry.offset) + static_cast<size_t>(entry.size);
                    if (reader.tell() != end && typeid(*object) != typeid(UObject)) {
                        registry().loadMismatches.push_back(object->fullName() + " consumed " + std::to_string(reader.tell() - static_cast<size_t>(entry.offset)) + " of " + std::to_string(entry.size));
                    }
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "warning: failed loading %s: %s\n", object->fullName().c_str(), error.what());
                }
                if (stackState != 0) {
                    UState* state = cast<UState>(linker.object(stackState));
                    UStruct* node = cast<UStruct>(linker.object(stackNode));
                    vm::restoreState(object, state, node, stackOffset, probeMask);
                    if (linker.savedGame && object->stateFrame) {
                        object->stateFrame->latentAction = legacyLatentAction(latentAction);
                        UState* frameState = object->stateFrame->state;
                        uint64_t base = frameState ? (frameState->probeMask | cls->probeMask) & frameState->ignoreMask : cls->probeMask & cls->ignoreMask;
                        object->stateFrame->disabledProbes = base & ~probeMask;
                    }
                }
            }
            syncObjectProperties(object);
            object->postLoad();
        }

        void allocateData(UObject* object)
        {
            UClass* cls = object->cls;
            if (!cls) {
                return;
            }
            if (!cls->defaultObject) {
                ensureLoaded(cls);
                cls->linkClass();
            }
            delete[] object->data;
            object->data = new uint8_t[std::max<size_t>(cls->size, 1)];
            cls->initValue(object->data);
            if (cls->defaultObject && cls->defaultObject != object) {
                cls->copyValue(object->data, cls->defaultObject->data);
            }
            syncObjectProperties(object);
        }

        void finishBootstrap()
        {
            Registry& reg = registry();
            reg.classClassPtr = intrinsicClass(Name("Class"));
            std::vector<UClass*> intrinsics;
            for (auto& [className, cls] : reg.intrinsics) {
                intrinsics.push_back(cls);
            }
            for (UClass* cls : intrinsics) {
                cls->cls = reg.classClassPtr;
                cls->linkClass();
            }
            for (UObject* object : std::vector<UObject*>(reg.table)) {
                if (object && cast<UClass>(object) && (!object->cls || !object->data)) {
                    if (!object->cls) {
                        object->cls = reg.classClassPtr;
                    }
                    allocateData(object);
                }
            }
            std::vector<UObject*> deferred = std::move(reg.deferredData);
            reg.deferredData.clear();
            for (UObject* object : deferred) {
                if (!object->cls) {
                    object->cls = reg.classClassPtr;
                }
                allocateData(object);
            }
        }

        UClass* objectClass()
        {
            Registry& reg = registry();
            if (reg.objectClassPtr) {
                return reg.objectClassPtr;
            }
            if (reg.bootstrapping) {
                Linker* core = findLinker(Name("Core"));
                if (!core) {
                    return nullptr;
                }
                return cast<UClass>(core->object(core->findExport(Name("Object"), Name("Class"), "")));
            }
            reg.bootstrapping = true;
            Linker* core = loadPackage(Name("Core"));
            if (!core) {
                throw std::runtime_error("Core package not found");
            }
            UClass* object = cast<UClass>(core->object(core->findExport(Name("Object"), Name("Class"), "")));
            ensureLoaded(object);
            reg.objectClassPtr = object;
            reg.nameProperty = object->findProperty(Name("Name"));
            reg.classProperty = object->findProperty(Name("Class"));
            reg.outerProperty = object->findProperty(Name("Outer"));
            reg.flagsProperty = object->findProperty(Name("ObjectFlags"));
            reg.bootstrapping = false;
            finishBootstrap();
            return object;
        }

        UClass* classClass()
        {
            Registry& reg = registry();
            if (reg.classClassPtr) {
                return reg.classClassPtr;
            }
            return intrinsicClass(Name("Class"));
        }

        bool isIntrinsic(const UObject* object)
        {
            auto it = registry().intrinsics.find(object->name);
            return it != registry().intrinsics.end() && it->second == object;
        }

        UClass* intrinsicClass(Name className)
        {
            Registry& reg = registry();
            auto it = reg.intrinsics.find(className);
            if (it != reg.intrinsics.end()) {
                return it->second;
            }
            if (className == Name("Object")) {
                return objectClass();
            }
            UClass* cls = new UClass();
            cls->name = className;
            cls->flags = RF_Public | RF_Native;
            cls->loaded = true;
            reg.intrinsics.emplace(className, cls);
            registerObject(cls);
            static const std::unordered_set<std::string> engineIntrinsics = { "primitive", "mesh", "lodmesh", "model", "polys", "level", "levelbase", "sound", "music", "palette", "font", "viewport", "client", "player", "netconnection", "netdriver", "engine", "renderbase", "renderdevice", "audiosubsystem", "eventmanager", "aieventtype", "aievent", "aisenderevent", "aireceiverevent" };
            if (engineIntrinsics.count(lower(className.str()))) {
                cls->outer = packageObject(Name("Engine"));
            } else {
                Linker* core = findLinker(Name("Core"));
                cls->outer = core ? core->packageObject : nullptr;
            }
            auto super = intrinsicSupers().find(lower(className.str()));
            UClass* parent = nullptr;
            if (super != intrinsicSupers().end()) {
                parent = findClass(super->second);
            } else {
                parent = objectClass();
            }
            cls->superField = parent;
            cls->cls = className == Name("Class") ? cls : classClass();
            if (!reg.bootstrapping) {
                cls->linkClass();
                allocateData(cls);
            }
            return cls;
        }

        UClass* findClass(std::string_view path)
        {
            std::string text(path);
            size_t dot = text.rfind('.');
            if (dot != std::string::npos) {
                std::string packageName = text.substr(0, text.find('.'));
                Name className(text.substr(dot + 1));
                Linker* linker = loadPackage(Name(packageName));
                if (!linker) {
                    return nullptr;
                }
                int32_t ref = linker->findExport(className, Name("Class"), "*");
                if (ref == 0) {
                    auto it = registry().intrinsics.find(className);
                    return it != registry().intrinsics.end() ? it->second : nullptr;
                }
                UClass* cls = cast<UClass>(linker->object(ref));
                ensureLoaded(cls);
                return cls;
            }
            Name className(text);
            auto intrinsic = registry().intrinsics.find(className);
            if (intrinsic != registry().intrinsics.end()) {
                return intrinsic->second;
            }
            if (intrinsicSupers().count(lower(text))) {
                return intrinsicClass(className);
            }
            for (auto& [packageName, linker] : registry().linkers) {
                int32_t ref = linker->findExport(className, Name("Class"), "*");
                if (ref != 0) {
                    UClass* cls = cast<UClass>(linker->object(ref));
                    ensureLoaded(cls);
                    return cls;
                }
            }
            return nullptr;
        }

        UObject* findObject(std::string_view path, UClass* cls)
        {
            std::string text(path);
            size_t dot = text.find('.');
            if (dot == std::string::npos) {
                for (UObject* object : registry().table) {
                    if (object && object->name == Name(text) && (!cls || object->isA(cls))) {
                        return object;
                    }
                }
                return nullptr;
            }
            Linker* linker = findLinker(Name(text.substr(0, dot)));
            if (!linker) {
                return nullptr;
            }
            size_t last = text.rfind('.');
            std::string group = last > dot ? text.substr(dot + 1, last - dot - 1) : std::string();
            Name objectName(text.substr(last + 1));
            Name className = cls ? cls->name : Name();
            int32_t ref = linker->findExport(objectName, Name(), group);
            if (ref == 0) {
                ref = linker->findExport(objectName, Name(), "*");
            }
            if (ref == 0) {
                return nullptr;
            }
            UObject* object = linker->object(ref);
            ensureLoaded(object);
            if (cls && !object->isA(cls) && !className.none()) {
                return nullptr;
            }
            return object;
        }

        UObject* loadObject(std::string_view path, UClass* cls)
        {
            std::string text(path);
            size_t dot = text.find('.');
            if (dot != std::string::npos) {
                loadPackage(Name(text.substr(0, dot)));
            }
            if (cls == classClass()) {
                return findClass(text);
            }
            UObject* object = findObject(text, cls);
            flushLoads();
            return object;
        }

        UObject* createNative(UClass* cls)
        {
            for (UClass* current = cls; current; current = current->superClass()) {
                auto it = registry().factories.find(current->name);
                if (it != registry().factories.end()) {
                    return it->second();
                }
            }
            return new UObject();
        }

        Name uniqueName(UClass* cls)
        {
            Registry& reg = registry();
            uint32_t& counter = reg.nameCounters[cls->name];
            while (true) {
                Name candidate(cls->name.str() + std::to_string(counter++));
                if (reg.usedNames.insert(candidate).second) {
                    return candidate;
                }
            }
        }

        UObject* newObject(UClass* cls, UObject* outer, Name objectName, uint32_t flags)
        {
            ensureLoaded(cls);
            cls->linkClass();
            UObject* object = createNative(cls);
            object->cls = cls;
            object->outer = outer;
            object->name = objectName.none() ? uniqueName(cls) : objectName;
            object->flags = flags;
            object->data = new uint8_t[std::max<size_t>(cls->size, 1)];
            cls->initValue(object->data);
            cls->copyValue(object->data, cls->defaultObject->data);
            registerObject(object);
            syncObjectProperties(object);
            return object;
        }

        UClass* createSubclass(UClass* parent, Name className)
        {
            ensureLoaded(parent);
            parent->linkClass();
            UClass* cls = new UClass();
            cls->name = className;
            registry().usedNames.insert(className);
            cls->cls = classClass();
            cls->outer = parent->outer;
            cls->flags = parent->flags;
            cls->superField = parent;
            cls->classFlags = parent->classFlags;
            cls->within = parent->within;
            cls->configName = parent->configName;
            cls->data = new uint8_t[std::max<size_t>(cls->cls->size, 1)];
            cls->cls->initValue(cls->data);
            registerObject(cls);
            syncObjectProperties(cls);
            cls->linkClass();
            return cls;
        }

        uint64_t destroyedObjects = 0;

        void destroyObject(UObject* object)
        {
            if (object) {
                object->destroyed = true;
                destroyedObjects++;
            }
        }

        uint64_t destroyCount()
        {
            return destroyedObjects;
        }

        void syncObjectProperties(UObject* object)
        {
            Registry& reg = registry();
            if (!object->data || !reg.nameProperty || !object->cls || object->cls->size < reg.nameProperty->offset + 4) {
                return;
            }
            *reinterpret_cast<uint32_t*>(object->data + reg.nameProperty->offset) = object->name.index();
            if (reg.classProperty) {
                *reinterpret_cast<UObject**>(object->data + reg.classProperty->offset) = object->cls;
            }
            if (reg.outerProperty) {
                *reinterpret_cast<UObject**>(object->data + reg.outerProperty->offset) = object->outer;
            }
            if (reg.flagsProperty) {
                *reinterpret_cast<uint32_t*>(object->data + reg.flagsProperty->offset) = object->flags;
            }
        }

        namespace {
            void readStructBinary(UStruct* structType, uint8_t* memory, Reader& reader, Linker& linker);

            void readBinaryValue(UProperty* property, uint8_t* memory, Reader& reader, Linker& linker)
            {
                switch (property->type) {
                case PropType::Byte:
                    *memory = reader.u8();
                    break;
                case PropType::Int:
                    *reinterpret_cast<int32_t*>(memory) = reader.i32();
                    break;
                case PropType::Bool:
                    *reinterpret_cast<uint32_t*>(memory) = reader.u8() != 0 ? 1 : 0;
                    break;
                case PropType::Float:
                    *reinterpret_cast<float*>(memory) = reader.f32();
                    break;
                case PropType::Object:
                case PropType::Class:
                    *reinterpret_cast<UObject**>(memory) = linker.readObject(reader);
                    break;
                case PropType::Name:
                    *reinterpret_cast<uint32_t*>(memory) = linker.readName(reader).index();
                    break;
                case PropType::Str:
                case PropType::String:
                    *reinterpret_cast<std::string*>(memory) = reader.string();
                    break;
                case PropType::Struct:
                    readStructBinary(property->structType, memory, reader, linker);
                    break;
                case PropType::Array:
                case PropType::Map:
                case PropType::FixedArray:
                case PropType::Pointer:
                    throw std::runtime_error("unsupported binary property type");
                }
            }

            void readStructBinary(UStruct* structType, uint8_t* memory, Reader& reader, Linker& linker)
            {
                for (UProperty* member : structType->fieldOrder()) {
                    if (member->propertyFlags & (CPF_Native | CPF_Transient)) {
                        continue;
                    }
                    for (int32_t i = 0; i < member->arrayDim; i++) {
                        readBinaryValue(member, memory + member->offset + static_cast<size_t>(i) * member->elementSize(), reader, linker);
                    }
                }
            }
        }

        void readTaggedProperties(UStruct* structType, uint8_t* data, Reader& reader, Linker& linker)
        {
            while (true) {
                Name propertyName = linker.readName(reader);
                if (propertyName.none()) {
                    break;
                }
                uint8_t info = reader.u8();
                PropertyType tagType = static_cast<PropertyType>(info & 0x0f);
                Name structName;
                if (tagType == PropertyType::Struct) {
                    structName = linker.readName(reader);
                }
                size_t size = 0;
                switch ((info >> 4) & 0x07) {
                case 0:
                    size = 1;
                    break;
                case 1:
                    size = 2;
                    break;
                case 2:
                    size = 4;
                    break;
                case 3:
                    size = 12;
                    break;
                case 4:
                    size = 16;
                    break;
                case 5:
                    size = reader.u8();
                    break;
                case 6:
                    size = static_cast<uint16_t>(reader.i16());
                    break;
                default:
                    size = reader.u32();
                    break;
                }
                int32_t arrayIndex = 0;
                bool infoBit = (info & 0x80) != 0;
                if (tagType == PropertyType::Bool) {
                    size = 0;
                } else if (infoBit) {
                    uint8_t byte1 = reader.u8();
                    if ((byte1 & 0xc0) == 0xc0) {
                        uint8_t byte2 = reader.u8();
                        uint8_t byte3 = reader.u8();
                        uint8_t byte4 = reader.u8();
                        arrayIndex = ((byte1 & 0x3f) << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
                    } else if (byte1 & 0x80) {
                        arrayIndex = ((byte1 & 0x7f) << 8) | reader.u8();
                    } else {
                        arrayIndex = byte1;
                    }
                }
                size_t start = reader.tell();

                UProperty* property = nullptr;
                if (!structType || !data) {
                    reader.seek(start + size);
                    continue;
                }
                for (auto it = structType->properties.rbegin(); it != structType->properties.rend(); ++it) {
                    if ((*it)->name == propertyName) {
                        if (tagType != PropertyType::Struct || (*it)->type == PropType::Struct) {
                            property = *it;
                            break;
                        }
                    }
                }
                if (!property || arrayIndex < 0 || arrayIndex >= property->arrayDim) {
                    reader.seek(start + size);
                    continue;
                }
                uint8_t* memory = data + property->offset + static_cast<size_t>(arrayIndex) * property->elementSize();
                try {
                    switch (tagType) {
                    case PropertyType::Byte:
                        if (property->type == PropType::Byte) {
                            *memory = reader.u8();
                        }
                        break;
                    case PropertyType::Int:
                        if (property->type == PropType::Int) {
                            *reinterpret_cast<int32_t*>(memory) = reader.i32();
                        }
                        break;
                    case PropertyType::Bool:
                        if (property->type == PropType::Bool) {
                            *reinterpret_cast<uint32_t*>(memory) = infoBit ? 1 : 0;
                        }
                        break;
                    case PropertyType::Float:
                        if (property->type == PropType::Float) {
                            *reinterpret_cast<float*>(memory) = reader.f32();
                        }
                        break;
                    case PropertyType::Object:
                    case PropertyType::Class:
                        if (property->type == PropType::Object || property->type == PropType::Class) {
                            *reinterpret_cast<UObject**>(memory) = linker.readObject(reader);
                        }
                        break;
                    case PropertyType::Name:
                        if (property->type == PropType::Name) {
                            *reinterpret_cast<uint32_t*>(memory) = linker.readName(reader).index();
                        }
                        break;
                    case PropertyType::Str:
                        if (property->type == PropType::Str || property->type == PropType::String) {
                            *reinterpret_cast<std::string*>(memory) = reader.string();
                        }
                        break;
                    case PropertyType::String:
                        if (property->type == PropType::Str || property->type == PropType::String) {
                            std::string text(size, '\0');
                            reader.bytes(text.data(), size);
                            text.resize(std::strlen(text.c_str()));
                            *reinterpret_cast<std::string*>(memory) = text;
                        }
                        break;
                    case PropertyType::Struct:
                        if (property->type == PropType::Struct && property->structType) {
                            readStructBinary(property->structType, memory, reader, linker);
                        }
                        break;
                    case PropertyType::Vector:
                    case PropertyType::Rotator:
                    case PropertyType::Array:
                    case PropertyType::Map:
                    case PropertyType::FixedArray:
                        break;
                    }
                } catch (const std::exception&) {
                }
                reader.seek(start + size);
            }
        }
    }
}
