#include "compiler/script_compiler.h"

#include "compiler/script_parser.h"
#include "core/objects.h"
#include "engine/package.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx::compiler {
    namespace {
        std::vector<std::string> splitLines(const std::string& text)
        {
            std::vector<std::string> lines;
            size_t at = 0;
            while (at < text.size()) {
                size_t end = text.find_first_of("\r\n", at);
                lines.push_back(text.substr(at, end == std::string::npos ? std::string::npos : end - at));
                at = end == std::string::npos ? text.size() : end;
                if (at < text.size() && text[at] == '\r') {
                    at++;
                }
                if (at < text.size() && text[at] == '\n') {
                    at++;
                }
            }
            return lines;
        }

        bool parseCommand(const std::string& line, const char* word)
        {
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) {
                return false;
            }
            size_t length = std::strlen(word);
            if (line.size() - start < length || !iequals(std::string_view(line).substr(start, length), word)) {
                return false;
            }
            size_t after = start + length;
            return after >= line.size() || !detail::isAlnum(line[after]);
        }

        size_t findNoCase(const std::string& haystack, const char* needle)
        {
            std::string lowered = haystack;
            std::string target = needle;
            for (char& c : lowered) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            for (char& c : target) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return lowered.find(target);
        }

        std::string parseToken(const std::string& line, size_t at)
        {
            while (at < line.size() && (line[at] == ' ' || line[at] == 9)) {
                at++;
            }
            std::string token;
            while (at < line.size() && line[at] != ' ' && line[at] != 9) {
                token += line[at++];
            }
            return token;
        }

        void resetDefaults(UClass* cls)
        {
            uint8_t* data = cls->defaultObject->data;
            cls->destroyValue(data);
            cls->initValue(data);
            if (UClass* parent = cls->superClass()) {
                parent->copyValue(data, parent->defaultObject->data);
            }
            objects::syncObjectProperties(cls->defaultObject);
        }
    }

    PackageCompiler::PackageCompiler(UObject* package, LogSink log)
        : impl(std::make_unique<Impl>())
    {
        impl->package = package;
        impl->log = std::move(log);
        impl->lookup.refresh();
    }

    PackageCompiler::~PackageCompiler() = default;

    void PackageCompiler::setNativeClasses(std::vector<Name> names)
    {
        impl->nativeClasses = std::move(names);
    }

    int PackageCompiler::errors() const
    {
        return impl->errorCount;
    }

    UClass* PackageCompiler::importClass(const std::filesystem::path& file, const std::string& text)
    {
        std::string scriptText;
        std::string defaultText;
        std::string className;
        std::string baseClassName;
        std::vector<std::string> lines = splitLines(text);
        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            if (parseCommand(line, "defaultproperties")) {
                for (i++; i < lines.size(); i++) {
                    size_t first = lines[i].find_first_not_of(" \t");
                    if (first != std::string::npos && lines[i][first] == '}') {
                        break;
                    }
                    defaultText += lines[i] + "\r\n";
                }
                continue;
            }
            scriptText += line + "\r\n";
            size_t comment = line.find("//");
            if (comment != std::string::npos) {
                line = line.substr(0, comment);
            }
            size_t at = std::string::npos;
            if (className.empty() && (at = findNoCase(line, "class")) != std::string::npos) {
                className = parseToken(line, at + 6);
            }
            if (baseClassName.empty() && ((at = findNoCase(line, "expands")) != std::string::npos || (at = findNoCase(line, "extends")) != std::string::npos)) {
                baseClassName = parseToken(line, at + 7);
                while (!baseClassName.empty() && baseClassName.back() == ';') {
                    baseClassName.pop_back();
                }
            }
        }
        std::string expected = file.stem().string();
        Name fileName(expected);
        if (className.empty() || (baseClassName.empty() && className != "Object")) {
            impl->log("Bad class definition '" + className + "'/'" + baseClassName + "' in " + file.string());
            impl->errorCount++;
            return nullptr;
        }
        if (!iequals(className, expected)) {
            impl->log("Script vs. class name mismatch (" + expected + "/" + className + ")");
        }
        Lookup& lookup = impl->lookup;
        auto inPackage = [&](const std::string& name) -> UClass* {
            Name found = Name::find(name);
            if (found.none()) {
                return nullptr;
            }
            for (UObject* object : objects::all()) {
                UClass* cls = cast<UClass>(object);
                if (cls && !cls->destroyed && cls->outer == impl->package && cls->name == found) {
                    return cls;
                }
            }
            return nullptr;
        };
        UClass* result = inPackage(className);
        if (!result) {
            Name classObjectName = iequals(className, expected) ? fileName : Name(className);
            result = detail::createField(new UClass(), impl->package, classObjectName, nullptr, RF_Public | RF_Standalone);
            lookup.add(result);
        }
        if (std::find(impl->nativeClasses.begin(), impl->nativeClasses.end(), result->name) != impl->nativeClasses.end()) {
            result->flags |= RF_Native;
        }
        UClass* base = inPackage(baseClassName);
        if (!base) {
            base = lookup.findClass(baseClassName);
        }
        if (!base && !baseClassName.empty()) {
            base = detail::createField(new UClass(), impl->package, Name(baseClassName), nullptr, RF_Public | RF_Standalone);
            base->superField = result;
            lookup.add(base);
        }
        result->superField = base;
        UTextBuffer* buffer = detail::createField(new UTextBuffer(), result, Name("ScriptText"), "TextBuffer", RF_NotForClient | RF_NotForServer);
        buffer->text = scriptText;
        result->scriptText = buffer;
        result->classFlags &= ~(CLASS_Parsed | CLASS_Compiled);
        impl->defaultProperties[result] = defaultText;
        impl->files[result] = file;
        impl->imported.push_back(result);
        return result;
    }

    bool PackageCompiler::compile()
    {
        Impl& context = *impl;
        std::vector<UClass*> allClasses;
        for (UObject* object : objects::all()) {
            if (UClass* cls = cast<UClass>(object)) {
                if (!cls->destroyed) {
                    allClasses.push_back(cls);
                }
            }
        }
        std::unordered_map<UClass*, std::vector<UClass*>> subclasses;
        for (UClass* cls : allClasses) {
            subclasses[cls->superClass()].push_back(cls);
        }
        detail::ScriptCompiler compiler(context);
        std::function<bool(UClass*)> parse = [&](UClass* cls) {
            if (!cls->scriptText) {
                return true;
            }
            if (!(cls->classFlags & CLASS_Parsed)) {
                if (!compiler.compileScript(cls, 0)) {
                    return false;
                }
                cls->linkClass();
            }
            for (UClass* child : subclasses[cls]) {
                if (!parse(child)) {
                    return false;
                }
            }
            return true;
        };
        std::function<bool(UClass*)> compileTree = [&](UClass* cls) {
            if (cls->scriptText && !(cls->classFlags & CLASS_Compiled)) {
                if (!compiler.compileScript(cls, 1)) {
                    return false;
                }
                resetDefaults(cls);
                detail::importDefaults(context, cls, context.defaultProperties[cls]);
            }
            for (UClass* child : subclasses[cls]) {
                if (!compileTree(child)) {
                    return false;
                }
            }
            return true;
        };
        UClass* root = objects::objectClass();
        bool success = parse(root) && compileTree(root);
        return success && context.errorCount == 0;
    }
}
