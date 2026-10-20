#include "engine/canvas.h"
#include "engine/resources.h"

namespace dx {
    float textWidth(UFont* font, std::string_view text, float spacing)
    {
        if (!font) {
            return 0;
        }
        float width = 0;
        for (char c : text) {
            const FontCharacter* glyph = font->glyph(static_cast<uint8_t>(c), nullptr);
            if (glyph) {
                width += static_cast<float>(glyph->width) + spacing;
            }
        }
        return width;
    }

    float textHeight(UFont* font)
    {
        if (!font) {
            return 0;
        }
        const FontCharacter* glyph = font->glyph(static_cast<uint8_t>('A'), nullptr);
        if (!glyph) {
            glyph = font->glyph(static_cast<uint8_t>(' '), nullptr);
        }
        return glyph ? static_cast<float>(glyph->height) : 0;
    }

    void drawText(Canvas2D& canvas, UFont* font, std::string_view text, float x, float y, LinearColor color, BlendStyle style, float spacing)
    {
        if (!font) {
            return;
        }
        float cursor = x;
        for (char c : text) {
            UTexture* texture = nullptr;
            const FontCharacter* glyph = font->glyph(static_cast<uint8_t>(c), &texture);
            if (!glyph) {
                continue;
            }
            if (texture && glyph->width > 0 && glyph->height > 0) {
                canvas.drawTile(texture, cursor, y, static_cast<float>(glyph->width), static_cast<float>(glyph->height), static_cast<float>(glyph->u), static_cast<float>(glyph->v), static_cast<float>(glyph->width), static_cast<float>(glyph->height), color, style == BlendStyle::Normal ? BlendStyle::Masked : style, false);
            }
            cursor += static_cast<float>(glyph->width) + spacing;
        }
    }
}
