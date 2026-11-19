#include "render/gl_canvas.h"

#include <glad/gl.h>

#include <algorithm>
#include <cmath>

namespace dx {
    GLCanvas::GLCanvas(UberShader& shaderRef, TextureCache& textureRef)
        : shader(shaderRef)
        , textures(textureRef)
    {
        glGenSamplers(1, &nearestSampler);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    GLCanvas::~GLCanvas()
    {
        glDeleteSamplers(1, &nearestSampler);
    }

    void GLCanvas::begin(int w, int h)
    {
        viewWidth = std::max(w, 1);
        viewHeight = std::max(h, 1);
        vertices.clear();
        batches.clear();
        clipRect.reset();
    }

    float GLCanvas::width() const
    {
        return static_cast<float>(viewWidth);
    }

    float GLCanvas::height() const
    {
        return static_cast<float>(viewHeight);
    }

    void GLCanvas::flush()
    {
        if (vertices.empty()) {
            return;
        }
        shader.use();
        shader.setViewProj(orthoMatrix(static_cast<float>(viewWidth), static_cast<float>(viewHeight)));
        shader.setPan(0, 0);
        shader.setTint(1, 1, 1, 1);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        shader.upload(vertices);
        for (size_t i = 0; i < batches.size(); i++) {
            int32_t end = i + 1 < batches.size() ? batches[i + 1].first : static_cast<int32_t>(vertices.size());
            drawBatch(batches[i], end - batches[i].first);
        }
        glBindSampler(0, 0);
        vertices.clear();
        batches.clear();
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void GLCanvas::drawBatch(const DrawBatch& batch, int32_t count)
    {
        const DrawState& state = batch.state;
        if (state.clip) {
            const ClipRect& clip = *state.clip;
            glEnable(GL_SCISSOR_TEST);
            glScissor(clip[0], viewHeight - clip[1] - clip[3], clip[2], clip[3]);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        int32_t flags = SHADER_Textured;
        BlendMode mode = BlendMode::Alpha;
        switch (state.style) {
        case BlendStyle::Masked:
            flags |= SHADER_AlphaTest;
            mode = BlendMode::Masked;
            break;
        case BlendStyle::Translucent:
            mode = BlendMode::Translucent;
            break;
        case BlendStyle::Modulated:
            mode = BlendMode::Modulated;
            if (state.masked) {
                flags |= SHADER_AlphaTest;
            }
            break;
        case BlendStyle::None:
        case BlendStyle::Normal:
            mode = state.masked ? BlendMode::Masked : (state.texture == textures.white() ? BlendMode::Alpha : BlendMode::Opaque);
            if (state.masked) {
                flags |= SHADER_AlphaTest;
            }
            break;
        }
        shader.setFlags(flags);
        shader.setBlend(mode);
        shader.bindTexture(state.texture);
        glBindSampler(0, state.smooth ? 0 : nearestSampler);
        shader.drawRange(batch.first, count, state.primitive);
    }

    void GLCanvas::setClip(float x, float y, float w, float h)
    {
        clipRect = ClipRect { static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)),
            std::max(static_cast<int>(std::ceil(w)), 0), std::max(static_cast<int>(std::ceil(h)), 0) };
    }

    void GLCanvas::clearClip()
    {
        clipRect.reset();
    }

    void GLCanvas::prepare(uint32_t texture, BlendStyle style, bool masked, bool smooth, uint32_t primitive)
    {
        DrawState state { texture, style, masked, smooth, primitive, clipRect };
        if (batches.empty() || batches.back().state != state) {
            batches.push_back({ state, static_cast<int32_t>(vertices.size()) });
        }
    }

    void GLCanvas::drawTile(UTexture* texture, float x, float y, float w, float h, float u, float v, float ul, float vl, LinearColor color, BlendStyle style, bool smooth)
    {
        if (style == BlendStyle::None) {
            return;
        }
        uint32_t id = textures.white();
        float texWidth = 1;
        float texHeight = 1;
        bool masked = false;
        UTexture* frame = textures.animated(texture);
        const GLTextureInfo* info = style == BlendStyle::Masked ? textures.getMasked(frame) : textures.get(frame);
        if (info) {
            id = info->id;
            texWidth = static_cast<float>(info->width);
            texHeight = static_cast<float>(info->height);
            masked = info->masked;
        } else if (texture) {
            return;
        }
        prepare(id, style, masked, smooth, GL_TRIANGLES);
        if (style == BlendStyle::Modulated) {
            color = { 1, 1, 1, 1 };
        }
        float u0 = u / texWidth;
        float v0 = v / texHeight;
        float u1 = (u + ul) / texWidth;
        float v1 = (v + vl) / texHeight;
        GLVertex corners[4];
        float xs[4] = { x, x + w, x + w, x };
        float ys[4] = { y, y, y + h, y + h };
        float us[4] = { u0, u1, u1, u0 };
        float vs[4] = { v0, v0, v1, v1 };
        for (int i = 0; i < 4; i++) {
            corners[i].x = xs[i];
            corners[i].y = ys[i];
            corners[i].u = us[i];
            corners[i].v = vs[i];
            corners[i].r = color.r;
            corners[i].g = color.g;
            corners[i].b = color.b;
            corners[i].a = color.a;
        }
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int index : order) {
            vertices.push_back(corners[index]);
        }
    }

    void GLCanvas::drawRect(float x, float y, float w, float h, LinearColor color, BlendStyle style)
    {
        drawTile(nullptr, x, y, w, h, 0, 0, 1, 1, color, style == BlendStyle::Masked ? BlendStyle::Normal : style, false);
    }

    void GLCanvas::drawLine(float x1, float y1, float x2, float y2, LinearColor color)
    {
        prepare(textures.white(), BlendStyle::Normal, false, true, GL_LINES);
        GLVertex a;
        a.x = x1 + 0.5f;
        a.y = y1 + 0.5f;
        a.r = color.r;
        a.g = color.g;
        a.b = color.b;
        a.a = color.a;
        GLVertex b = a;
        b.x = x2 + 0.5f;
        b.y = y2 + 0.5f;
        vertices.push_back(a);
        vertices.push_back(b);
    }
}
