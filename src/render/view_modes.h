#pragma once

#include "core/math.h"

#include <cstdint>
#include <string>

namespace dx {
    enum class RenderMode : uint8_t {
        Textured,
        Wireframe,
        ZonePortal,
        BspCuts,
        LightingOnly,
        Unlit,
    };

    constexpr int RenderModeCount = 6;

    const char* renderModeName(RenderMode mode);
    bool renderModeNamed(const std::string& name, RenderMode& mode);
    Vector hsvColor(uint8_t hue, uint8_t saturation, uint8_t value);
    Vector zoneColor(uint8_t zone, int32_t node, const Vector& textureColor);
    Vector bspCutColor(int32_t node);
}
