#include "compiler/assets.h"

#include "compiler/exec_parse.h"
#include "core/objects.h"
#include "engine/package.h"
#include "engine/resources.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

namespace dx::compiler {
    namespace {
        std::string upper(std::string text)
        {
            for (char& c : text) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            return text;
        }

        std::filesystem::path sourcePath(const std::filesystem::path& directory, std::string file)
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

        UObject* findChild(UObject* outer, Name name, UClass* cls)
        {
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->outer == outer && object->name == name && object->cls == cls) {
                    return object;
                }
            }
            return nullptr;
        }

        struct SoundTarget {
            std::string file;
            UObject* outer = nullptr;
            Name name;
        };

        bool parseAudioImport(ExecContext& context, const std::string& command, SoundTarget& target, bool createGroups)
        {
            std::string_view text = command;
            if (!parseCommand(text, "AUDIO") || !parseCommand(text, "IMPORT")) {
                return false;
            }
            if (!parseString(text, "FILE=", target.file)) {
                context.log("Warning: Bad AUDIO IMPORT, missing FILE=");
                return true;
            }
            target.outer = context.package;
            std::string packageName;
            if (parseString(text, "PACKAGE=", packageName)) {
                target.outer = objects::packageObject(Name(packageName));
            }
            std::string group;
            if (parseString(text, "GROUP=", group) && !iequals(group, "None")) {
                UClass* packageClass = objects::intrinsicClass(Name("Package"));
                UObject* groupObject = findChild(target.outer, Name(group), packageClass);
                if (!groupObject && createGroups) {
                    groupObject = objects::newObject(packageClass, target.outer, Name(group), RF_Public);
                }
                target.outer = groupObject;
            }
            std::string name;
            if (!parseString(text, "NAME=", name)) {
                std::string file = target.file;
                std::replace(file.begin(), file.end(), '\\', '/');
                name = std::filesystem::path(file).stem().string();
            }
            target.name = Name(name);
            return true;
        }

        bool importAudio(ExecContext& context, const std::string& command)
        {
            SoundTarget target;
            if (!parseAudioImport(context, command, target, true)) {
                return false;
            }
            if (target.file.empty()) {
                return true;
            }
            std::filesystem::path path = sourcePath(context.directory, target.file);
            std::string extension = upper(path.extension().string());
            if (!extension.empty()) {
                extension.erase(0, 1);
            }
            if (extension == "UFX") {
                context.log("Warning: Invalid old-format sound " + target.name.str());
                return true;
            }
            if (extension != "WAV" && extension != "MP3") {
                context.log("Warning: Unrecognized sound format '" + extension + "' in " + target.name.str());
                return true;
            }
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                context.log("Warning: Can't find file '" + target.file + "' for import");
                return true;
            }
            UClass* soundClass = objects::intrinsicClass(Name("Sound"));
            UObject* object = findChild(target.outer, target.name, soundClass);
            if (!object) {
                object = objects::newObject(soundClass, target.outer, target.name, RF_Public | RF_Standalone);
            }
            USound* sound = cast<USound>(object);
            sound->format = Name(extension);
            sound->bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            return true;
        }

        bool writeBytes(const std::filesystem::path& file, const std::vector<uint8_t>& bytes)
        {
            if (file.has_parent_path()) {
                std::filesystem::create_directories(file.parent_path());
            }
            std::ofstream out(file, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return static_cast<bool>(out);
        }

        bool exportAudioSource(ExecContext& context, const std::string& command)
        {
            SoundTarget target;
            if (!parseAudioImport(context, command, target, false)) {
                return false;
            }
            USound* sound = target.outer ? cast<USound>(findChild(target.outer, target.name, objects::intrinsicClass(Name("Sound")))) : nullptr;
            if (!sound) {
                context.log("Warning: AUDIO IMPORT source: no sound " + target.name.str());
                return false;
            }
            objects::ensureLoaded(sound);
            std::string file = target.file;
            std::replace(file.begin(), file.end(), '\\', '/');
            return writeBytes(context.directory / file, sound->bytes);
        }

        bool exportAudioObject(UObject* object, const std::string& extension, const std::filesystem::path& file)
        {
            objects::ensureLoaded(object);
            if (USound* sound = cast<USound>(object)) {
                if (!iequals(extension, "wav")) {
                    return false;
                }
                return writeBytes(file, sound->bytes);
            }
            if (UMusic* music = cast<UMusic>(object)) {
                if (!iequals(extension, music->format.str())) {
                    return false;
                }
                return writeBytes(file, music->bytes);
            }
            return false;
        }

        void writeLazyBytes(PackageWriter& package, Writer& out, const std::vector<uint8_t>& bytes)
        {
            size_t skip = package.beginLazyArray(out);
            out.index(static_cast<int32_t>(bytes.size()));
            out.bytes(bytes.data(), bytes.size());
            package.endLazyArray(out, skip);
        }

        bool serializeAudio(PackageWriter& package, UObject* object, Writer& out)
        {
            if (USound* sound = cast<USound>(object)) {
                package.writeName(out, sound->format);
                writeLazyBytes(package, out, sound->bytes);
                return true;
            }
            if (UMusic* music = cast<UMusic>(object)) {
                package.writeName(out, music->format);
                writeLazyBytes(package, out, music->bytes);
                return true;
            }
            return false;
        }
    }

    AssetHandler audioAssets()
    {
        AssetHandler handler;
        handler.import = importAudio;
        handler.exportSource = exportAudioSource;
        handler.exportObject = exportAudioObject;
        handler.serialize = serializeAudio;
        return handler;
    }
}
