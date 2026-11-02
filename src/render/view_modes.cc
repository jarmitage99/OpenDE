#include "render/view_modes.h"

#include <algorithm>

namespace dx {
    namespace {
        const char* modeNames[RenderModeCount] = { "Textured", "Wireframe", "Zone/Portal", "BSP Cuts", "Lighting Only", "Unlit" };
        const char* modeKeys[RenderModeCount] = { "textured", "wireframe", "zones", "bspcuts", "lighting", "unlit" };
    }

    const char* renderModeName(RenderMode mode)
    {
        return modeNames[static_cast<int>(mode)];
    }

    bool renderModeNamed(const std::string& name, RenderMode& mode)
    {
        for (int i = 0; i < RenderModeCount; i++) {
            if (name == modeKeys[i]) {
                mode = static_cast<RenderMode>(i);
                return true;
            }
        }
        return false;
    }

    Vector hsvColor(uint8_t hue, uint8_t saturation, uint8_t value)
    {
        float brightness = static_cast<float>(value) * 1.4f / 255.0f;
        brightness *= 0.7f / (0.01f + std::sqrt(brightness));
        brightness = std::clamp(brightness, 0.0f, 1.0f);
        float h = static_cast<float>(hue);
        Vector base;
        if (hue < 86) {
            base = { (85.0f - h) / 85.0f, h / 85.0f, 0.0f };
        } else if (hue < 171) {
            base = { 0.0f, (170.0f - h) / 85.0f, (h - 85.0f) / 85.0f };
        } else {
            base = { (h - 170.0f) / 85.0f, 0.0f, (255.0f - h) / 84.0f };
        }
        float s = static_cast<float>(saturation) / 255.0f;
        return (base + (Vector { 1, 1, 1 } - base) * s) * brightness;
    }

    Vector zoneColor(uint8_t zone, int32_t node, const Vector& textureColor)
    {
        Vector color = textureColor;
        if (zone != 0) {
            int32_t index = zone;
            color = Vector { static_cast<float>((index * 67) & 255), static_cast<float>((index * 1371) & 255), static_cast<float>((index * 1991) & 255) } / 256.0f;
        }
        return color * (0.5f + static_cast<float>(node & 7) / 16.0f);
    }

    Vector bspCutColor(int32_t node)
    {
        return hsvColor(static_cast<uint8_t>((node * 67) & 255), 0, 255);
    }
}
