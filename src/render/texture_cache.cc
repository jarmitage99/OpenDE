#include "render/texture_cache.h"

#include "core/objects.h"
#include "engine/props_engine.h"

#include <glad/gl.h>

#ifdef __EMSCRIPTEN__
#include "render/gl_shader.h"
#include <emscripten.h>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <exception>
#include <vector>

namespace dx {
    namespace {
#ifdef __EMSCRIPTEN__
        EM_JS(void, uploadProceduralPixels, (GLenum target, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data), {
            GLctx.texSubImage2D(target, 0, 0, 0, width, height, format, type, HEAPU8, data >>> 0);
        });

        void createRippleInput(uint32_t& texture, GLint internalFormat, GLenum format, int32_t width, int32_t height, const void* data)
        {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
#endif

        void setParameters(bool smooth, int32_t levels, bool trilinear, float anisotropy)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            if (smooth) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, levels > 1 ? (trilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST) : GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic) {
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, smooth && levels > 1 ? anisotropy : 1.0f);
            }
        }
    }

    TextureCache::TextureCache()
    {
        uint8_t pixel[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        setParameters(false, 1, false, 1.0f);
        if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic) {
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
        }
    }

    TextureCache::~TextureCache()
    {
        clear();
        glDeleteTextures(1, &whiteTexture);
    }

    void TextureCache::clear()
    {
        for (auto& [texture, info] : textures) {
            glDeleteTextures(1, &info.id);
        }
        textures.clear();
        for (auto& [texture, info] : maskedTextures) {
            glDeleteTextures(1, &info.id);
        }
        maskedTextures.clear();
        animations.clear();
        for (auto& [texture, entry] : proceduralTextures) {
#ifdef __EMSCRIPTEN__
            entry.clearRippleInputs();
#endif
            for (GLTextureInfo& buffer : entry.buffers) {
                glDeleteTextures(1, &buffer.id);
            }
        }
        proceduralTextures.clear();
        procedural.clear();
        replacements.rescan();
    }

    void TextureCache::resetProcedural(UTexture* texture)
    {
        procedural.reset(texture);
#ifdef __EMSCRIPTEN__
        auto entry = proceduralTextures.find(texture);
        if (entry != proceduralTextures.end()) {
            entry->second.clearRippleInputs();
            entry->second.lastFrame = 0;
        }
#endif
    }

    uint32_t TextureCache::white()
    {
        return whiteTexture;
    }

    void TextureCache::beginFrame(double now)
    {
        time = now;
        frame++;
    }

    UTexture* TextureCache::animated(UTexture* texture)
    {
        if (!texture || !texture->data || !texture::AnimNext(texture)) {
            return texture;
        }
        return advance(texture, animations[texture], time, constantRate);
    }

    UTexture* TextureCache::advance(UTexture* texture, TextureAnimation& animation, double now, float constantRate)
    {
        auto constantTimeTick = [&]() {
            UTexture* from = animation.current ? animation.current : texture;
            UObject* following = from->data ? texture::AnimNext(from) : nullptr;
            objects::ensureLoaded(following);
            UTexture* candidate = cast<UTexture>(following);
            animation.current = candidate ? candidate : texture;
        };
        if (now != animation.lastUpdate) {
            float delta = static_cast<float>(now - animation.lastUpdate);
            animation.lastUpdate = now;
            while (animation.primeCurrent < texture::PrimeCount(texture)) {
                animation.primeCurrent++;
                constantTimeTick();
            }
            float maxRate = texture::MaxFrameRate(texture);
            float minRate = texture::MinFrameRate(texture);
            if (maxRate == 0.0f && constantRate > 0.0f) {
                maxRate = constantRate;
                minRate = constantRate;
            }
            if (maxRate == 0.0f) {
                constantTimeTick();
            } else {
                float minTime = 1.0f / std::clamp(maxRate, 0.01f, 100.0f);
                float maxTime = 1.0f / std::clamp(minRate, 0.01f, 100.0f);
                animation.accumulator += delta;
                if (animation.accumulator >= minTime) {
                    constantTimeTick();
                    if (animation.accumulator < maxTime) {
                        animation.accumulator = 0;
                    } else {
                        animation.accumulator -= maxTime;
                        if (animation.accumulator > maxTime) {
                            animation.accumulator = maxTime;
                        }
                    }
                }
            }
        }
        return animation.current ? animation.current : texture;
    }

    const GLTextureInfo* TextureCache::get(UTexture* texture)
    {
        if (!texture) {
            return nullptr;
        }
        objects::ensureLoaded(texture);
        auto it = textures.find(texture);
        if (it != textures.end()) {
            return it->second.id ? &it->second : nullptr;
        }
        auto dynamic = proceduralTextures.find(texture);
        if (dynamic != proceduralTextures.end()) {
            return getProcedural(texture, dynamic->second);
        }
        if (ProceduralTextures::isProcedural(texture)) {
            return getProcedural(texture, proceduralTextures[texture]);
        }

        GLTextureInfo& info = textures[texture];
        if (!texture->decodable()) {
            return nullptr;
        }
        return upload(texture, info, texture->masked()) ? &info : nullptr;
    }

    const GLTextureInfo* TextureCache::getProcedural(UTexture* texture, ProceduralTexture& entry)
    {
        const GLTextureInfo& current = entry.buffers[entry.front];
        if (current.id && entry.lastFrame == frame) {
            return &current;
        }
        entry.lastFrame = frame;
        uint32_t width = 0;
        uint32_t height = 0;
#ifdef __EMSCRIPTEN__
        RippleFrame ripple;
        if (!procedural.update(texture, time, proceduralPixels, width, height, frame, &ripple) || (!ripple.heights && proceduralPixels.empty())) {
#else
        if (!procedural.update(texture, time, proceduralPixels, width, height, frame) || proceduralPixels.empty()) {
#endif
            return current.id ? &current : nullptr;
        }
        uint32_t next = entry.front;
#ifdef __EMSCRIPTEN__
        if (current.id) {
            next ^= 1;
        }
#endif
        GLTextureInfo& info = entry.buffers[next];
        const bool reuseStorage = info.id && info.width == width && info.height == height;
        if (!info.id) {
            glGenTextures(1, &info.id);
        }
        const auto uploadWidth = static_cast<GLsizei>(width);
        const auto uploadHeight = static_cast<GLsizei>(height);
        glBindTexture(GL_TEXTURE_2D, info.id);
#ifdef __EMSCRIPTEN__
        if (ripple.heights) {
            if (!reuseStorage) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, uploadWidth, uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                setParameters(true, 1, trilinear, anisotropy);
            }
            renderRipple(entry, next, ripple, uploadWidth, uploadHeight);
        } else if (reuseStorage) {
            uploadProceduralPixels(GL_TEXTURE_2D, uploadWidth, uploadHeight, GL_RGBA, GL_UNSIGNED_BYTE, proceduralPixels.data());
#else
        if (reuseStorage) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, uploadWidth, uploadHeight, GL_RGBA, GL_UNSIGNED_BYTE, proceduralPixels.data());
#endif
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, uploadWidth, uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, proceduralPixels.data());
            setParameters(true, 1, trilinear, anisotropy);
        }
        info.width = width;
        info.height = height;
        info.masked = texture->masked();
        info.procedural = true;
        info.smooth = true;
#ifdef __EMSCRIPTEN__
        entry.front = next;
#endif
        return &info;
    }

#ifdef __EMSCRIPTEN__
    void TextureCache::ProceduralTexture::clearRippleInputs()
    {
        glDeleteTextures(2, heights);
        glDeleteTextures(1, &lookup);
        glDeleteTextures(1, &source);
        heights[0] = 0;
        heights[1] = 0;
        lookup = 0;
        source = 0;
    }

    void TextureCache::renderRipple(ProceduralTexture& entry, uint32_t next, const RippleFrame& ripple, int32_t width, int32_t height)
    {
        if (!ripplePass) {
            ripplePass = std::make_unique<RipplePass>();
        }
        if (!entry.lookup) {
            // The first row is the RGBA palette. The other four rows store the slope table.
            std::array<uint8_t, 256 * 5 * 4> lookup {};
            for (size_t i = 0; i < 256; i++) {
                uint32_t index = static_cast<uint32_t>(i);
                uint32_t color = ripple.palette ? ripple.palette[i] : index * 0x010101u;
                lookup[i * 4] = static_cast<uint8_t>(color);
                lookup[i * 4 + 1] = static_cast<uint8_t>(color >> 8);
                lookup[i * 4 + 2] = static_cast<uint8_t>(color >> 16);
                lookup[i * 4 + 3] = i == 0 ? 0 : 255;
            }
            for (size_t i = 0; i < 1024; i++) {
                lookup[(256 + i) * 4] = ripple.slopes[i];
            }
            createRippleInput(entry.lookup, GL_RGBA8, GL_RGBA, 256, 5, lookup.data());
            if (ripple.source) {
                createRippleInput(entry.source, GL_R8, GL_RED, width, height, ripple.source);
            }
        }
        uint32_t& heights = entry.heights[next];
        if (!heights) {
            createRippleInput(heights, GL_R8, GL_RED, width / 2, height, ripple.heights);
        } else {
            glBindTexture(GL_TEXTURE_2D, heights);
            uploadProceduralPixels(GL_TEXTURE_2D, width / 2, height, GL_RED, GL_UNSIGNED_BYTE, ripple.heights);
        }
        ripplePass->render(entry.buffers[next].id, heights, entry.lookup, entry.source, width, height, ripple.even);
        glBindTexture(GL_TEXTURE_2D, entry.buffers[next].id);
    }
#endif

    const GLTextureInfo* TextureCache::getMasked(UTexture* texture)
    {
        if (!texture) {
            return nullptr;
        }
        objects::ensureLoaded(texture);
        if (texture->masked() || texture->format() != 0 || ProceduralTextures::isProcedural(texture) || !texture->decodable()) {
            return get(texture);
        }
        auto it = maskedTextures.find(texture);
        if (it != maskedTextures.end()) {
            return it->second.id ? &it->second : nullptr;
        }
        GLTextureInfo& info = maskedTextures[texture];
        return upload(texture, info, true) ? &info : nullptr;
    }

    bool TextureCache::upload(UTexture* texture, GLTextureInfo& info, bool masked)
    {
        info.width = texture->width();
        info.height = texture->height();
        info.masked = masked;
        if (uploadReplacement(texture, info)) {
            return true;
        }
        glGenTextures(1, &info.id);
        glBindTexture(GL_TEXTURE_2D, info.id);
        int32_t levels = 0;
        for (size_t mip = 0; mip < texture->mips.size(); mip++) {
            const TextureMip& level = texture->mips[mip];
            if (level.width == 0 || level.height == 0) {
                break;
            }
            if (mip > 0 && (level.width != std::max(1u, info.width >> mip) || level.height != std::max(1u, info.height >> mip))) {
                break;
            }
            std::vector<uint8_t> pixels;
            try {
                pixels = texture->rgba(mip);
            } catch (const std::exception&) {
                break;
            }
            if (texture->format() == 0) {
                size_t count = std::min(level.data.size(), pixels.size() / 4);
                for (size_t i = 0; i < count; i++) {
                    pixels[i * 4 + 3] = level.data[i] == 0 ? 0 : 255;
                    if (info.masked && level.data[i] == 0) {
                        pixels[i * 4 + 0] = 0;
                        pixels[i * 4 + 1] = 0;
                        pixels[i * 4 + 2] = 0;
                    }
                }
            }
            glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mip), GL_RGBA8, static_cast<GLsizei>(level.width), static_cast<GLsizei>(level.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            levels++;
            if (level.width == 1 && level.height == 1) {
                break;
            }
        }
        if (levels == 0) {
            glDeleteTextures(1, &info.id);
            info.id = 0;
            return false;
        }
        info.smooth = !texture::bNoSmooth(texture);
        info.levels = levels;
        setParameters(info.smooth, levels, trilinear, anisotropy);
        return true;
    }

    bool TextureCache::uploadReplacement(UTexture* texture, GLTextureInfo& info)
    {
        ReplacementImage image;
        if (!replacements.load(texture, image)) {
            return false;
        }
        if (info.masked) {
            for (size_t i = 0; i < image.rgba.size(); i += 4) {
                if (image.rgba[i + 3] == 0) {
                    image.rgba[i + 0] = 0;
                    image.rgba[i + 1] = 0;
                    image.rgba[i + 2] = 0;
                }
            }
        }
        glGenTextures(1, &info.id);
        glBindTexture(GL_TEXTURE_2D, info.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        info.smooth = !texture::bNoSmooth(texture);
        info.levels = static_cast<int32_t>(std::bit_width(std::max(image.width, image.height)));
        setParameters(info.smooth, info.levels, trilinear, anisotropy);
        return true;
    }

    void TextureCache::setConstantRate(float rate)
    {
        constantRate = rate;
        procedural.setConstantRate(rate);
    }

    void TextureCache::setFiltering(float newAnisotropy, bool newTrilinear)
    {
        newAnisotropy = std::clamp(newAnisotropy, 1.0f, maxAnisotropy);
        if (newAnisotropy == anisotropy && newTrilinear == trilinear) {
            return;
        }
        anisotropy = newAnisotropy;
        trilinear = newTrilinear;
        for (auto* cache : { &textures, &maskedTextures }) {
            for (auto& [texture, info] : *cache) {
                if (info.id) {
                    glBindTexture(GL_TEXTURE_2D, info.id);
                    setParameters(info.smooth, info.levels, trilinear, anisotropy);
                }
            }
        }
    }
}
