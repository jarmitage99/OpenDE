#pragma once

#include "core/object.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dx::compiler {
    void loadPackageExports(UObject* package);
    bool runAssetCommand(UObject* package, const std::filesystem::path& directory, const std::string& command, std::vector<std::string>& log);
    std::filesystem::path assetPackagePath(UObject* package);
    bool saveAssetPackage(UObject* package, const std::filesystem::path& path, std::string* error);
}
