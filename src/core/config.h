#pragma once

#include "core/object.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dx {
    class Ini {
    public:
        bool load(const std::filesystem::path& path);
        bool save() const;
        std::optional<std::string> get(std::string_view section, std::string_view key) const;
        std::vector<std::string> getAll(std::string_view section, std::string_view key) const;
        void set(std::string_view section, std::string_view key, std::string_view value);
        std::vector<std::pair<std::string, std::string>> section(std::string_view section) const;

        std::filesystem::path path;

    private:
        struct Section {
            std::string name;
            std::vector<std::pair<std::string, std::string>> entries;
        };

        const Section* find(std::string_view name) const;
        std::vector<Section> sections;
    };

    namespace config {
        std::filesystem::path findGameRoot(const std::filesystem::path& fallback);
        std::filesystem::path userDirectory();
        void initialize(const std::filesystem::path& systemDirectory);
        Ini& system();
        Ini& user();
        Ini* forName(Name configName);
        Ini* localization(std::string_view package);
        std::string localize(std::string_view section, std::string_view key, std::string_view package);
        void applyClassDefaults(UClass* cls);
        void setApplyClassDefaults(bool enabled);
        void applyObjectConfig(UObject* object);
        void saveObjectConfig(UObject* object, UClass* cls);
        Name configNameOf(const UClass* cls);
    }
}
