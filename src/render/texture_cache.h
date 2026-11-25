#pragma once

#include "engine/resources.h"
#include "render/procedural.h"
#include "render/render_device.h"
#include "render/texture_replacement.h"

#include <cstdint>
#include <unordered_map>

namespace dx {
#ifdef __EMSCRIPTEN__
    class RipplePass;
#endif

    struct TextureAnimation {
        UTexture* current = nullptr;
        double lastUpdate = 0;
        float accumulator = 0;
        int32_t primeCurrent = 0;
    };

    class TextureCache {
    public:
        static UTexture* advance(UTexture* head, TextureAnimation& animation, double now, float constantRate = 0.0f);

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
        void resetProcedural(UTexture* texture);
        void setFiltering(float newAnisotropy, bool newTrilinear);
        void setConstantRate(float rate);

    private:
        struct ProceduralTexture {
#ifdef __EMSCRIPTEN__
            GLTextureInfo buffers[2];
            uint32_t front = 0;
            uint32_t heights[2] = {};
            uint32_t lookup = 0;
            uint32_t source = 0;
            void clearRippleInputs();
#else
            GLTextureInfo buffers[1];
            static constexpr uint32_t front = 0;
#endif
            uint64_t lastFrame = 0;
        };

        const GLTextureInfo* getProcedural(UTexture* texture, ProceduralTexture& entry);
#ifdef __EMSCRIPTEN__
        void renderRipple(ProceduralTexture& entry, uint32_t next, const RippleFrame& ripple, int32_t width, int32_t height);
        std::unique_ptr<RipplePass> ripplePass;
#endif
        bool upload(UTexture* texture, GLTextureInfo& info, bool masked);
        bool uploadReplacement(UTexture* texture, GLTextureInfo& info);

        std::unordered_map<UTexture*, GLTextureInfo> textures;
        std::unordered_map<UTexture*, GLTextureInfo> maskedTextures;
        std::unordered_map<UTexture*, TextureAnimation> animations;
        std::unordered_map<UTexture*, ProceduralTexture> proceduralTextures;
        ProceduralTextures procedural;
        std::vector<uint8_t> proceduralPixels;
        TextureReplacements replacements;
        uint32_t whiteTexture = 0;
        double time = 0;
        uint64_t frame = 0;
        float anisotropy = 1;
        float maxAnisotropy = 1;
        bool trilinear = false;
        float constantRate = 0;
    };
}
