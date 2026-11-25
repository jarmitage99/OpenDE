#pragma once

#include "engine/canvas.h"
#include "render/gl_shader.h"
#include "render/texture_cache.h"

#include <array>
#include <optional>
#include <vector>

namespace dx {
    class GLCanvas : public Canvas2D {
    public:
        GLCanvas(UberShader& shader, TextureCache& textures);
        ~GLCanvas() override;

        void begin(int width, int height);
        void flush();

        float width() const override;
        float height() const override;
        void setClip(float x, float y, float w, float h) override;
        void clearClip() override;
        void drawTile(UTexture* texture, float x, float y, float w, float h, float u, float v, float ul, float vl, LinearColor color, BlendStyle style, bool smooth) override;
        void drawRect(float x, float y, float w, float h, LinearColor color, BlendStyle style) override;
        void drawLine(float x1, float y1, float x2, float y2, LinearColor color) override;

    private:
        using ClipRect = std::array<int, 4>;

        struct DrawState {
            uint32_t texture;
            BlendStyle style;
            bool masked;
            bool smooth;
            uint32_t primitive;
            std::optional<ClipRect> clip;

            bool operator==(const DrawState&) const = default;
        };

        struct DrawBatch {
            DrawState state;
            int32_t first;
        };

        void prepare(uint32_t texture, BlendStyle style, bool masked, bool smooth, uint32_t primitive);
        void drawBatch(const DrawBatch& batch, int32_t count);

        UberShader& shader;
        TextureCache& textures;
        int viewWidth = 1;
        int viewHeight = 1;
        std::optional<ClipRect> clipRect;
        std::vector<GLVertex> vertices;
        std::vector<DrawBatch> batches;
        uint32_t nearestSampler = 0;
    };
}
