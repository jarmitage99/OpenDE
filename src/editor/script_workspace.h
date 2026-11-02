#pragma once

#include "core/object.h"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace dx::editor {
    struct CompileMessage {
        std::string text;
        UClass* cls = nullptr;
        int line = 0;
        bool error = false;
    };

    class ScriptWorkspace {
    public:
        bool compile(const std::map<UClass*, std::string>& edits, bool all);
        UClass* newClass(const std::string& packageName, const std::string& className, UClass* parent);
        int exportScripts(bool changedOnly);
        bool savePackage(const std::string& packageName);
        bool exportClass(UClass* cls);
        UClass* loadClass(const std::filesystem::path& file);
        bool deleteClass(UClass* cls);
        bool setDefault(UClass* cls, const std::string& property, const std::string& value);
        void markChanged(UClass* cls);
        std::vector<std::string> scriptPackages() const;
        std::filesystem::path sourcePath(UClass* cls) const;
        const std::vector<CompileMessage>& messages() const;
        const std::set<UClass*>& changed() const;

    private:
        void log(const std::string& line, const std::map<std::string, UClass*>& files);
        void report(const std::string& text, bool error);

        std::vector<CompileMessage> results;
        std::set<UClass*> changedClasses;
        std::map<UClass*, int> lineOffsets;
    };
}
