#pragma once

#include "core/object.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dx::compiler {
    std::vector<UObject*> packageContents(UObject* package);
    bool saveScriptPackage(UObject* package, const std::filesystem::path& path, std::string* error);
}
