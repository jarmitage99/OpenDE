#include "compiler/assets.h"
#include "compiler/exec_parse.h"
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
#include <unordered_map>
#include <vector>

namespace {
    void usage()
    {
        std::printf("Usage:\n");
        std::printf("   ucc <command> <parameters>\n\n");
        std::printf("Commands for \"ucc\":\n");
        std::printf("   ucc batchexport <package.u> <class> <ext> <dir>    Export objects in bulk\n");
        std::printf("   ucc exportsources <package.u> <directory>          Export scripts and #exec source files\n");
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

    dx::Linker* loadForExport(const std::string& file)
    {
        std::string packageName = std::filesystem::path(file).stem().string();
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(packageName));
        if (!linker) {
            std::fprintf(stderr, "cannot load package %s\n", packageName.c_str());
            return nullptr;
        }
        loadFully(linker);
        return linker;
    }

    int exportClasses(dx::Linker* linker, const std::filesystem::path& directory)
    {
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

    int batchExport(const std::vector<std::string>& args)
    {
        if (args.size() < 4) {
            std::fprintf(stderr, "usage: ucc batchexport <package> <classname> <extension> <directory>\n");
            return 1;
        }
        dx::Linker* linker = loadForExport(args[0]);
        if (!linker) {
            return 1;
        }
        std::filesystem::path directory(args[3]);
        if (lower(args[1]) == "class" && lower(args[2]) == "uc") {
            return exportClasses(linker, directory);
        }
        dx::UClass* cls = dx::objects::findClass(args[1]);
        if (!cls) {
            std::fprintf(stderr, "class %s not found\n", args[1].c_str());
            return 1;
        }
        std::filesystem::create_directories(directory);
        int exported = 0;
        for (size_t i = 0; i < linker->file().exports().size(); i++) {
            dx::UObject* object = linker->object(static_cast<int32_t>(i + 1));
            if (!object || !object->cls || !object->isA(cls)) {
                continue;
            }
            std::filesystem::path file = directory / (object->name.str() + "." + args[2]);
            if (dx::compiler::exportAssetObject(object, lower(args[2]), file)) {
                std::printf("Exported %s to %s\n", object->pathName().c_str(), file.string().c_str());
                exported++;
            } else {
                std::printf("Can't export %s to %s\n", object->pathName().c_str(), args[2].c_str());
            }
        }
        std::printf("Exported %d objects\n", exported);
        return 0;
    }

    int exportSources(const std::vector<std::string>& args)
    {
        if (args.size() < 2) {
            std::fprintf(stderr, "usage: ucc exportsources <package> <directory>\n");
            return 1;
        }
        dx::Linker* linker = loadForExport(args[0]);
        if (!linker) {
            return 1;
        }
        std::filesystem::path root = std::filesystem::path(args[1]) / linker->packageName.str();
        exportClasses(linker, root / "Classes");
        dx::compiler::ExecContext context;
        context.package = linker->packageObject;
        context.directory = root;
        context.log = [](const std::string& line) {
            std::printf("%s\n", line.c_str());
        };
        int written = 0;
        int failed = 0;
        for (size_t i = 0; i < linker->file().exports().size(); i++) {
            dx::UClass* cls = dx::cast<dx::UClass>(linker->object(static_cast<int32_t>(i + 1)));
            if (!cls || !cls->scriptText) {
                continue;
            }
            std::istringstream lines(cls->scriptText->text);
            std::string line;
            while (std::getline(lines, line)) {
                std::string_view text = line;
                while (!text.empty() && (text.back() == '\r' || text.back() == '\n')) {
                    text.remove_suffix(1);
                }
                if (!dx::compiler::parseCommand(text, "#exec")) {
                    continue;
                }
                std::string_view objText = text;
                if (dx::compiler::parseCommand(objText, "OBJ") && dx::compiler::parseCommand(objText, "LOAD")) {
                    continue;
                }
                if (dx::compiler::exportAssetSource(context, std::string(text))) {
                    written++;
                } else {
                    std::printf("Can't export source for #exec %s\n", std::string(text).c_str());
                    failed++;
                }
            }
        }
        std::printf("Exported sources for %d #exec directives, %d failed\n", written, failed);
        return failed == 0 ? 0 : 1;
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

    void rebindResident(dx::Linker* linker, dx::UObject* package)
    {
        std::unordered_map<std::string, dx::UObject*> live;
        for (dx::UObject* object : dx::compiler::packageContents(package)) {
            live.emplace(lower(object->pathName()), object);
        }
        dx::compiler::discardStaleFields(package);
        for (size_t i = 0; i < linker->file().exports().size(); i++) {
            int32_t ref = static_cast<int32_t>(i + 1);
            dx::UObject* cached = linker->object(ref);
            if (cached && cached->destroyed) {
                auto found = live.find(lower(cached->pathName()));
                linker->rebind(ref, found != live.end() ? found->second : nullptr);
            }
        }
    }

    std::string exportPath(const dx::Package& file, int32_t ref)
    {
        const dx::ExportEntry& entry = file.exportAt(ref);
        std::string name = file.nameAt(entry.name);
        return entry.outer > 0 ? exportPath(file, entry.outer) + "." + name : file.name() + "." + name;
    }

    bool buildPackage(const std::filesystem::path& root, const std::string& name, const std::filesystem::path& sources, const std::filesystem::path& output)
    {
        std::printf("Building %s from %s\n", name.c_str(), sources.string().c_str());
        std::vector<std::filesystem::path> files = classFiles(sources);
        if (files.empty()) {
            std::printf("No files for package: %s\n", sources.string().c_str());
            return false;
        }
        dx::Linker* residentLinker = dx::objects::findLinker(dx::Name(name));
        bool resident = residentLinker != nullptr;
        if (resident) {
            loadFully(residentLinker);
        }
        dx::objects::setPackageFile(dx::Name(name), {});
        dx::UObject* package = dx::objects::packageObject(dx::Name(name));
        dx::compiler::PackageCompiler compiler(package, [](const std::string& line) {
            std::printf("%s\n", line.c_str());
        });
        std::filesystem::path shipped = root / "System" / (name + ".u");
        std::unordered_map<std::string, size_t> conform;
        if (std::filesystem::exists(shipped)) {
            dx::Package original(shipped);
            for (size_t i = 0; i < original.exports().size(); i++) {
                conform.emplace(lower(exportPath(original, static_cast<int32_t>(i + 1))), i);
            }
            std::vector<dx::Name> natives;
            for (size_t i = 0; i < original.exports().size(); i++) {
                const dx::ExportEntry& entry = original.exports()[i];
                if (entry.classRef == 0 && (entry.flags & dx::RF_Native)) {
                    natives.emplace_back(original.nameAt(entry.name));
                }
            }
            compiler.setNativeClasses(std::move(natives));
            std::unordered_map<std::string, size_t> created;
            for (size_t i = 0; i < original.exports().size(); i++) {
                if (original.exports()[i].classRef == 0) {
                    created.emplace(lower(original.nameAt(original.exports()[i].name)), i);
                }
            }
            std::stable_sort(files.begin(), files.end(), [&](const std::filesystem::path& a, const std::filesystem::path& b) {
                auto first = created.find(lower(a.stem().string()));
                auto second = created.find(lower(b.stem().string()));
                size_t left = first != created.end() ? first->second : created.size();
                size_t right = second != created.end() ? second->second : created.size();
                return left < right;
            });
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
        if (resident) {
            rebindResident(residentLinker, package);
        }
        std::vector<dx::UObject*> contents = dx::compiler::packageContents(package);
        std::stable_sort(contents.begin(), contents.end(), [&](dx::UObject* a, dx::UObject* b) {
            auto first = conform.find(lower(a->pathName()));
            auto second = conform.find(lower(b->pathName()));
            size_t left = first != conform.end() ? first->second : conform.size();
            size_t right = second != conform.end() ? second->second : conform.size();
            return left < right;
        });
        std::string error;
        if (!dx::compiler::saveScriptPackage(package, contents, output, &error)) {
            std::printf("Failed to save %s: %s\n", output.string().c_str(), error.c_str());
            return false;
        }
        std::printf("Saved %s\n", output.string().c_str());
        dx::objects::setPackageFile(dx::Name(name), output);
        if (resident) {
            return true;
        }
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
        for (dx::UObject* object : std::vector<dx::UObject*>(dx::objects::all())) {
            dx::UClass* cls = dx::cast<dx::UClass>(object);
            if (cls && !cls->destroyed && cls->superField && cls->superField->destroyed) {
                cls->superField = dx::objects::findClass(cls->superField->pathName());
            }
        }
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
    if (command == "exportsources") {
        return exportSources(args);
    }
    if (command == "make") {
        return make(root);
    }
    usage();
    return 1;
}
