#include "compiler/assets.h"

#include "compiler/exec_parse.h"
#include "core/objects.h"
#include "engine/package.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <vector>

namespace dx::compiler {
    namespace {
        UClass* extStringClass()
        {
            return objects::findClass("Extension.ExtString");
        }

        std::string& textOf(UObject* object)
        {
            return *reinterpret_cast<std::string*>(object->propertyAddress(Name("Text")));
        }

        bool isExtString(UObject* object)
        {
            UClass* cls = extStringClass();
            return object && object->cls && cls && object->isA(cls);
        }

        std::vector<std::filesystem::path> sortedEntries(const std::filesystem::path& directory, bool directories)
        {
            std::vector<std::filesystem::path> entries;
            if (!std::filesystem::is_directory(directory)) {
                return entries;
            }
            for (const auto& item : std::filesystem::directory_iterator(directory)) {
                if (directories && item.is_directory()) {
                    entries.push_back(item.path());
                } else if (!directories && item.is_regular_file() && iequals(item.path().extension().string(), ".txt")) {
                    entries.push_back(item.path());
                }
            }
            std::sort(entries.begin(), entries.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
                std::string left = a.filename().string();
                std::string right = b.filename().string();
                return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(), [](char x, char y) {
                    return std::tolower(static_cast<unsigned char>(x)) < std::tolower(static_cast<unsigned char>(y));
                });
            });
            return entries;
        }

        std::filesystem::path resolve(const std::filesystem::path& directory, std::string file)
        {
            std::replace(file.begin(), file.end(), '\\', '/');
            std::filesystem::path result = directory;
            for (const std::filesystem::path& part : std::filesystem::path(file)) {
                std::filesystem::path exact = result / part;
                if (!std::filesystem::exists(exact) && std::filesystem::is_directory(result)) {
                    for (const auto& item : std::filesystem::directory_iterator(result)) {
                        if (iequals(item.path().filename().string(), part.string())) {
                            exact = item.path();
                            break;
                        }
                    }
                }
                result = exact;
            }
            return result;
        }

        UObject* findText(UObject* package, Name name)
        {
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->outer == package && object->name == name && isExtString(object)) {
                    return object;
                }
            }
            return nullptr;
        }

        void importFile(ExecContext& context, const std::filesystem::path& file)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in) {
                context.log("DeusExTextImport WARNING: Can't find file '" + file.string() + "' for import");
                return;
            }
            std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            text = text.substr(0, text.find('\0'));
            Name name(file.stem().string());
            UObject* object = findText(context.package, name);
            if (!object) {
                object = objects::newObject(extStringClass(), context.package, name, RF_Public | RF_Standalone);
            }
            textOf(object) = text;
        }

        std::string directoryFor(const std::string& name)
        {
            if (name.size() > 2 && std::isdigit(static_cast<unsigned char>(name[0])) && std::isdigit(static_cast<unsigned char>(name[1])) && name[2] == '_') {
                return "Mission" + name.substr(0, 2);
            }
            return "Misc";
        }

        bool writeText(UObject* object, const std::filesystem::path& file)
        {
            std::filesystem::create_directories(file.parent_path());
            std::ofstream out(file, std::ios::binary);
            const std::string& text = textOf(object);
            out.write(text.data(), static_cast<std::streamsize>(text.size()));
            return static_cast<bool>(out);
        }

        bool import(ExecContext& context, const std::string& command)
        {
            std::string_view text = command;
            if (parseCommand(text, "ALLDEUSEXTEXT")) {
                if (!parseCommand(text, "IMPORT")) {
                    return false;
                }
                for (const std::filesystem::path& directory : sortedEntries(context.directory / "Text", true)) {
                    for (const std::filesystem::path& file : sortedEntries(directory, false)) {
                        importFile(context, file);
                    }
                }
                return true;
            }
            if (!parseCommand(text, "DEUSEXTEXT") || !parseCommand(text, "IMPORT")) {
                return false;
            }
            std::string file;
            if (!parseString(text, "FILE=", file)) {
                context.log("Bad DEUSEXTEXT IMPORT");
                return true;
            }
            importFile(context, resolve(context.directory, file));
            return true;
        }

        bool exportSource(ExecContext& context, const std::string& command)
        {
            std::string_view text = command;
            if (parseCommand(text, "ALLDEUSEXTEXT")) {
                if (!parseCommand(text, "IMPORT")) {
                    return false;
                }
                bool success = true;
                for (UObject* object : objects::all()) {
                    if (object && !object->destroyed && object->outer == context.package && isExtString(object)) {
                        std::string name = object->name.str();
                        success = writeText(object, context.directory / "Text" / directoryFor(name) / (name + ".txt")) && success;
                    }
                }
                return success;
            }
            if (!parseCommand(text, "DEUSEXTEXT") || !parseCommand(text, "IMPORT")) {
                return false;
            }
            std::string file;
            if (!parseString(text, "FILE=", file)) {
                return false;
            }
            std::replace(file.begin(), file.end(), '\\', '/');
            UObject* object = findText(context.package, Name(std::filesystem::path(file).stem().string()));
            return object && writeText(object, context.directory / file);
        }

        bool exportObject(UObject* object, const std::string& extension, const std::filesystem::path& file)
        {
            return extension == "txt" && isExtString(object) && writeText(object, file);
        }

        bool serialize(PackageWriter&, UObject* object, Writer& out)
        {
            if (!isExtString(object)) {
                return false;
            }
            out.string(textOf(object));
            return true;
        }
    }

    AssetHandler textAssets()
    {
        AssetHandler handler;
        handler.import = import;
        handler.exportSource = exportSource;
        handler.exportObject = exportObject;
        handler.serialize = serialize;
        return handler;
    }
}
