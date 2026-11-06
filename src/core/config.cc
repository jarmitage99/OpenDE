#include "core/config.h"

#include "core/objects.h"
#include "engine/package.h"

#include <SDL_filesystem.h>
#include <SDL_stdinc.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace dx {
    namespace {
        std::string trim(std::string_view text)
        {
            size_t begin = 0;
            size_t end = text.size();
            while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r' || text[begin] == '\n')) {
                begin++;
            }
            while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r' || text[end - 1] == '\n')) {
                end--;
            }
            return std::string(text.substr(begin, end - begin));
        }

        std::string lower(std::string_view text)
        {
            std::string result(text);
            for (char& c : result) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return result;
        }

        struct ConfigState {
            std::filesystem::path systemDirectory;
            std::filesystem::path modDirectory;
            Ini system;
            Ini user;
            std::unordered_map<std::string, std::unique_ptr<Ini>> others;
            std::unordered_map<std::string, std::unique_ptr<Ini>> localizations;
        };

        ConfigState& state()
        {
            static ConfigState instance;
            return instance;
        }

        std::string packageOf(const UObject* object)
        {
            const UObject* current = object;
            while (current->outer) {
                current = current->outer;
            }
            return current->name.str();
        }

        std::string configSection(const UProperty* property, const UClass* cls)
        {
            const UClass* owner = cast<UClass>(property->outer);
            if ((property->propertyFlags & CPF_GlobalConfig) && owner) {
                cls = owner;
            }
            return packageOf(cls) + "." + cls->name.str();
        }

        void applyProperty(UProperty* property, uint8_t* data, Ini& ini, const std::string& section)
        {
            if (property->arrayDim > 1) {
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    auto value = ini.get(section, property->name.str() + "[" + std::to_string(i) + "]");
                    if (value) {
                        property->fromText(data + property->offset + static_cast<size_t>(i) * property->elementSize(), *value);
                    }
                }
                return;
            }
            auto value = ini.get(section, property->name.str());
            if (value) {
                property->fromText(data + property->offset, *value);
            }
        }

        void loadConfigPropagated(UObject* object, UClass* cls, Ini& ini)
        {
            if (!(cls->classFlags & CLASS_Config)) {
                return;
            }
            if (cls->superClass()) {
                loadConfigPropagated(object, cls->superClass(), ini);
            }
            for (UProperty* property : cls->properties) {
                if (property->propertyFlags & CPF_Config) {
                    applyProperty(property, object->data, ini, configSection(property, cls));
                }
            }
        }
    }

    bool Ini::load(const std::filesystem::path& filePath)
    {
        path = filePath;
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.size() >= 2 && static_cast<uint8_t>(content[0]) == 0xff && static_cast<uint8_t>(content[1]) == 0xfe) {
            std::string narrow;
            for (size_t i = 2; i + 1 < content.size(); i += 2) {
                uint16_t c = static_cast<uint16_t>(static_cast<uint8_t>(content[i]) | (static_cast<uint8_t>(content[i + 1]) << 8));
                narrow.push_back(c < 256 ? static_cast<char>(c) : '?');
            }
            content = std::move(narrow);
        }
        Section* current = nullptr;
        size_t pos = 0;
        while (pos < content.size()) {
            size_t end = content.find('\n', pos);
            if (end == std::string::npos) {
                end = content.size();
            }
            std::string line = trim(std::string_view(content).substr(pos, end - pos));
            pos = end + 1;
            if (line.empty() || line[0] == ';') {
                continue;
            }
            if (line.front() == '[' && line.back() == ']') {
                std::string sectionName = line.substr(1, line.size() - 2);
                current = nullptr;
                for (Section& existing : sections) {
                    if (iequals(existing.name, sectionName)) {
                        current = &existing;
                    }
                }
                if (!current) {
                    sections.push_back({ sectionName, {} });
                    current = &sections.back();
                }
                continue;
            }
            size_t equals = line.find('=');
            if (!current || equals == std::string::npos) {
                continue;
            }
            current->entries.emplace_back(trim(line.substr(0, equals)), trim(line.substr(equals + 1)));
        }
        return true;
    }

    bool Ini::save() const
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        for (const Section& section : sections) {
            file << "[" << section.name << "]\r\n";
            for (const auto& [key, value] : section.entries) {
                file << key << "=" << value << "\r\n";
            }
            file << "\r\n";
        }
        return true;
    }

    const Ini::Section* Ini::find(std::string_view name) const
    {
        for (const Section& section : sections) {
            if (iequals(section.name, name)) {
                return &section;
            }
        }
        return nullptr;
    }

    std::optional<std::string> Ini::get(std::string_view sectionName, std::string_view key) const
    {
        const Section* section = find(sectionName);
        if (!section) {
            return std::nullopt;
        }
        for (const auto& [entryKey, value] : section->entries) {
            if (iequals(entryKey, key)) {
                return value;
            }
        }
        return std::nullopt;
    }

    std::vector<std::string> Ini::getAll(std::string_view sectionName, std::string_view key) const
    {
        std::vector<std::string> values;
        const Section* section = find(sectionName);
        if (!section) {
            return values;
        }
        for (const auto& [entryKey, value] : section->entries) {
            if (iequals(entryKey, key)) {
                values.push_back(value);
            }
        }
        return values;
    }

    void Ini::set(std::string_view sectionName, std::string_view key, std::string_view value)
    {
        Section* section = nullptr;
        for (Section& existing : sections) {
            if (iequals(existing.name, sectionName)) {
                section = &existing;
            }
        }
        if (!section) {
            sections.push_back({ std::string(sectionName), {} });
            section = &sections.back();
        }
        for (auto& [entryKey, entryValue] : section->entries) {
            if (iequals(entryKey, key)) {
                entryValue = value;
                return;
            }
        }
        section->entries.emplace_back(std::string(key), std::string(value));
    }

    std::vector<std::pair<std::string, std::string>> Ini::section(std::string_view sectionName) const
    {
        const Section* found = find(sectionName);
        if (!found) {
            return {};
        }
        return found->entries;
    }

    namespace config {
        std::filesystem::path fromUtf8(char* text)
        {
            std::filesystem::path path(std::u8string(reinterpret_cast<const char8_t*>(text)));
            SDL_free(text);
            return path;
        }

        std::vector<std::filesystem::path> steamRoots()
        {
            std::vector<std::filesystem::path> roots;
#ifdef _WIN32
            for (const char* variable : { "ProgramFiles(x86)", "ProgramFiles" }) {
                if (const char* value = std::getenv(variable)) {
                    roots.push_back(std::filesystem::path(value) / "Steam");
                }
            }
#else
            if (const char* home = std::getenv("HOME")) {
                std::filesystem::path base(home);
                roots.push_back(base / "Library" / "Application Support" / "Steam");
                roots.push_back(base / ".steam" / "steam");
                roots.push_back(base / ".local" / "share" / "Steam");
                roots.push_back(base / ".var" / "app" / "com.valvesoftware.Steam" / ".local" / "share" / "Steam");
            }
#endif
            return roots;
        }

        std::vector<std::filesystem::path> steamLibraries(const std::filesystem::path& root)
        {
            std::vector<std::filesystem::path> libraries { root };
            std::ifstream file(root / "steamapps" / "libraryfolders.vdf");
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream stream(line);
                std::string key;
                std::string value;
                if (stream >> std::quoted(key) >> std::quoted(value) && key == "path") {
                    libraries.emplace_back(value);
                }
            }
            return libraries;
        }

        std::filesystem::path findGameRoot(const std::filesystem::path& fallback)
        {
            std::vector<std::filesystem::path> candidates;
            if (const char* value = std::getenv("DEUSEX_GAME_DATA")) {
                candidates.emplace_back(value);
            }
            if (const char* value = std::getenv("APPIMAGE")) {
                candidates.push_back(std::filesystem::path(value).parent_path() / "game-data");
            }
            if (char* base = SDL_GetBasePath()) {
                candidates.push_back(fromUtf8(base) / "game-data");
            }
            for (const std::filesystem::path& root : steamRoots()) {
                for (const std::filesystem::path& library : steamLibraries(root)) {
                    candidates.push_back(library / "steamapps" / "common" / "Deus Ex");
                }
            }
            for (const std::filesystem::path& candidate : candidates) {
                std::error_code error;
                if (std::filesystem::is_regular_file(candidate / "System" / "DeusEx.u", error)) {
                    return candidate;
                }
            }
            return fallback;
        }

        std::filesystem::path userDirectory()
        {
            const char* xdg = std::getenv("XDG_DATA_HOME");
#ifdef _WIN32
            if (!xdg) {
                if (char* pref = SDL_GetPrefPath("", "deusex")) {
                    return fromUtf8(pref);
                }
            }
#endif
            const char* home = std::getenv("HOME");
            std::filesystem::path base = xdg ? std::filesystem::path(xdg) : (home ? std::filesystem::path(home) / ".local" / "share" : std::filesystem::temp_directory_path());
            std::filesystem::path directory = base / "deusex";
            if (!state().modDirectory.empty()) {
                directory = directory / "mods" / state().modDirectory.filename();
            }
            std::error_code error;
            std::filesystem::create_directories(directory, error);
            return directory;
        }

        void loadPreferred(Ini& ini, const std::filesystem::path& userFile, const std::filesystem::path& gameFile, const std::filesystem::path& fallbackFile)
        {
            if (!ini.load(userFile) && !ini.load(gameFile)) {
                ini.load(fallbackFile);
            }
            ini.path = userFile;
        }

        namespace {
            std::filesystem::path findModIni(const std::filesystem::path& mod)
            {
                std::vector<std::filesystem::path> candidates;
                for (const auto& item : std::filesystem::recursive_directory_iterator(mod)) {
                    if (item.is_regular_file() && iequals(item.path().extension().string(), ".ini")) {
                        candidates.push_back(item.path());
                    }
                }
                std::sort(candidates.begin(), candidates.end());
                for (const std::filesystem::path& candidate : candidates) {
                    Ini ini;
                    if (ini.load(candidate) && !ini.getAll("Core.System", "Paths").empty()) {
                        return candidate;
                    }
                }
                return {};
            }

            bool endsWith(const std::string& text, std::string_view suffix)
            {
                return text.size() > suffix.size() && iequals(std::string_view(text).substr(text.size() - suffix.size()), suffix);
            }

            std::filesystem::path findModUserIni(const std::filesystem::path& systemIni)
            {
                std::string stem = systemIni.stem().string();
                std::string wanted = stem + "User.ini";
                if (endsWith(stem, "Default")) {
                    wanted = stem.substr(0, stem.size() - 7) + "DefUser.ini";
                }
                for (const auto& item : std::filesystem::directory_iterator(systemIni.parent_path())) {
                    if (iequals(item.path().filename().string(), wanted)) {
                        return item.path();
                    }
                }
                return {};
            }

            std::string userCopyName(const std::filesystem::path& file)
            {
                std::string stem = file.stem().string();
                if (endsWith(stem, "DefUser")) {
                    return stem.substr(0, stem.size() - 7) + "User.ini";
                }
                if (endsWith(stem, "Default")) {
                    return stem.substr(0, stem.size() - 7) + ".ini";
                }
                return file.filename().string();
            }

            bool loadLocalizationFile(Ini& ini, const std::filesystem::path& directory, const std::string& file)
            {
                if (ini.load(directory / file)) {
                    return true;
                }
                bool found = false;
                if (std::filesystem::is_directory(directory)) {
                    for (const auto& item : std::filesystem::directory_iterator(directory)) {
                        if (iequals(item.path().filename().string(), file)) {
                            found = ini.load(item.path()) || found;
                        }
                    }
                }
                return found;
            }
        }

        void useMod(const std::filesystem::path& directory)
        {
            state().modDirectory = directory;
        }

        std::filesystem::path modDirectory()
        {
            return state().modDirectory;
        }

        std::filesystem::path resolveCase(const std::filesystem::path& path)
        {
            std::filesystem::path result;
            for (const std::filesystem::path& part : path) {
                std::filesystem::path next = result / part;
                if (!result.empty() && !std::filesystem::exists(next) && std::filesystem::is_directory(result)) {
                    for (const auto& item : std::filesystem::directory_iterator(result)) {
                        if (iequals(item.path().filename().string(), part.string())) {
                            next = item.path();
                            break;
                        }
                    }
                }
                result = next;
            }
            return result;
        }

        std::filesystem::path findFile(std::string relativeToSystem)
        {
            std::replace(relativeToSystem.begin(), relativeToSystem.end(), '\\', '/');
            std::vector<std::filesystem::path> systems;
            if (!state().modDirectory.empty()) {
                systems.push_back(state().modDirectory / "System");
            }
            systems.push_back(state().systemDirectory);
            for (const std::filesystem::path& system : systems) {
                std::filesystem::path candidate = resolveCase((system / relativeToSystem).lexically_normal());
                if (std::filesystem::is_regular_file(candidate)) {
                    return candidate;
                }
            }
            return {};
        }

        void initialize(const std::filesystem::path& systemDirectory, const std::filesystem::path& systemIni, const std::filesystem::path& userIni)
        {
            ConfigState& s = state();
            s.systemDirectory = systemDirectory;
            std::filesystem::path userDir = userDirectory();
            std::filesystem::path systemFile = systemIni;
            std::filesystem::path userFile = userIni;
            if (!s.modDirectory.empty() && systemFile.empty()) {
                systemFile = findModIni(s.modDirectory);
            }
            if (!s.modDirectory.empty() && userFile.empty() && !systemFile.empty()) {
                userFile = findModUserIni(systemFile);
            }
            if (systemFile.empty()) {
                systemFile = systemDirectory / "DeusEx.ini";
            }
            if (userFile.empty()) {
                userFile = systemDirectory / "User.ini";
            }
            loadPreferred(s.system, userDir / userCopyName(systemFile), systemFile, systemDirectory / "Default.ini");
            loadPreferred(s.user, userDir / userCopyName(userFile), userFile, systemDirectory / "DefUser.ini");
        }

        Ini& system()
        {
            return state().system;
        }

        Ini& user()
        {
            return state().user;
        }

        Ini* forName(Name configName)
        {
            if (configName.none() || configName == Name("System")) {
                return &state().system;
            }
            if (configName == Name("User")) {
                return &state().user;
            }
            std::string key = lower(configName.str());
            auto it = state().others.find(key);
            if (it != state().others.end()) {
                return it->second.get();
            }
            auto ini = std::make_unique<Ini>();
            std::filesystem::path preferred = state().modDirectory.empty() ? state().systemDirectory : state().modDirectory / "System";
            loadPreferred(*ini, userDirectory() / (configName.str() + ".ini"), preferred / (configName.str() + ".ini"), state().systemDirectory / (configName.str() + ".ini"));
            Ini* result = ini.get();
            state().others.emplace(key, std::move(ini));
            return result;
        }

        Ini* localization(std::string_view package)
        {
            std::string key = lower(package);
            auto it = state().localizations.find(key);
            if (it != state().localizations.end()) {
                return it->second.get();
            }
            auto ini = std::make_unique<Ini>();
            std::vector<std::filesystem::path> directories;
            if (!state().modDirectory.empty()) {
                directories.push_back(state().modDirectory / "System");
            }
            directories.push_back(state().systemDirectory);
            std::string language = lower(state().system.get("Engine.Engine", "Language").value_or("int"));
            std::vector<std::string> extensions { language };
            if (language != "int") {
                extensions.push_back("int");
            }
            for (const std::string& extension : extensions) {
                for (const std::filesystem::path& directory : directories) {
                    if (loadLocalizationFile(*ini, directory, std::string(package) + "." + extension)) {
                        break;
                    }
                }
            }
            Ini* result = ini.get();
            state().localizations.emplace(key, std::move(ini));
            return result;
        }

        std::string localize(std::string_view section, std::string_view key, std::string_view package)
        {
            Ini* ini = localization(package);
            if (ini) {
                auto value = ini->get(section, key);
                if (value) {
                    std::string text = *value;
                    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
                        text = text.substr(1, text.size() - 2);
                    }
                    return text;
                }
            }
            return "<?int?" + std::string(package) + "." + std::string(section) + "." + std::string(key) + "?>";
        }

        Name configNameOf(const UClass* cls)
        {
            for (const UClass* current = cls; current; current = current->superClass()) {
                if (!current->configName.none()) {
                    return current->configName;
                }
            }
            return Name("System");
        }

        namespace {
            bool& classDefaultsEnabled()
            {
                static bool enabled = true;
                return enabled;
            }
        }

        void setApplyClassDefaults(bool enabled)
        {
            classDefaultsEnabled() = enabled;
        }

        void applyClassDefaults(UClass* cls)
        {
            if (!classDefaultsEnabled()) {
                return;
            }
            uint8_t* data = cls->defaultObject->data;
            std::string package = packageOf(cls);
            Ini* ini = forName(configNameOf(cls));
            for (UProperty* property : cls->properties) {
                if ((property->propertyFlags & CPF_Config) && ini) {
                    applyProperty(property, data, *ini, configSection(property, cls));
                }
                if (property->propertyFlags & CPF_Localized) {
                    Ini* loc = localization(package);
                    if (loc) {
                        applyProperty(property, data, *loc, cls->name.str());
                    }
                }
            }
        }

        void applyObjectConfig(UObject* object)
        {
            UClass* cls = object->cls;
            Ini* ini = forName(configNameOf(cls));
            if (!ini) {
                return;
            }
            for (UProperty* property : cls->properties) {
                if (property->propertyFlags & (CPF_Config | CPF_GlobalConfig)) {
                    applyProperty(property, object->data, *ini, configSection(property, cls));
                }
            }
        }

        void saveObjectConfig(UObject* object, UClass* cls)
        {
            Ini* ini = forName(configNameOf(cls));
            if (!ini) {
                return;
            }
            for (UProperty* property : cls->properties) {
                if (!(property->propertyFlags & (CPF_Config | CPF_GlobalConfig))) {
                    continue;
                }
                std::string section = configSection(property, cls);
                if (property->arrayDim > 1) {
                    for (int32_t i = 0; i < property->arrayDim; i++) {
                        ini->set(section, property->name.str() + "[" + std::to_string(i) + "]", property->toText(object->data + property->offset + static_cast<size_t>(i) * property->elementSize()));
                    }
                } else {
                    ini->set(section, property->name.str(), property->toText(object->data + property->offset));
                }
            }
            ini->save();
            UClass* top = object->cls;
            while (top->superClass() && (top->superClass()->classFlags & CLASS_Config)) {
                top = top->superClass();
            }
            for (UObject* candidate : objects::all()) {
                UClass* loaded = cast<UClass>(candidate);
                if (loaded && loaded->defaultObject && loaded->isChildOf(top)) {
                    Ini* defaults = forName(configNameOf(loaded));
                    if (defaults) {
                        loadConfigPropagated(loaded->defaultObject, loaded, *defaults);
                    }
                }
            }
        }
    }
}
