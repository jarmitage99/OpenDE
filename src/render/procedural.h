#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dx {
    struct ProceduralState;

    class ProceduralTextures {
    public:
        ProceduralTextures();
        ~ProceduralTextures();
        ProceduralTextures(const ProceduralTextures&) = delete;
        ProceduralTextures& operator=(const ProceduralTextures&) = delete;

        static bool isProcedural(UTexture* texture);
        bool update(UTexture* texture, double time, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height);
        void clear();

    private:
        std::unordered_map<UTexture*, std::unique_ptr<ProceduralState>> states;
    };
}
