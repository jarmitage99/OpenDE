#pragma once

#include "engine/resources.h"
#include "render/procedural.h"

#include <cstdint>
#include <unordered_map>

namespace dx {
    struct GLTextureInfo {
        uint32_t id = 0;
        uint32_t width = 1;
        uint32_t height = 1;
        bool masked = false;
        bool procedural = false;
        bool smooth = false;
        int32_t levels = 1;
    };

    struct TextureAnimation {
        UTexture* current = nullptr;
        double lastUpdate = 0;
        float accumulator = 0;
        int32_t primeCurrent = 0;
    };

    class TextureCache {
    public:
        static UTexture* advance(UTexture* head, TextureAnimation& animation, double now);

        TextureCache();
        ~TextureCache();
        TextureCache(const TextureCache&) = delete;
        TextureCache& operator=(const TextureCache&) = delete;

        void beginFrame(double time);
        UTexture* animated(UTexture* texture);
        const GLTextureInfo* get(UTexture* texture);
        const GLTextureInfo* getMasked(UTexture* texture);
        uint32_t white();
        void clear();
        void setFiltering(float newAnisotropy, bool newTrilinear);

    private:
        bool upload(UTexture* texture, GLTextureInfo& info, bool masked);

        std::unordered_map<UTexture*, GLTextureInfo> textures;
        std::unordered_map<UTexture*, GLTextureInfo> maskedTextures;
        std::unordered_map<UTexture*, TextureAnimation> animations;
        std::unordered_map<UTexture*, uint64_t> proceduralFrames;
        ProceduralTextures procedural;
        uint32_t whiteTexture = 0;
        double time = 0;
        uint64_t frame = 0;
        float anisotropy = 1;
        float maxAnisotropy = 1;
        bool trilinear = false;
    };
}
