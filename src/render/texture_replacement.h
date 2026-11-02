#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx {
    struct ReplacementImage {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> rgba;
    };

    bool decodePng(const std::vector<uint8_t>& bytes, ReplacementImage& image);

    class TextureReplacements {
    public:
        TextureReplacements();
        explicit TextureReplacements(std::vector<std::filesystem::path> searchRoots);

        static bool replaceable(UTexture* texture);
        std::filesystem::path find(UTexture* texture);
        bool load(UTexture* texture, ReplacementImage& image);
        void rescan();

    private:
        void scan();

        std::vector<std::filesystem::path> roots;
        bool defaultRoots = false;
        bool scanned = false;
        bool empty = true;
        std::vector<std::unordered_map<std::string, std::filesystem::path>> indexes;
    };
}
