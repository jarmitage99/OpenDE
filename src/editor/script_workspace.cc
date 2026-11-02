#include "editor/script_workspace.h"

#include "compiler/exporter.h"
#include "compiler/script_compiler.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace dx::editor {
    namespace {
        struct SavedClass {
            UField* superField = nullptr;
            UField* next = nullptr;
            UTextBuffer* scriptText = nullptr;
            UField* children = nullptr;
            Name friendlyName;
            uint32_t line = 0;
            uint32_t textPos = 0;
            std::vector<uint8_t> script;
            std::vector<UProperty*> properties;
            std::vector<UProperty*> constructed;
            size_t size = 0;
            size_t alignment = 1;
            bool linked = false;
            uint64_t probeMask = 0;
            uint64_t ignoreMask = 0;
            uint16_t labelTableOffset = 0;
            uint32_t stateFlags = 0;
            std::unordered_map<Name, UFunction*> functions;
            uint32_t classFlags = 0;
            uint32_t guid[4] = {};
            std::vector<ClassDependency> dependencies;
            std::vector<Name> packageImports;
            UClass* within = nullptr;
            Name configName;
            UObject* defaultObject = nullptr;
            std::unordered_map<Name, UFunction*> vtable;
            std::unordered_map<Name, UState*> states;
            uint32_t flags = 0;
            std::vector<UObject*> owned;
        };

        SavedClass save(UClass* cls)
        {
            SavedClass state;
            state.superField = cls->superField;
            state.next = cls->next;
            state.scriptText = cls->scriptText;
            state.children = cls->children;
            state.friendlyName = cls->friendlyName;
            state.line = cls->line;
            state.textPos = cls->textPos;
            state.script = cls->script;
            state.properties = cls->properties;
            state.constructed = cls->constructed;
            state.size = cls->size;
            state.alignment = cls->alignment;
            state.linked = cls->linked;
            state.probeMask = cls->probeMask;
            state.ignoreMask = cls->ignoreMask;
            state.labelTableOffset = cls->labelTableOffset;
            state.stateFlags = cls->stateFlags;
            state.functions = cls->functions;
            state.classFlags = cls->classFlags;
            std::copy(std::begin(cls->guid), std::end(cls->guid), std::begin(state.guid));
            state.dependencies = cls->dependencies;
            state.packageImports = cls->packageImports;
            state.within = cls->within;
            state.configName = cls->configName;
            state.defaultObject = cls->defaultObject;
            state.vtable = cls->vtable;
            state.states = cls->states;
            state.flags = cls->flags;
            return state;
        }

        void clearProperties(UClass* cls)
        {
            cls->properties = std::vector<UProperty*>();
            cls->findProperty(Name());
        }

        void restore(UClass* cls, const SavedClass& state)
        {
            clearProperties(cls);
            cls->superField = state.superField;
            cls->next = state.next;
            cls->scriptText = state.scriptText;
            cls->children = state.children;
            cls->friendlyName = state.friendlyName;
            cls->line = state.line;
            cls->textPos = state.textPos;
            cls->script = state.script;
            cls->properties = state.properties;
            cls->constructed = state.constructed;
            cls->size = state.size;
            cls->alignment = state.alignment;
            cls->linked = state.linked;
            cls->probeMask = state.probeMask;
            cls->ignoreMask = state.ignoreMask;
            cls->labelTableOffset = state.labelTableOffset;
            cls->stateFlags = state.stateFlags;
            cls->functions = state.functions;
            cls->classFlags = state.classFlags;
            std::copy(std::begin(state.guid), std::end(state.guid), std::begin(cls->guid));
            cls->dependencies = state.dependencies;
            cls->packageImports = state.packageImports;
            cls->within = state.within;
            cls->configName = state.configName;
            cls->defaultObject = state.defaultObject;
            cls->vtable = state.vtable;
            cls->states = state.states;
            cls->flags = state.flags;
        }

        void resetLink(UClass* cls)
        {
            clearProperties(cls);
            cls->constructed.clear();
            cls->size = 0;
            cls->alignment = 1;
            cls->linked = false;
            cls->defaultObject = nullptr;
            cls->vtable.clear();
            cls->states.clear();
            cls->functions.clear();
        }

        UClass* ownerIn(UObject* object, const std::set<UClass*>& classes)
        {
            for (UObject* outer = object->outer; outer; outer = outer->outer) {
                UClass* cls = cast<UClass>(outer);
                if (cls && classes.count(cls)) {
                    return cls;
                }
            }
            return nullptr;
        }

        bool sameLayout(const SavedClass& old, const UClass* cls)
        {
            if (old.size != cls->size || old.properties.size() != cls->properties.size()) {
                return false;
            }
            for (size_t i = 0; i < old.properties.size(); i++) {
                const UProperty* a = old.properties[i];
                const UProperty* b = cls->properties[i];
                if (a->name != b->name || a->offset != b->offset || a->type != b->type || a->elementSize() != b->elementSize() || a->arrayDim != b->arrayDim) {
                    return false;
                }
            }
            return true;
        }

        bool compatible(const UProperty* a, const UProperty* b)
        {
            if (a->type != b->type || a->elementSize() != b->elementSize()) {
                return false;
            }
            if (a->type == PropType::Array || a->type == PropType::FixedArray) {
                return a->inner && b->inner && compatible(a->inner, b->inner);
            }
            if (a->type == PropType::Struct) {
                return a->structType && b->structType && a->structType->size == b->structType->size;
            }
            return true;
        }

        void migrate(UObject* object, const SavedClass& old)
        {
            UClass* cls = object->cls;
            std::unordered_map<Name, UProperty*> previous;
            for (UProperty* property : old.properties) {
                previous[property->name] = property;
            }
            uint8_t* fresh = new uint8_t[std::max<size_t>(cls->size, 1)];
            cls->initValue(fresh);
            cls->copyValue(fresh, cls->defaultObject->data);
            for (UProperty* property : cls->properties) {
                auto found = previous.find(property->name);
                if (found == previous.end() || !compatible(found->second, property)) {
                    continue;
                }
                int32_t count = std::min(property->arrayDim, found->second->arrayDim);
                for (int32_t i = 0; i < count; i++) {
                    size_t at = static_cast<size_t>(i) * property->elementSize();
                    property->copy(fresh + property->offset + at, object->data + found->second->offset + at);
                }
            }
            for (UProperty* property : old.constructed) {
                property->destroyAll(object->data + property->offset);
            }
            delete[] object->data;
            object->data = fresh;
            objects::syncObjectProperties(object);
        }

        void loadClasses()
        {
            const std::vector<UObject*>& table = objects::all();
            for (size_t i = 0; i < table.size(); i++) {
                UClass* cls = cast<UClass>(table[i]);
                if (cls && !cls->destroyed && !cls->loaded) {
                    objects::ensureLoaded(cls);
                }
            }
            objects::flushLoads();
            for (size_t i = 0; i < table.size(); i++) {
                UClass* cls = cast<UClass>(table[i]);
                if (cls && !cls->destroyed && cls->scriptText) {
                    objects::ensureLoaded(cls->scriptText);
                }
            }
            objects::flushLoads();
        }

        bool validName(const std::string& text)
        {
            if (text.empty() || std::isdigit(static_cast<unsigned char>(text[0]))) {
                return false;
            }
            return std::all_of(text.begin(), text.end(), [](char c) {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            });
        }

        bool iequals(const std::string& a, const std::string& b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
            });
        }

        bool definesDefaults(const std::string& text)
        {
            std::istringstream lines(text);
            std::string line;
            while (std::getline(lines, line)) {
                size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos && iequals(line.substr(start, 17), "defaultproperties")) {
                    return true;
                }
            }
            return false;
        }

        void destroyArtifacts(size_t firstNew, std::set<UClass*> classes, const std::vector<UObject*>& packages)
        {
            const std::vector<UObject*>& table = objects::all();
            for (size_t i = firstNew; i < table.size(); i++) {
                UClass* cls = cast<UClass>(table[i]);
                if (cls && std::find(packages.begin(), packages.end(), cls->outer) != packages.end()) {
                    classes.insert(cls);
                }
            }
            for (size_t i = firstNew; i < table.size(); i++) {
                UObject* object = table[i];
                if (!object || object->destroyed) {
                    continue;
                }
                UClass* cls = cast<UClass>(object);
                bool artifact = ownerIn(object, classes) != nullptr || (cls && classes.count(cls));
                if (object->cls && classes.count(object->cls) && object->name.str().rfind("Default__", 0) == 0) {
                    artifact = true;
                }
                if (artifact) {
                    object->destroyed = true;
                }
            }
        }
    }

    bool ScriptWorkspace::compile(const std::map<UClass*, std::string>& edits, bool all)
    {
        results.clear();
        lineOffsets.clear();
        loadClasses();
        std::map<UClass*, std::vector<UClass*>> children;
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (cls && !cls->destroyed && cls->superClass()) {
                children[cls->superClass()].push_back(cls);
            }
        }
        for (auto& entry : children) {
            std::sort(entry.second.begin(), entry.second.end(), [](UClass* a, UClass* b) {
                return a->name.str() < b->name.str();
            });
        }
        std::set<UClass*> targets;
        std::function<void(UClass*)> addTree = [&](UClass* cls) {
            if (!cls->scriptText || !targets.insert(cls).second) {
                return;
            }
            for (UClass* child : children[cls]) {
                addTree(child);
            }
        };
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (!cls || cls->destroyed || !cls->scriptText) {
                continue;
            }
            bool stale = (cls->classFlags & (CLASS_Parsed | CLASS_Compiled)) != (CLASS_Parsed | CLASS_Compiled);
            if (all || stale || edits.count(cls)) {
                addTree(cls);
            }
        }
        if (targets.empty()) {
            report("Success: Everything is up to date", false);
            return true;
        }
        std::vector<UClass*> ordered;
        std::function<void(UClass*)> visit = [&](UClass* cls) {
            if (targets.count(cls)) {
                ordered.push_back(cls);
            }
            for (UClass* child : children[cls]) {
                visit(child);
            }
        };
        visit(objects::objectClass());
        std::vector<UObject*> packages;
        std::map<UClass*, std::string> sources;
        std::map<UClass*, SavedClass> saved;
        std::map<std::string, UClass*> files;
        for (UClass* cls : ordered) {
            if (std::find(packages.begin(), packages.end(), cls->outer) == packages.end()) {
                packages.push_back(cls->outer);
            }
            std::string original = cls->scriptText->text;
            auto edit = edits.find(cls);
            if (edit != edits.end() && definesDefaults(edit->second)) {
                sources[cls] = edit->second;
            } else {
                if (edit != edits.end()) {
                    cls->scriptText->text = edit->second;
                    size_t body = edit->second.find_first_not_of("\r\n ");
                    lineOffsets[cls] = static_cast<int>(std::count(edit->second.begin(), edit->second.begin() + static_cast<std::ptrdiff_t>(std::min(body, edit->second.size())), '\n'));
                }
                sources[cls] = compiler::exportClass(cls);
                cls->scriptText->text = original;
            }
            saved[cls] = save(cls);
            files[sourcePath(cls).string()] = cls;
        }
        for (UObject* object : objects::all()) {
            if (!object || object->destroyed) {
                continue;
            }
            if (UClass* owner = ownerIn(object, targets)) {
                saved[owner].owned.push_back(object);
            }
        }
        size_t firstNew = objects::all().size();
        bool success = true;
        for (UObject* package : packages) {
            std::vector<UClass*> group;
            for (UClass* cls : ordered) {
                if (cls->outer == package) {
                    group.push_back(cls);
                }
            }
            for (UClass* cls : group) {
                for (UObject* object : saved[cls].owned) {
                    object->destroyed = true;
                }
                if (cls->defaultObject) {
                    cls->defaultObject->destroyed = true;
                }
                resetLink(cls);
            }
            compiler::PackageCompiler packageCompiler(package, [&](const std::string& line) {
                log(line, files);
            });
            for (UClass* cls : group) {
                if (packageCompiler.importClass(sourcePath(cls), sources[cls]) != cls) {
                    report("Error, class " + cls->name.str() + " cannot be renamed or redefined here", true);
                    success = false;
                    break;
                }
            }
            if (!success || !packageCompiler.compile()) {
                success = false;
                break;
            }
        }
        if (!success) {
            destroyArtifacts(firstNew, targets, packages);
            for (auto& [cls, state] : saved) {
                restore(cls, state);
                for (UObject* object : state.owned) {
                    object->destroyed = false;
                }
                if (state.defaultObject) {
                    state.defaultObject->destroyed = false;
                }
            }
            report("Failure: compile errors, all classes restored", true);
            return false;
        }
        std::map<UClass*, std::vector<UObject*>> instances;
        for (UObject* object : objects::all()) {
            if (object && !object->destroyed && object->data && object->cls && targets.count(object->cls) && object != object->cls->defaultObject) {
                instances[object->cls].push_back(object);
            }
        }
        for (UClass* cls : ordered) {
            const SavedClass& old = saved[cls];
            if (sameLayout(old, cls)) {
                continue;
            }
            if (cls->flags & RF_Native) {
                report("Warning, native class " + cls->name.str() + " size mismatch (script " + std::to_string(cls->size) + ", was " + std::to_string(old.size) + ")", false);
            }
            for (UObject* object : instances[cls]) {
                migrate(object, old);
            }
        }
        for (const auto& edit : edits) {
            changedClasses.insert(edit.first);
        }
        report("Success: Compiled " + std::to_string(ordered.size()) + " class(es)", false);
        return true;
    }

    UClass* ScriptWorkspace::newClass(const std::string& packageName, const std::string& className, UClass* parent)
    {
        results.clear();
        if (!validName(packageName) || !validName(className)) {
            report("Error, invalid package or class name", true);
            return nullptr;
        }
        if (parent) {
            objects::ensureLoaded(parent);
            objects::ensureLoaded(parent->scriptText);
            objects::flushLoads();
        }
        if (!parent || !parent->scriptText) {
            report("Error, parent class has no script", true);
            return nullptr;
        }
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (cls && !cls->destroyed && iequals(cls->name.str(), className)) {
                report("Error, class " + cls->pathName() + " already exists", true);
                return nullptr;
            }
        }
        std::string rule = "//=============================================================================\r\n";
        std::string text = rule + "// " + className + ".\r\n" + rule + "class " + className + " expands " + parent->name.str() + ";\r\n";
        std::filesystem::path path = config::userDirectory() / packageName / "Classes" / (className + ".uc");
        std::map<std::string, UClass*> files;
        size_t firstNew = objects::all().size();
        UObject* package = objects::packageObject(Name(packageName));
        compiler::PackageCompiler packageCompiler(package, [&](const std::string& line) {
            log(line, files);
        });
        UClass* cls = packageCompiler.importClass(path, text);
        if (cls) {
            files[path.string()] = cls;
        }
        if (!cls || !packageCompiler.compile()) {
            destroyArtifacts(firstNew, {}, { package });
            report("Failure: class " + className + " was not created", true);
            return nullptr;
        }
        changedClasses.insert(cls);
        report("Created " + packageName + "." + className, false);
        return cls;
    }

    int ScriptWorkspace::exportScripts(bool changedOnly)
    {
        results.clear();
        loadClasses();
        int count = 0;
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (!cls || cls->destroyed || !cls->scriptText) {
                continue;
            }
            if (changedOnly && !changedClasses.count(cls)) {
                continue;
            }
            std::filesystem::path path = sourcePath(cls);
            std::error_code error;
            std::filesystem::create_directories(path.parent_path(), error);
            std::ofstream out(path, std::ios::binary);
            out << compiler::exportClass(cls);
            if (!out) {
                report("Error, cannot write " + path.string(), true);
                continue;
            }
            count++;
        }
        if (changedOnly) {
            changedClasses.clear();
        }
        report("Exported " + std::to_string(count) + " class(es) to " + config::userDirectory().string(), false);
        return count;
    }

    bool ScriptWorkspace::savePackage(const std::string& packageName)
    {
        results.clear();
        Name name(packageName);
        if (Linker* linker = objects::findLinker(name)) {
            for (size_t i = 0; i < linker->file().exports().size(); i++) {
                objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
            }
            objects::flushLoads();
        }
        UObject* package = objects::packageObject(name);
        for (UObject* object : compiler::packageContents(package)) {
            if (!cast<UField>(object) && !cast<UTextBuffer>(object)) {
                std::string kind = object->cls ? object->cls->name.str() : std::string("object");
                report("Error, cannot save " + packageName + ": " + kind + " " + object->name.str() + " is not script content", true);
                return false;
            }
        }
        std::filesystem::path path = config::userDirectory() / "System" / (packageName + ".u");
        std::error_code ignored;
        std::filesystem::create_directories(path.parent_path(), ignored);
        std::string error;
        if (!compiler::saveScriptPackage(package, path, &error)) {
            report("Error, failed to save " + path.string() + ": " + error, true);
            return false;
        }
        report("Saved " + path.string(), false);
        return true;
    }

    bool ScriptWorkspace::exportClass(UClass* cls)
    {
        results.clear();
        if (!cls || !cls->scriptText) {
            report("Error, class has no script to export", true);
            return false;
        }
        objects::ensureLoaded(cls->scriptText);
        objects::flushLoads();
        std::filesystem::path path = sourcePath(cls);
        std::error_code ignored;
        std::filesystem::create_directories(path.parent_path(), ignored);
        std::ofstream out(path, std::ios::binary);
        out << compiler::exportClass(cls);
        if (!out) {
            report("Error, cannot write " + path.string(), true);
            return false;
        }
        changedClasses.erase(cls);
        report("Exported " + cls->name.str() + " to " + path.string(), false);
        return true;
    }

    UClass* ScriptWorkspace::loadClass(const std::filesystem::path& file)
    {
        results.clear();
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            report("Error, cannot open " + file.string(), true);
            return nullptr;
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string text = buffer.str();
        std::string className = file.stem().string();
        std::string packageName = iequals(file.parent_path().filename().string(), "Classes") ? file.parent_path().parent_path().filename().string() : std::string("MyLevel");
        if (!validName(packageName) || !validName(className)) {
            report("Error, invalid package or class name for " + file.string(), true);
            return nullptr;
        }
        loadClasses();
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (!cls || cls->destroyed || !iequals(cls->name.str(), className)) {
                continue;
            }
            if (!cls->scriptText || !cls->outer || !iequals(cls->outer->name.str(), packageName)) {
                report("Error, class " + cls->pathName() + " already exists", true);
                return nullptr;
            }
            if (!compile({ { cls, text } }, false)) {
                return nullptr;
            }
            report("Loaded " + cls->pathName() + " from " + file.string(), false);
            return cls;
        }
        std::map<std::string, UClass*> files;
        size_t firstNew = objects::all().size();
        UObject* package = objects::packageObject(Name(packageName));
        compiler::PackageCompiler packageCompiler(package, [&](const std::string& line) {
            log(line, files);
        });
        UClass* cls = packageCompiler.importClass(file, text);
        if (cls) {
            files[file.string()] = cls;
        }
        bool compiled = cls && packageCompiler.compile() && (cls->classFlags & CLASS_Compiled);
        if (!compiled) {
            if (cls && cls->superClass() && !cls->superClass()->scriptText) {
                report("Error, superclass " + cls->superClass()->name.str() + " of class " + className + " not found", true);
            }
            destroyArtifacts(firstNew, {}, { package });
            report("Failure: " + file.string() + " was not loaded", true);
            return nullptr;
        }
        changedClasses.insert(cls);
        report("Loaded " + cls->pathName() + " from " + file.string(), false);
        return cls;
    }

    bool ScriptWorkspace::deleteClass(UClass* cls)
    {
        results.clear();
        if (!cls || !cls->scriptText || (cls->flags & RF_Native)) {
            report("Error, cannot delete a built-in or native class", true);
            return false;
        }
        for (UObject* object : objects::all()) {
            if (!object || object->destroyed || object == cls || object == cls->defaultObject) {
                continue;
            }
            UClass* other = cast<UClass>(object);
            if (other && other->superClass() == cls) {
                report("Error, cannot delete " + cls->name.str() + ": it has subclass " + other->name.str(), true);
                return false;
            }
            if (object->cls == cls) {
                report("Error, cannot delete " + cls->name.str() + ": it is used by " + object->pathName(), true);
                return false;
            }
            UProperty* property = cast<UProperty>(object);
            if (property && (property->objectClass == cls || property->metaClass == cls) && !ownerIn(property, { cls })) {
                report("Error, cannot delete " + cls->name.str() + ": it is referenced by " + property->pathName(), true);
                return false;
            }
        }
        for (UObject* object : objects::all()) {
            if (object && !object->destroyed && ownerIn(object, { cls })) {
                object->destroyed = true;
            }
        }
        if (cls->defaultObject) {
            cls->defaultObject->destroyed = true;
        }
        cls->destroyed = true;
        changedClasses.erase(cls);
        report("Deleted " + cls->pathName(), false);
        return true;
    }

    bool ScriptWorkspace::setDefault(UClass* cls, const std::string& property, const std::string& value)
    {
        results.clear();
        std::string name = property;
        int32_t index = 0;
        size_t bracket = name.find_first_of("([");
        if (bracket != std::string::npos) {
            index = std::atoi(name.c_str() + bracket + 1);
            name = name.substr(0, bracket);
        }
        UProperty* found = cls && cls->defaultObject ? cls->findProperty(Name(name)) : nullptr;
        if (!found || index < 0 || index >= found->arrayDim) {
            report("Error, " + (cls ? cls->name.str() : std::string("class")) + " has no property " + property, true);
            return false;
        }
        uint8_t* memory = cls->defaultObject->data + found->offset + static_cast<size_t>(index) * found->elementSize();
        if (!found->fromText(memory, value)) {
            report("Error, cannot set " + property + " to " + value, true);
            return false;
        }
        changedClasses.insert(cls);
        report("Set default " + cls->name.str() + "." + property + " to " + found->toText(memory), false);
        return true;
    }

    void ScriptWorkspace::markChanged(UClass* cls)
    {
        changedClasses.insert(cls);
    }

    std::vector<std::string> ScriptWorkspace::scriptPackages() const
    {
        std::set<std::string> names;
        for (UObject* object : objects::all()) {
            UClass* cls = cast<UClass>(object);
            if (cls && !cls->destroyed && cls->scriptText && cls->outer) {
                names.insert(cls->outer->name.str());
            }
        }
        return std::vector<std::string>(names.begin(), names.end());
    }

    std::filesystem::path ScriptWorkspace::sourcePath(UClass* cls) const
    {
        return config::userDirectory() / cls->outer->name.str() / "Classes" / (cls->name.str() + ".uc");
    }

    const std::vector<CompileMessage>& ScriptWorkspace::messages() const
    {
        return results;
    }

    const std::set<UClass*>& ScriptWorkspace::changed() const
    {
        return changedClasses;
    }

    void ScriptWorkspace::log(const std::string& line, const std::map<std::string, UClass*>& files)
    {
        if (line.rfind("Parsing ", 0) == 0 || line.rfind("Compiling ", 0) == 0) {
            return;
        }
        static const std::regex located(R"(^(.*)\((\d+)\) : (Error|Warning), (.*)$)");
        std::smatch match;
        CompileMessage message;
        message.text = line;
        if (std::regex_match(line, match, located)) {
            auto found = files.find(match[1].str());
            message.cls = found != files.end() ? found->second : nullptr;
            message.line = std::stoi(match[2].str());
            auto offset = lineOffsets.find(message.cls);
            if (offset != lineOffsets.end()) {
                message.line += offset->second;
            }
            message.error = match[3].str() == "Error";
            if (message.cls) {
                message.text = message.cls->name.str() + "(" + std::to_string(message.line) + ") : " + match[3].str() + ", " + match[4].str();
            }
        } else {
            message.error = line.find("Error") != std::string::npos || line.find("Failure") != std::string::npos;
        }
        results.push_back(message);
    }

    void ScriptWorkspace::report(const std::string& text, bool error)
    {
        CompileMessage message;
        message.text = text;
        message.error = error;
        results.push_back(message);
    }
}
