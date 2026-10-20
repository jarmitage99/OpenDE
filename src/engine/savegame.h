#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dx {
    class World;
    class Linker;
    class UObject;

    namespace savegame {
        constexpr int32_t QuickSaveIndex = -1;

        void setSaveRoot(const std::filesystem::path& root);
        std::filesystem::path saveRoot();
        std::filesystem::path currentDirectory();
        std::filesystem::path slotDirectory(int32_t index);
        std::vector<int32_t> slotIndices();
        int32_t newSlotIndex();
        uint64_t slotSize(int32_t index);
        std::string mapBaseName(const std::string& url);

        bool saveLevel(World& world, const std::filesystem::path& file, bool includePlayer, std::string* error);
        bool saveGame(World& world, int32_t index, const std::string& description);
        bool deleteGame(int32_t index);
        void deleteCurrent();
        UObject* readSaveInfo(int32_t index);
        bool loadRequested(const std::string& url, int32_t& index);
        bool prepareLoad(int32_t index, std::string& map);
        void storeDepartingLevel(World& world, const std::string& targetUrl);
        bool selectLevel(const std::string& url);
        Linker* takePendingLevel(const std::string& map);
        void levelRestored(World& world);
    }
}
