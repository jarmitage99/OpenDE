#include "render/texture_cache.h"

#include "core/objects.h"
#include "engine/props_engine.h"

#include <glad/gl.h>

#include <algorithm>
#include <exception>
#include <vector>

namespace dx {
    namespace {
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
        proceduralFrames.clear();
        procedural.clear();
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
        return advance(texture, animations[texture], time);
    }

    UTexture* TextureCache::advance(UTexture* texture, TextureAnimation& animation, double now)
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
            if (maxRate == 0.0f) {
                constantTimeTick();
            } else {
                float minTime = 1.0f / std::clamp(maxRate, 0.01f, 100.0f);
                float maxTime = 1.0f / std::clamp(texture::MinFrameRate(texture), 0.01f, 100.0f);
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
        if (it != textures.end() && !it->second.procedural) {
            return it->second.id ? &it->second : nullptr;
        }
        bool isProcedural = it != textures.end() ? it->second.procedural : ProceduralTextures::isProcedural(texture);
        if (isProcedural) {
            uint64_t& lastFrame = proceduralFrames[texture];
            if (it != textures.end() && lastFrame == frame) {
                return &it->second;
            }
            lastFrame = frame;
            std::vector<uint8_t> rgba;
            uint32_t width = 0;
            uint32_t height = 0;
            bool changed = procedural.update(texture, time, rgba, width, height);
            if (it != textures.end() && !changed) {
                return &it->second;
            }
            if (rgba.empty()) {
                return it != textures.end() ? &it->second : nullptr;
            }
            GLTextureInfo& info = textures[texture];
            info.procedural = true;
            if (!info.id) {
                glGenTextures(1, &info.id);
            }
            info.width = width;
            info.height = height;
            info.masked = texture->masked();
            glBindTexture(GL_TEXTURE_2D, info.id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            info.smooth = true;
            info.levels = 1;
            setParameters(true, 1, trilinear, anisotropy);
            return &info;
        }

        GLTextureInfo& info = textures[texture];
        if (!texture->decodable()) {
            return nullptr;
        }
        return upload(texture, info, texture->masked()) ? &info : nullptr;
    }

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
