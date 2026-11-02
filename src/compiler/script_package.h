#pragma once

#include "core/object.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dx::compiler {
    std::vector<UObject*> packageContents(UObject* package);
    void discardStaleFields(UObject* package);
    bool saveScriptPackage(UObject* package, const std::filesystem::path& path, std::string* error);
    bool saveScriptPackage(UObject* package, const std::vector<UObject*>& contents, const std::filesystem::path& path, std::string* error);
}
