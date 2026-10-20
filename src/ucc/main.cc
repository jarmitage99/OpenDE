#include "compiler/exporter.h"
#include "compiler/script_compiler.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/world.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
    void usage()
    {
        std::printf("Usage:\n");
        std::printf("   ucc <command> <parameters>\n\n");
        std::printf("Commands for \"ucc\":\n");
        std::printf("   ucc batchexport <package.u> class uc <directory>   Export UnrealScript sources\n");
        std::printf("   ucc make                                           Rebuild UnrealScript packages\n");
    }

    std::string lower(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    void loadFully(dx::Linker* linker)
    {
        for (size_t i = 0; i < linker->file().exports().size(); i++) {
            dx::objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
        }
        dx::objects::flushLoads();
    }

    int batchExport(const std::vector<std::string>& args)
    {
        if (args.size() < 4 || args[1] != "class" || args[2] != "uc") {
            std::fprintf(stderr, "usage: ucc batchexport <package.u> class uc <directory>\n");
            return 1;
        }
        std::string packageName = std::filesystem::path(args[0]).stem().string();
        std::filesystem::path directory(args[3]);
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(packageName));
        if (!linker) {
            std::fprintf(stderr, "cannot load package %s\n", packageName.c_str());
            return 1;
        }
        loadFully(linker);
        std::filesystem::create_directories(directory);
        const dx::Package& file = linker->file();
        int exported = 0;
        int skipped = 0;
        for (size_t i = 0; i < file.exports().size(); i++) {
            int32_t ref = static_cast<int32_t>(i + 1);
            if (file.classOf(ref) != "Class") {
                continue;
            }
            dx::UClass* cls = dx::cast<dx::UClass>(linker->object(ref));
            if (!cls) {
                continue;
            }
            if (!cls->scriptText) {
                std::printf("No script text for %s\n", cls->name.str().c_str());
                skipped++;
                continue;
            }
            dx::objects::ensureLoaded(cls->scriptText);
            std::ofstream out(directory / (cls->name.str() + ".uc"), std::ios::binary);
            out << dx::compiler::exportClass(cls);
            exported++;
        }
        std::printf("Exported %d classes, %d without script text\n", exported, skipped);
        return 0;
    }

    std::vector<std::filesystem::path> classFiles(const std::filesystem::path& directory)
    {
        std::vector<std::filesystem::path> files;
        for (const auto& item : std::filesystem::directory_iterator(directory)) {
            if (item.is_regular_file() && lower(item.path().extension().string()) == ".uc") {
                files.push_back(item.path());
            }
        }
        std::sort(files.begin(), files.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
            return lower(a.filename().string()) < lower(b.filename().string());
        });
        return files;
    }

    bool buildPackage(const std::filesystem::path& root, const std::string& name, const std::filesystem::path& sources, const std::filesystem::path& output)
    {
        std::printf("Building %s from %s\n", name.c_str(), sources.string().c_str());
        std::vector<std::filesystem::path> files = classFiles(sources);
        if (files.empty()) {
            std::printf("No files for package: %s\n", sources.string().c_str());
            return false;
        }
        dx::UObject* package = dx::objects::packageObject(dx::Name(name));
        dx::compiler::PackageCompiler compiler(package, [](const std::string& line) {
            std::printf("%s\n", line.c_str());
        });
        std::filesystem::path shipped = root / "System" / (name + ".u");
        if (std::filesystem::exists(shipped)) {
            dx::Package original(shipped);
            std::vector<dx::Name> natives;
            for (size_t i = 0; i < original.exports().size(); i++) {
                const dx::ExportEntry& entry = original.exports()[i];
                if (entry.classRef == 0 && (entry.flags & dx::RF_Native)) {
                    natives.emplace_back(original.nameAt(entry.name));
                }
            }
            compiler.setNativeClasses(std::move(natives));
        }
        for (const std::filesystem::path& file : files) {
            std::ifstream in(file, std::ios::binary);
            std::stringstream text;
            text << in.rdbuf();
            compiler.importClass(file, text.str());
        }
        if (!compiler.compile()) {
            std::printf("Failure - %d error(s)\n", compiler.errors());
            return false;
        }
        std::string error;
        if (!dx::compiler::saveScriptPackage(package, output, &error)) {
            std::printf("Failed to save %s: %s\n", output.string().c_str(), error.c_str());
            return false;
        }
        std::printf("Saved %s\n", output.string().c_str());
        for (dx::UObject* object : dx::compiler::packageContents(package)) {
            if (dx::UClass* cls = dx::cast<dx::UClass>(object)) {
                cls->defaultObject->destroyed = true;
            }
            object->destroyed = true;
        }
        dx::Linker* linker = dx::objects::loadPackageFile(dx::Name(name), output, false);
        if (!linker) {
            std::printf("Failed to reload %s\n", output.string().c_str());
            return false;
        }
        loadFully(linker);
        return true;
    }

    int make(const std::filesystem::path& root)
    {
        std::filesystem::path user = dx::config::userDirectory();
        std::vector<std::string> packages = dx::config::system().getAll("Editor.EditorEngine", "EditPackages");
        for (const std::string& name : packages) {
            std::printf("--------------------%s--------------------\n", name.c_str());
            std::filesystem::path output = user / "System" / (name + ".u");
            std::filesystem::path userSources = user / name / "Classes";
            std::filesystem::path gameSources = root / name / "Classes";
            if (std::filesystem::exists(output)) {
                dx::Linker* linker = dx::objects::loadPackage(dx::Name(name));
                if (!linker) {
                    std::printf("Failed to load %s\n", output.string().c_str());
                    return 1;
                }
                loadFully(linker);
                continue;
            }
            std::filesystem::path sources;
            if (std::filesystem::is_directory(userSources)) {
                sources = userSources;
            } else if (dx::Linker* linker = dx::objects::loadPackage(dx::Name(name))) {
                loadFully(linker);
                continue;
            } else if (std::filesystem::is_directory(gameSources)) {
                sources = gameSources;
            } else {
                std::printf("No files for package: %s\n", name.c_str());
                return 1;
            }
            if (!buildPackage(root, name, sources, output)) {
                return 1;
            }
        }
        std::printf("Success\n");
        return 0;
    }
}

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        usage();
        return 0;
    }
    std::filesystem::path root = dx::config::findGameRoot(DEUSEX_GAME_DATA);
    dx::objects::setEditorMode(true);
    dx::config::setApplyClassDefaults(false);
    std::string command = lower(args[0]);
    args.erase(args.begin());
    dx::engine::initializeCore(root.string());
    if (command == "batchexport") {
        return batchExport(args);
    }
    if (command == "make") {
        return make(root);
    }
    usage();
    return 1;
}
