#include "compiler/asset_package.h"
#include "compiler/assets.h"
#include "compiler/con_file.h"
#include "compiler/script_package.h"
#include "core/objects.h"
#include "engine/package.h"

namespace dx::compiler {
    namespace {
        UObject* loadedPackage(const std::string& packageName)
        {
            Linker* linker = objects::loadPackage(Name(packageName));
            if (!linker) {
                return nullptr;
            }
            loadPackageExports(linker->packageObject);
            return linker->packageObject;
        }

        std::string quoted(const std::string& file)
        {
            return "CONVERSATION IMPORT FILE=\"" + file + "\"";
        }

        void discard(const std::string& packageName)
        {
            UObject* package = objects::packageObject(Name(packageName));
            for (UObject* object : packageContents(package)) {
                object->destroyed = true;
            }
        }
    }

    std::vector<std::string> packageConFiles(const std::string& packageName)
    {
        UObject* package = loadedPackage(packageName);
        return package ? conversationImportFiles(package) : std::vector<std::string>();
    }

    bool exportPackageConFile(const std::string& packageName, const std::string& file, const std::filesystem::path& directory, std::vector<std::string>& log)
    {
        UObject* package = loadedPackage(packageName);
        if (!package) {
            log.push_back("cannot load package " + packageName);
            return false;
        }
        ExecContext context;
        context.package = package;
        context.directory = directory;
        context.log = [&log](const std::string& line) {
            log.push_back(line);
        };
        return exportAssetSource(context, quoted(file));
    }

    int32_t compileConFile(const std::filesystem::path& directory, const std::string& file, const std::string& packageName, std::vector<std::string>& log)
    {
        ConFile source;
        std::string error;
        if (!loadConFile(directory / file, source, &error)) {
            log.push_back(error);
            return -1;
        }
        std::string prefix = packageName == "DeusExConversations" ? "DeusExCon" : packageName;
        discard(prefix + "Text");
        discard(prefix + "Audio" + source.audioPackage);
        UObject* package = objects::packageObject(Name(packageName));
        runAssetCommand(package, directory, quoted(file), log);
        UClass* conversationClass = objects::findClass("ConSys.Conversation");
        UObject* text = objects::packageObject(Name(prefix + "Text"));
        int32_t count = 0;
        for (UObject* object : packageContents(text)) {
            if (object->cls == conversationClass) {
                count++;
            }
        }
        return count;
    }
}
