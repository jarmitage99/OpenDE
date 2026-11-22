#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace dx::mods {
    struct Layout {
        std::vector<std::filesystem::path> addons;
        std::filesystem::path base;
        std::filesystem::path ini;
    };

    struct SearchDirectory {
        std::filesystem::path directory;
        std::string extension;
        std::filesystem::path owner;
    };

    struct Issue {
        bool fatal = false;
        std::string text;
    };

    std::filesystem::path findRoot(const std::filesystem::path& gameRoot);
    std::vector<std::string> available(const std::filesystem::path& modsRoot);
    std::filesystem::path findIni(const std::filesystem::path& mod);
    Layout layout(const std::vector<std::filesystem::path>& mods);
    std::vector<SearchDirectory> searchDirectories(const std::filesystem::path& gameRoot, const Layout& layout, const std::vector<std::string>& paths);
    std::vector<std::string> enabled();
    bool isEnabled(std::string_view name);
    void setEnabled(std::string_view name, bool enabled);
    bool takeChanged();
    std::vector<Issue> check(const std::filesystem::path& gameRoot, const std::vector<std::filesystem::path>& mods);
    std::string describe(const std::filesystem::path& gameRoot, const std::filesystem::path& modsRoot, const std::string& name, const std::vector<std::string>& others);
}
