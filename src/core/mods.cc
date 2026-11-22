#include "core/mods.h"

#include "core/config.h"
#include "core/object.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"
#include "engine/reader.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace dx::mods {
    namespace {
        const char* const folders[] = { "System", "Textures", "Sounds", "Music", "Maps" };
        const char* const extensions[] = { ".u", ".utx", ".uax", ".umx", ".dx", ".unr" };

        std::string lower(std::string_view text)
        {
            std::string result(text);
            for (char& c : result) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return result;
        }

        std::string displayName(const std::filesystem::path& owner)
        {
            return owner.empty() ? std::string("the game") : owner.filename().string();
        }

        bool& changedFlag()
        {
            static bool changed = false;
            return changed;
        }

        std::filesystem::path selectionFile()
        {
            return config::baseUserDirectory() / "Mods.ini";
        }

        struct Native {
            int32_t index = 0;
            std::string className;
            std::string function;
        };

        struct Summary {
            std::vector<std::array<std::string, 3>> imports;
            std::set<std::string> exports;
            std::vector<Native> natives;
        };

        std::vector<uint8_t> readRange(std::ifstream& file, uint64_t offset, uint64_t length, uint64_t size)
        {
            if (offset >= size) {
                return {};
            }
            std::vector<uint8_t> bytes(static_cast<size_t>(std::min(length, size - offset)));
            file.clear();
            file.seekg(static_cast<std::streamoff>(offset));
            file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return bytes;
        }

        uint32_t littleEndian(const std::vector<uint8_t>& bytes, size_t at, size_t count)
        {
            uint32_t value = 0;
            for (size_t i = 0; i < count; i++) {
                value |= static_cast<uint32_t>(bytes[at + i]) << (8 * i);
            }
            return value;
        }

        Summary readSummary(const std::filesystem::path& path)
        {
            Summary summary;
            std::error_code error;
            uint64_t size = std::filesystem::file_size(path, error);
            std::ifstream file(path, std::ios::binary);
            if (error || !file || size < 36) {
                return summary;
            }
            std::vector<uint8_t> header = readRange(file, 0, 36, size);
            Reader head(header, 0);
            if (head.u32() != 0x9E2A83C1) {
                return summary;
            }
            uint16_t version = static_cast<uint16_t>(head.i16());
            head.i16();
            head.u32();
            uint32_t nameCount = head.u32();
            uint32_t nameOffset = head.u32();
            uint32_t exportCount = head.u32();
            uint32_t exportOffset = head.u32();
            uint32_t importCount = head.u32();
            uint32_t importOffset = head.u32();

            std::vector<uint8_t> nameBytes = readRange(file, nameOffset, static_cast<uint64_t>(nameCount) * 300, size);
            Reader nameReader(nameBytes, 0);
            std::vector<std::string> names;
            for (uint32_t i = 0; i < nameCount; i++) {
                std::string name;
                if (version < 64) {
                    for (uint8_t c = nameReader.u8(); c != 0; c = nameReader.u8()) {
                        name.push_back(static_cast<char>(c));
                    }
                } else {
                    name = nameReader.string();
                }
                nameReader.u32();
                names.push_back(std::move(name));
            }

            std::vector<uint8_t> importBytes = readRange(file, importOffset, static_cast<uint64_t>(importCount) * 20, size);
            Reader importReader(importBytes, 0);
            std::vector<ImportEntry> imports(importCount);
            for (ImportEntry& entry : imports) {
                entry.classPackage = importReader.index();
                entry.className = importReader.index();
                entry.outer = importReader.i32();
                entry.name = importReader.index();
            }

            std::vector<uint8_t> exportBytes = readRange(file, exportOffset, static_cast<uint64_t>(exportCount) * 40, size);
            Reader exportReader(exportBytes, 0);
            std::vector<ExportEntry> exports(exportCount);
            for (ExportEntry& entry : exports) {
                entry.classRef = exportReader.index();
                entry.superRef = exportReader.index();
                entry.outer = exportReader.i32();
                entry.name = exportReader.index();
                entry.flags = exportReader.u32();
                entry.size = exportReader.index();
                if (entry.size > 0) {
                    entry.offset = exportReader.index();
                }
            }

            auto nameAt = [&](int32_t index) -> const std::string& {
                return names.at(static_cast<size_t>(index));
            };
            auto importAt = [&](int32_t ref) -> const ImportEntry& {
                return imports.at(static_cast<size_t>(-ref - 1));
            };
            auto classOf = [&](const ExportEntry& entry) -> std::string {
                if (entry.classRef == 0) {
                    return "Class";
                }
                if (entry.classRef < 0) {
                    return nameAt(importAt(entry.classRef).name);
                }
                return nameAt(exports.at(static_cast<size_t>(entry.classRef - 1)).name);
            };

            for (const ImportEntry& entry : imports) {
                const std::string& className = nameAt(entry.className);
                if (iequals(className, "Package") || entry.outer >= 0) {
                    continue;
                }
                int32_t outer = entry.outer;
                while (importAt(outer).outer < 0) {
                    outer = importAt(outer).outer;
                }
                if (importAt(outer).outer != 0) {
                    continue;
                }
                summary.imports.push_back({ nameAt(importAt(outer).name), className, nameAt(entry.name) });
            }

            for (const ExportEntry& entry : exports) {
                std::string className = classOf(entry);
                summary.exports.insert(lower(className) + "." + lower(nameAt(entry.name)));
                if (className != "Function" || entry.size < 9) {
                    continue;
                }
                std::vector<uint8_t> tail = readRange(file, static_cast<uint64_t>(entry.offset) + static_cast<uint64_t>(entry.size) - 9, 9, size);
                if (tail.size() < 9) {
                    continue;
                }
                uint32_t netFlags = littleEndian(tail, 3, 4);
                bool net = (netFlags & FUNC_Net) != 0;
                uint32_t flags = net ? netFlags : littleEndian(tail, 5, 4);
                int32_t index = static_cast<int32_t>(littleEndian(tail, net ? 0 : 2, 2));
                if (!(flags & FUNC_Native)) {
                    continue;
                }
                int32_t owner = entry.outer;
                while (owner > 0 && classOf(exports.at(static_cast<size_t>(owner - 1))) != "Class") {
                    owner = exports.at(static_cast<size_t>(owner - 1)).outer;
                }
                if (owner > 0) {
                    summary.natives.push_back({ index, nameAt(exports.at(static_cast<size_t>(owner - 1)).name), nameAt(entry.name) });
                }
            }
            return summary;
        }

        const Summary* summarize(const std::filesystem::path& path)
        {
            static std::map<std::filesystem::path, std::optional<Summary>> cache;
            auto it = cache.find(path);
            if (it == cache.end()) {
                std::optional<Summary> summary;
                try {
                    summary = readSummary(path);
                } catch (const std::exception&) {
                    summary.reset();
                }
                it = cache.emplace(path, std::move(summary)).first;
            }
            return it->second ? &*it->second : nullptr;
        }

        bool sameContents(const std::filesystem::path& a, const std::filesystem::path& b)
        {
            std::error_code error;
            if (std::filesystem::file_size(a, error) != std::filesystem::file_size(b, error)) {
                return false;
            }
            std::ifstream first(a, std::ios::binary);
            std::ifstream second(b, std::ios::binary);
            std::vector<char> left(1 << 16);
            std::vector<char> right(1 << 16);
            while (first && second) {
                first.read(left.data(), static_cast<std::streamsize>(left.size()));
                second.read(right.data(), static_cast<std::streamsize>(right.size()));
                if (first.gcount() != second.gcount() || !std::equal(left.begin(), left.begin() + first.gcount(), right.begin())) {
                    return false;
                }
            }
            return true;
        }

        struct Entry {
            std::filesystem::path file;
            std::filesystem::path owner;
        };

        struct Scan {
            std::map<std::string, Entry> winners;
            std::map<std::string, std::vector<Entry>> copies;
        };

        std::vector<std::string> pathsFor(const std::filesystem::path& gameRoot, const Layout& layout)
        {
            Ini ini;
            if (!ini.load(layout.ini) && !ini.load(gameRoot / "System" / "DeusEx.ini")) {
                ini.load(gameRoot / "System" / "Default.ini");
            }
            return ini.getAll("Core.System", "Paths");
        }

        Scan scan(const std::filesystem::path& gameRoot, const Layout& layout)
        {
            Scan result;
            for (const SearchDirectory& search : searchDirectories(gameRoot, layout, pathsFor(gameRoot, layout))) {
                std::error_code error;
                if (!std::filesystem::is_directory(search.directory, error)) {
                    continue;
                }
                for (const auto& item : std::filesystem::directory_iterator(search.directory)) {
                    std::string extension = lower(item.path().extension().string());
                    if (std::find(std::begin(extensions), std::end(extensions), extension) == std::end(extensions) || (!search.extension.empty() && search.extension != extension)) {
                        continue;
                    }
                    std::string stem = lower(item.path().stem().string());
                    Entry entry { item.path(), search.owner };
                    result.winners.emplace(stem, entry);
                    std::vector<Entry>& copies = result.copies[stem];
                    bool seen = std::any_of(copies.begin(), copies.end(), [&](const Entry& copy) {
                        return copy.owner == entry.owner;
                    });
                    if (!seen) {
                        copies.push_back(entry);
                    }
                }
            }
            return result;
        }

        std::string sample(const std::vector<std::string>& items)
        {
            std::string text;
            for (size_t i = 0; i < items.size() && i < 3; i++) {
                text += (i == 0 ? "" : ", ") + items[i];
            }
            if (items.size() > 3) {
                text += " and " + std::to_string(items.size() - 3) + " more";
            }
            return text;
        }

        std::vector<Issue> structuralIssues(const std::vector<std::filesystem::path>& mods, const Scan& scanned)
        {
            std::vector<Issue> issues;
            std::vector<std::filesystem::path> bases;
            for (const std::filesystem::path& mod : mods) {
                if (!findIni(mod).empty()) {
                    bases.push_back(mod);
                }
            }
            for (size_t i = 1; i < bases.size(); i++) {
                issues.push_back({ true, displayName(bases[0]) + " and " + displayName(bases[i]) + " each bring their own game setup, so only one of them can be enabled." });
            }
            std::map<std::pair<std::filesystem::path, std::filesystem::path>, std::pair<std::vector<std::string>, std::vector<std::string>>> clashes;
            for (const auto& [stem, copies] : scanned.copies) {
                std::vector<Entry> fromMods;
                for (const Entry& copy : copies) {
                    if (!copy.owner.empty()) {
                        fromMods.push_back(copy);
                    }
                }
                for (size_t i = 1; i < fromMods.size(); i++) {
                    if (sameContents(fromMods[0].file, fromMods[i].file)) {
                        continue;
                    }
                    auto& clash = clashes[{ fromMods[0].owner, fromMods[i].owner }];
                    bool script = lower(fromMods[i].file.extension().string()) == ".u";
                    (script ? clash.first : clash.second).push_back(fromMods[0].file.filename().string());
                }
            }
            for (const auto& [owners, clash] : clashes) {
                if (!clash.first.empty()) {
                    issues.push_back({ true, displayName(owners.first) + " and " + displayName(owners.second) + " both ship their own " + sample(clash.first) + ", so only one of them can be enabled." });
                }
                if (!clash.second.empty()) {
                    issues.push_back({ false, displayName(owners.first) + " replaces " + sample(clash.second) + " from " + displayName(owners.second) + "." });
                }
            }
            return issues;
        }

        std::vector<Issue> packageIssues(const Scan& scanned)
        {
            std::vector<Issue> issues;
            std::map<std::filesystem::path, std::vector<std::string>> unimplemented;
            for (const auto& [stem, entry] : scanned.winners) {
                if (entry.owner.empty() || lower(entry.file.extension().string()) != ".u") {
                    continue;
                }
                const Summary* summary = summarize(entry.file);
                if (!summary) {
                    issues.push_back({ false, displayName(entry.owner) + ": " + entry.file.filename().string() + " could not be read." });
                    continue;
                }
                std::set<std::string> missingPackages;
                std::map<std::string, std::vector<std::string>> missingObjects;
                for (const auto& [package, className, name] : summary->imports) {
                    std::string root = lower(package);
                    if (root == stem) {
                        continue;
                    }
                    auto provider = scanned.winners.find(root);
                    if (provider == scanned.winners.end()) {
                        missingPackages.insert(package);
                        continue;
                    }
                    const Summary* other = summarize(provider->second.file);
                    if (!other || other->exports.count(lower(className) + "." + lower(name))) {
                        continue;
                    }
                    if (iequals(className, "Class") && objects::isIntrinsicClassName(name)) {
                        continue;
                    }
                    missingObjects[root].push_back(className + " " + package + "." + name);
                }
                for (const std::string& package : missingPackages) {
                    issues.push_back({ true, displayName(entry.owner) + ": " + entry.file.filename().string() + " needs the package " + package + ", which is not installed." });
                }
                for (const auto& [root, objects] : missingObjects) {
                    const Entry& provider = scanned.winners.at(root);
                    issues.push_back({ true, displayName(entry.owner) + ": " + entry.file.filename().string() + " uses objects that " + provider.file.filename().string() + " from " + displayName(provider.owner) + " does not have: " + sample(objects) + "." });
                }
                for (const Native& native : summary->natives) {
                    if (!vm::hasNative(native.index, native.className, native.function)) {
                        unimplemented[entry.owner].push_back(native.className + "." + native.function);
                    }
                }
            }
            for (const auto& [owner, functions] : unimplemented) {
                issues.push_back({ false, displayName(owner) + ": native functions that are not implemented: " + sample(functions) + "." });
            }
            return issues;
        }
    }

    std::filesystem::path findRoot(const std::filesystem::path& gameRoot)
    {
        if (const char* value = std::getenv("DEUSEX_MODS")) {
            return std::filesystem::path(value);
        }
        std::error_code error;
        std::filesystem::path inside = gameRoot / "mods";
        if (std::filesystem::is_directory(inside, error)) {
            return inside;
        }
        std::filesystem::path beside = (gameRoot / "..").lexically_normal() / "mods";
        if (std::filesystem::is_directory(beside, error)) {
            return beside;
        }
        return inside;
    }

    std::vector<std::string> available(const std::filesystem::path& modsRoot)
    {
        std::vector<std::string> names;
        std::error_code error;
        for (const auto& item : std::filesystem::directory_iterator(modsRoot, error)) {
            std::string name = item.path().filename().string();
            if (item.is_directory() && !name.empty() && name[0] != '.') {
                names.push_back(name);
            }
        }
        std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) {
            return lower(a) < lower(b);
        });
        return names;
    }

    std::filesystem::path findIni(const std::filesystem::path& mod)
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

    Layout layout(const std::vector<std::filesystem::path>& mods)
    {
        Layout result;
        for (const std::filesystem::path& mod : mods) {
            std::filesystem::path ini = result.base.empty() ? findIni(mod) : std::filesystem::path();
            if (!ini.empty()) {
                result.base = mod;
                result.ini = ini;
            } else {
                result.addons.push_back(mod);
            }
        }
        return result;
    }

    std::vector<SearchDirectory> searchDirectories(const std::filesystem::path& gameRoot, const Layout& layout, const std::vector<std::string>& paths)
    {
        std::vector<SearchDirectory> directories;
        for (const std::filesystem::path& addon : layout.addons) {
            for (const char* folder : folders) {
                directories.push_back({ config::resolveCase(addon / folder), "", addon });
            }
        }
        std::vector<std::filesystem::path> roots;
        if (!layout.base.empty()) {
            roots.push_back(layout.base);
        }
        roots.push_back(gameRoot);
        for (std::string entry : paths) {
            std::replace(entry.begin(), entry.end(), '\\', '/');
            std::filesystem::path pattern(entry);
            for (const std::filesystem::path& root : roots) {
                std::filesystem::path owner = root == gameRoot ? std::filesystem::path() : root;
                directories.push_back({ config::resolveCase((root / "System" / pattern.parent_path()).lexically_normal()), lower(pattern.extension().string()), owner });
            }
        }
        if (paths.empty()) {
            for (const char* folder : folders) {
                for (const std::filesystem::path& root : roots) {
                    std::filesystem::path owner = root == gameRoot ? std::filesystem::path() : root;
                    directories.push_back({ root / folder, "", owner });
                }
            }
        }
        return directories;
    }

    std::vector<std::string> enabled()
    {
        Ini ini;
        ini.load(selectionFile());
        std::vector<std::string> names;
        for (const auto& [name, value] : ini.section("Mods")) {
            if (iequals(value, "True")) {
                names.push_back(name);
            }
        }
        std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) {
            return lower(a) < lower(b);
        });
        std::vector<std::string> ordered;
        for (const std::string& wanted : ini.getAll("Order", "Mod")) {
            auto it = std::find_if(names.begin(), names.end(), [&](const std::string& name) {
                return iequals(name, wanted);
            });
            if (it != names.end()) {
                ordered.push_back(*it);
                names.erase(it);
            }
        }
        ordered.insert(ordered.end(), names.begin(), names.end());
        return ordered;
    }

    bool isEnabled(std::string_view name)
    {
        Ini ini;
        ini.load(selectionFile());
        return iequals(ini.get("Mods", name).value_or("False"), "True");
    }

    void setEnabled(std::string_view name, bool enabled)
    {
        Ini ini;
        ini.load(selectionFile());
        ini.path = selectionFile();
        if (iequals(ini.get("Mods", name).value_or("False"), "True") != enabled) {
            changedFlag() = true;
        }
        ini.set("Mods", name, enabled ? "True" : "False");
        ini.save();
    }

    bool takeChanged()
    {
        bool changed = changedFlag();
        changedFlag() = false;
        return changed;
    }

    std::vector<Issue> check(const std::filesystem::path& gameRoot, const std::vector<std::filesystem::path>& mods)
    {
        std::vector<Issue> issues;
        std::vector<std::filesystem::path> present;
        for (const std::filesystem::path& mod : mods) {
            std::error_code error;
            if (std::filesystem::is_directory(mod, error)) {
                present.push_back(mod);
            } else {
                issues.push_back({ true, "The mod folder " + mod.string() + " was not found." });
            }
        }
        Scan scanned = scan(gameRoot, layout(present));
        std::vector<Issue> structural = structuralIssues(present, scanned);
        std::vector<Issue> packages = packageIssues(scanned);
        issues.insert(issues.end(), structural.begin(), structural.end());
        issues.insert(issues.end(), packages.begin(), packages.end());
        return issues;
    }

    std::string describe(const std::filesystem::path& gameRoot, const std::filesystem::path& modsRoot, const std::string& name, const std::vector<std::string>& others)
    {
        std::filesystem::path mod = modsRoot / name;
        std::filesystem::path ini = findIni(mod);
        std::string text = ini.empty() ? "Add-on, loaded on top of the game or of a full mod." : "Full mod with its own setup in " + ini.filename().string() + ".";
        for (const Issue& issue : check(gameRoot, { mod })) {
            text += " " + issue.text;
        }
        std::vector<std::string> conflicts;
        for (const std::string& other : others) {
            if (other == name) {
                continue;
            }
            std::vector<std::filesystem::path> pair { mod, modsRoot / other };
            std::vector<Issue> issues = structuralIssues(pair, scan(gameRoot, layout(pair)));
            if (std::any_of(issues.begin(), issues.end(), [](const Issue& issue) {
                    return issue.fatal;
                })) {
                conflicts.push_back(other);
            }
        }
        if (!conflicts.empty()) {
            std::string list;
            for (size_t i = 0; i < conflicts.size(); i++) {
                list += (i == 0 ? "" : ", ") + conflicts[i];
            }
            text += " Can't be combined with " + list + ".";
        }
        text += " Changes apply after restarting the game.";
        return text;
    }
}
