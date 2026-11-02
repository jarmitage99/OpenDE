#include "render/texture_replacement.h"

#include "core/config.h"
#include "core/objects.h"
#include "render/procedural.h"

#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>

namespace dx {
    namespace {
        std::string lowered(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return text;
        }

        std::filesystem::path childNamed(const std::filesystem::path& directory, const std::string& name)
        {
            std::error_code error;
            if (!std::filesystem::is_directory(directory, error)) {
                return {};
            }
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, error)) {
                if (lowered(entry.path().filename().string()) == name && entry.is_directory(error)) {
                    return entry.path();
                }
            }
            return {};
        }
    }

    bool decodePng(const std::vector<uint8_t>& bytes, ReplacementImage& image)
    {
        if (bytes.empty() || bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return false;
        }
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
        if (!pixels) {
            return false;
        }
        image.width = static_cast<uint32_t>(width);
        image.height = static_cast<uint32_t>(height);
        image.rgba.assign(pixels, pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        stbi_image_free(pixels);
        return true;
    }

    TextureReplacements::TextureReplacements()
        : defaultRoots(true)
    {
    }

    TextureReplacements::TextureReplacements(std::vector<std::filesystem::path> searchRoots)
        : roots(std::move(searchRoots))
    {
    }

    bool TextureReplacements::replaceable(UTexture* texture)
    {
        if (!texture || ProceduralTextures::isProcedural(texture)) {
            return false;
        }
        for (UObject* outer = texture->outer; outer; outer = outer->outer) {
            if (cast<UFont>(outer)) {
                return false;
            }
        }
        return true;
    }

    void TextureReplacements::rescan()
    {
        scanned = false;
    }

    void TextureReplacements::scan()
    {
        scanned = true;
        if (defaultRoots) {
            roots = { config::userDirectory(), objects::gameRoot() };
        }
        indexes.assign(roots.size(), {});
        empty = true;
        for (size_t i = 0; i < roots.size(); i++) {
            std::filesystem::path directory = childNamed(childNamed(roots[i], "textures"), "hd");
            if (directory.empty()) {
                continue;
            }
            std::error_code error;
            for (std::filesystem::recursive_directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
                const std::filesystem::path& file = it->path();
                if (lowered(file.extension().string()) != ".png" || !it->is_regular_file(error)) {
                    continue;
                }
                std::filesystem::path relative = file.lexically_relative(directory);
                relative.replace_extension();
                indexes[i].emplace(lowered(relative.generic_string()), file);
                empty = false;
            }
        }
    }

    std::filesystem::path TextureReplacements::find(UTexture* texture)
    {
        if (!scanned) {
            scan();
        }
        if (empty || !replaceable(texture)) {
            return {};
        }
        std::vector<std::string> chain;
        for (UObject* object = texture; object; object = object->outer) {
            chain.push_back(object->name.str());
        }
        std::reverse(chain.begin(), chain.end());
        std::string grouped;
        for (const std::string& part : chain) {
            grouped += grouped.empty() ? part : "/" + part;
        }
        std::vector<std::string> keys = { lowered(grouped) };
        if (chain.size() > 2) {
            keys.push_back(lowered(chain.front() + "/" + chain.back()));
        }
        for (const auto& index : indexes) {
            for (const std::string& key : keys) {
                auto it = index.find(key);
                if (it != index.end()) {
                    return it->second;
                }
            }
        }
        return {};
    }

    bool TextureReplacements::load(UTexture* texture, ReplacementImage& image)
    {
        std::filesystem::path path = find(texture);
        if (path.empty()) {
            return false;
        }
        std::ifstream file(path, std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return decodePng(bytes, image);
    }
}
