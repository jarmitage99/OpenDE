#include "compiler/asset_package.h"

#include "compiler/assets.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/resources.h"

#include <algorithm>
#include <unordered_map>

namespace dx::compiler {
    void loadPackageExports(UObject* package)
    {
        Linker* linker = objects::findLinker(package->name);
        if (!linker) {
            return;
        }
        for (size_t i = 0; i < linker->file().exports().size(); i++) {
            objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
        }
        objects::flushLoads();
    }

    bool runAssetCommand(UObject* package, const std::filesystem::path& directory, const std::string& command, std::vector<std::string>& log)
    {
        loadPackageExports(package);
        std::vector<std::string> lines;
        ExecContext context;
        context.package = package;
        context.directory = directory;
        context.log = [&lines](const std::string& line) {
            lines.push_back(line);
        };
        bool handled = importAsset(context, command);
        finishAssets(context);
        if (!handled) {
            lines.push_back("Warning: unsupported command " + command);
        }
        log.insert(log.end(), lines.begin(), lines.end());
        return lines.empty();
    }

    std::filesystem::path assetPackagePath(UObject* package)
    {
        loadPackageExports(package);
        bool code = false;
        bool sounds = false;
        bool music = false;
        bool textures = false;
        for (UObject* object : packageContents(package)) {
            if (cast<UClass>(object) || cast<UMesh>(object)) {
                code = true;
            } else if (cast<USound>(object)) {
                sounds = true;
            } else if (cast<UMusic>(object)) {
                music = true;
            } else if (cast<UTexture>(object) || cast<UFont>(object)) {
                textures = true;
            }
        }
        std::filesystem::path user = config::userDirectory();
        std::string name = package->name.str();
        if (code) {
            return user / "System" / (name + ".u");
        }
        if (sounds) {
            return user / "Sounds" / (name + ".uax");
        }
        if (music) {
            return user / "Music" / (name + ".umx");
        }
        if (textures) {
            return user / "Textures" / (name + ".utx");
        }
        return user / "System" / (name + ".u");
    }

    bool saveAssetPackage(UObject* package, const std::filesystem::path& path, std::string* error)
    {
        loadPackageExports(package);
        std::error_code ignored;
        std::filesystem::create_directories(path.parent_path(), ignored);
        std::vector<UObject*> contents = packageContents(package);
        std::unordered_map<UObject*, size_t> order;
        if (Linker* linker = objects::findLinker(package->name)) {
            for (size_t i = 0; i < linker->file().exports().size(); i++) {
                order.emplace(linker->object(static_cast<int32_t>(i + 1)), i);
            }
        }
        std::stable_sort(contents.begin(), contents.end(), [&order](UObject* a, UObject* b) {
            auto first = order.find(a);
            auto second = order.find(b);
            return (first != order.end() ? first->second : order.size()) < (second != order.end() ? second->second : order.size());
        });
        if (!saveScriptPackage(package, contents, path, error)) {
            return false;
        }
        objects::setPackageFile(package->name, path);
        return true;
    }
}
