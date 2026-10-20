#pragma once

#include "core/math.h"

#include <cstdint>
#include <string_view>

namespace dx {
    class UTexture;
    class UFont;

    enum class BlendStyle : uint8_t {
        None = 0,
        Normal = 1,
        Masked = 2,
        Translucent = 3,
        Modulated = 4,
    };

    struct LinearColor {
        float r = 1;
        float g = 1;
        float b = 1;
        float a = 1;
    };

    class Canvas2D {
    public:
        virtual ~Canvas2D() = default;
        virtual float width() const = 0;
        virtual float height() const = 0;
        virtual void setClip(float x, float y, float w, float h) = 0;
        virtual void clearClip() = 0;
        virtual void drawTile(UTexture* texture, float x, float y, float w, float h, float u, float v, float ul, float vl, LinearColor color, BlendStyle style, bool smooth) = 0;
        virtual void drawRect(float x, float y, float w, float h, LinearColor color, BlendStyle style) = 0;
        virtual void drawLine(float x1, float y1, float x2, float y2, LinearColor color) = 0;
    };

    float textWidth(UFont* font, std::string_view text, float spacing);
    float textHeight(UFont* font);
    void drawText(Canvas2D& canvas, UFont* font, std::string_view text, float x, float y, LinearColor color, BlendStyle style, float spacing);
}
