#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace dx::compiler {
    struct DumpedExport {
        std::string className;
        std::string super;
        uint32_t flags = 0;
        std::string body;
    };

    std::map<std::string, DumpedExport> dumpPackage(const std::filesystem::path& file, const std::filesystem::path& systemDirectory);
}
