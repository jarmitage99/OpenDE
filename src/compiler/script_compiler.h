#pragma once

#include "core/object.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dx::compiler {
    using LogSink = std::function<void(const std::string&)>;

    class PackageCompiler {
    public:
        PackageCompiler(UObject* package, LogSink log);
        ~PackageCompiler();
        PackageCompiler(const PackageCompiler&) = delete;
        PackageCompiler& operator=(const PackageCompiler&) = delete;

        void setNativeClasses(std::vector<Name> names);
        UClass* importClass(const std::filesystem::path& file, const std::string& text);
        bool compile();
        int errors() const;

        struct Impl;

    private:
        std::unique_ptr<Impl> impl;
    };
}
