#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dx {
    struct ProceduralState;

    struct RippleFrame {
        const uint8_t* heights = nullptr;
        const uint8_t* slopes = nullptr;
        const uint8_t* source = nullptr;
        const uint32_t* palette = nullptr;
        bool even = false;
    };

    class ProceduralTextures {
    public:
        ProceduralTextures();
        ~ProceduralTextures();
        ProceduralTextures(const ProceduralTextures&) = delete;
        ProceduralTextures& operator=(const ProceduralTextures&) = delete;

        static bool isProcedural(UTexture* texture);
        bool update(UTexture* texture, double time, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height, uint64_t frame = 0, RippleFrame* ripple = nullptr);
        void clear();
        void reset(UTexture* texture);
        void setConstantRate(float rate);

    private:
        ProceduralState& updateState(UTexture* texture, double time, uint64_t frame, bool asSource, bool gpuRipple = false);
        std::unordered_map<UTexture*, std::unique_ptr<ProceduralState>> states;
        uint32_t nextPhase = 0;
        float constantRate = 0;
    };
}
