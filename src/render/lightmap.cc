#include "render/lightmap.h"

#include "core/objects.h"
#include "engine/props_engine.h"
#include "render/lighting.h"

#include <glad/gl.h>

#include <algorithm>
#include <cmath>

namespace dx {
    namespace {
        struct SurfaceFrame {
            Vector axisU;
            Vector axisV;
            Vector normal;
            Vector base;
        };

        SurfaceFrame frameOf(UModel* model, const BspSurf& surf)
        {
            SurfaceFrame frame;
            frame.axisU = model->vectors.at(static_cast<size_t>(surf.vTextureU));
            frame.axisV = model->vectors.at(static_cast<size_t>(surf.vTextureV));
            frame.normal = model->vectors.at(static_cast<size_t>(surf.vNormal));
            frame.base = model->points.at(static_cast<size_t>(surf.pBase));
            return frame;
        }

        Vector solvePlanePoint(const SurfaceFrame& frame, float u, float v, float d)
        {
            const Vector& a = frame.axisU;
            const Vector& b = frame.axisV;
            const Vector& c = frame.normal;
            float determinant = a.dot(b.cross(c));
            if (std::abs(determinant) < 1e-12f) {
                return frame.base;
            }
            Vector result = (b.cross(c) * u + c.cross(a) * v + a.cross(b) * d) / determinant;
            return result;
        }

        constexpr uint8_t LE_TorchWaver = 1;
        constexpr uint8_t LE_FireWaver = 2;
        constexpr uint8_t LE_WateryShimmer = 3;
        constexpr uint8_t LE_CloudCast = 7;

        void loadShadow(UModel* model, const LightMapIndex& index, int32_t light, std::vector<uint8_t>& shadow)
        {
            int32_t width = index.uClamp;
            int32_t height = index.vClamp;
            int32_t pitch = (width + 7) / 8;
            size_t offset = static_cast<size_t>(index.dataOffset) + static_cast<size_t>(light) * static_cast<size_t>(pitch * height);
            std::vector<uint8_t> bits(static_cast<size_t>(width * height), 0);
            if (offset + static_cast<size_t>(pitch * height) <= model->lightBits.size()) {
                const uint8_t* source = model->lightBits.data() + offset;
                for (int32_t y = 0; y < height; y++) {
                    for (int32_t x = 0; x < width; x++) {
                        bits[static_cast<size_t>(y * width + x)] = (source[y * pitch + (x >> 3)] & (1 << (x & 7))) ? 1 : 0;
                    }
                }
            }
            static const int32_t weights[3][3] = { { 24, 40, 24 }, { 40, 64, 40 }, { 24, 40, 24 } };
            shadow.assign(bits.size(), 0);
            for (int32_t y = 0; y < height; y++) {
                for (int32_t x = 0; x < width; x++) {
                    int32_t value = 0;
                    for (int32_t dy = -1; dy <= 1; dy++) {
                        int32_t sy = std::clamp(y + dy, 0, height - 1);
                        int32_t row = 0;
                        for (int32_t dx = -1; dx <= 1; dx++) {
                            int32_t sx = std::clamp(x + dx, 0, width - 1);
                            if (bits[static_cast<size_t>(sy * width + sx)] > 0) {
                                row += weights[dy + 1][dx + 1];
                            }
                        }
                        value += row * 255 / 320;
                    }
                    shadow[static_cast<size_t>(y * width + x)] = static_cast<uint8_t>(value);
                }
            }
        }

        void illuminate(UModel* model, const BspSurf& surf, const SurfaceLighting& lighting, UObject* light, const std::vector<uint8_t>& source, float time, LightmapClip& clip, std::vector<uint8_t>& illumination)
        {
            Vector location = actor::Location(light);
            PreparedLight prepared = prepareLight(light, location);
            clip = lightmapClip(model, surf, lighting.transform.local(location), prepared.radius);
            illumination.assign(static_cast<size_t>(lighting.width * lighting.height), 0);
            for (int32_t v = clip.minV; v < clip.maxV; v++) {
                for (int32_t u = clip.minU; u < clip.maxU; u++) {
                    size_t i = static_cast<size_t>(v * lighting.width + u);
                    int32_t src = source.empty() ? 127 : source[i];
                    if (src == 0) {
                        continue;
                    }
                    illumination[i] = static_cast<uint8_t>(lightIntensity(prepared, lighting.points[i], lighting.normal, time, src));
                }
            }
        }

        void merge(UObject* light, const LightmapClip& clip, const std::vector<uint8_t>& illumination, int32_t width, float time, std::vector<uint8_t>& bytes)
        {
            Vector color = lightColor(light, time);
            int64_t palette[3] = {
                static_cast<int64_t>(std::floor(color.x * 65536.0f)),
                static_cast<int64_t>(std::floor(color.y * 65536.0f)),
                static_cast<int64_t>(std::floor(color.z * 65536.0f)),
            };
            uint8_t effect = actor::LightEffect(light);
            int32_t key = 0;
            for (int32_t v = clip.minV; v < clip.maxV; v++) {
                for (int32_t u = clip.minU; u < clip.maxU; u++) {
                    size_t i = static_cast<size_t>(v * width + u);
                    int32_t intensity = illumination[i];
                    if (effect == LE_TorchWaver) {
                        intensity = appFloor(static_cast<float>(intensity * (0.95 + 0.05 * static_cast<double>(lightRandomBase(key++))))) & 0xff;
                    } else if (effect == LE_FireWaver) {
                        intensity = appFloor(static_cast<float>(intensity * (0.80 + 0.20 * static_cast<double>(lightRandomBase(key++))))) & 0xff;
                    } else if (effect == LE_WateryShimmer) {
                        intensity = appFloor(static_cast<float>(intensity * (0.60 + 0.40 * static_cast<double>(lightRandom(key++))))) & 0xff;
                    }
                    for (size_t c = 0; c < 3; c++) {
                        int64_t add = std::min((intensity * palette[c]) >> 16, int64_t { 127 });
                        bytes[i * 3 + c] = static_cast<uint8_t>(std::min<int64_t>(bytes[i * 3 + c] + add, 127));
                    }
                }
            }
        }

        constexpr int32_t Padding = 2;

        void padPixels(const std::vector<uint8_t>& bytes, int32_t width, int32_t height, std::vector<uint8_t>& pixels)
        {
            pixels.resize(static_cast<size_t>((width + 2 * Padding) * (height + 2 * Padding) * 4));
            size_t at = 0;
            for (int32_t y = -Padding; y < height + Padding; y++) {
                for (int32_t x = -Padding; x < width + Padding; x++) {
                    size_t i = static_cast<size_t>(std::clamp(y, 0, height - 1) * width + std::clamp(x, 0, width - 1)) * 3;
                    pixels[at + 0] = static_cast<uint8_t>(bytes[i + 0] * 2);
                    pixels[at + 1] = static_cast<uint8_t>(bytes[i + 1] * 2);
                    pixels[at + 2] = static_cast<uint8_t>(bytes[i + 2] * 2);
                    pixels[at + 3] = 255;
                    at += 4;
                }
            }
        }
    }

    Vector lightmapTexelPoint(UModel* model, const BspSurf& surf, int32_t x, int32_t y)
    {
        const LightMapIndex& index = model->lightMaps.at(static_cast<size_t>(surf.lightMap));
        SurfaceFrame frame = frameOf(model, surf);
        float u = frame.axisU.dot(frame.base) + index.pan.x + static_cast<float>(x) * index.uScale;
        float v = frame.axisV.dot(frame.base) + index.pan.y + static_cast<float>(y) * index.vScale;
        float d = frame.normal.dot(frame.base);
        return solvePlanePoint(frame, u, v, d);
    }

    LightmapClip lightmapClip(UModel* model, const BspSurf& surf, const Vector& lightLocation, float radius)
    {
        const LightMapIndex& index = model->lightMaps.at(static_cast<size_t>(surf.lightMap));
        SurfaceFrame frame = frameOf(model, surf);
        float planeDot = (lightLocation - frame.base).dot(frame.normal);
        float planeRadius = sqrtApprox(static_cast<float>(std::max(static_cast<double>(radius * radius) * 1.05 - static_cast<double>(planeDot * planeDot), 0.0)));
        Vector center = lightLocation - frame.normal * planeDot;
        float centerU = (center - frame.base).dot(frame.axisU) - index.pan.x;
        float centerV = (center - frame.base).dot(frame.axisV) - index.pan.y;
        float radiusU = planeRadius * sqrtApprox(frame.axisU.sizeSquared());
        float radiusV = planeRadius * sqrtApprox(frame.axisV.sizeSquared());
        LightmapClip clip;
        clip.minU = std::max(static_cast<int32_t>(std::nearbyint((centerU - radiusU) / index.uScale)), 0);
        clip.minV = std::max(static_cast<int32_t>(std::nearbyint((centerV - radiusV) / index.vScale)), 0);
        clip.maxU = std::min(static_cast<int32_t>(std::nearbyint((centerU + radiusU) / index.uScale)), index.uClamp);
        clip.maxV = std::min(static_cast<int32_t>(std::nearbyint((centerV + radiusV) / index.vScale)), index.vClamp);
        return clip;
    }

    Vector LightmapTransform::point(const Vector& local) const
    {
        if (identity) {
            return local;
        }
        return location + untransformVector(rotation, (local - prePivot) * scale);
    }

    Vector LightmapTransform::direction(const Vector& local) const
    {
        if (identity) {
            return local;
        }
        return untransformVector(rotation, local * Vector { 1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z }).normal();
    }

    Vector LightmapTransform::local(const Vector& world) const
    {
        if (identity) {
            return world;
        }
        return transformVector(rotation, world - location) * Vector { 1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z } + prePivot;
    }

    SurfaceLighting prepareSurfaceLighting(UModel* model, const BspSurf& surf, const Vector& ambient, const LightmapTransform& transform, const std::vector<UObject*>* lights)
    {
        SurfaceLighting lighting;
        lighting.transform = transform;
        const LightMapIndex& index = model->lightMaps.at(static_cast<size_t>(surf.lightMap));
        lighting.width = std::max(index.uClamp, 1);
        lighting.height = std::max(index.vClamp, 1);
        size_t count = static_cast<size_t>(lighting.width * lighting.height);
        uint8_t ambientBytes[3] = {
            static_cast<uint8_t>(std::clamp(static_cast<int32_t>(std::floor(ambient.x * 64.0f)), 0, 255)),
            static_cast<uint8_t>(std::clamp(static_cast<int32_t>(std::floor(ambient.y * 64.0f)), 0, 255)),
            static_cast<uint8_t>(std::clamp(static_cast<int32_t>(std::floor(ambient.z * 64.0f)), 0, 255)),
        };
        lighting.base.resize(count * 3);
        for (size_t i = 0; i < count; i++) {
            lighting.base[i * 3 + 0] = ambientBytes[0];
            lighting.base[i * 3 + 1] = ambientBytes[1];
            lighting.base[i * 3 + 2] = ambientBytes[2];
        }
        lighting.points.resize(count);
        for (int32_t y = 0; y < lighting.height; y++) {
            for (int32_t x = 0; x < lighting.width; x++) {
                lighting.points[static_cast<size_t>(y * lighting.width + x)] = transform.point(lightmapTexelPoint(model, surf, x, y));
            }
        }
        lighting.normal = transform.direction(model->vectors.at(static_cast<size_t>(surf.vNormal)).normal());
        std::vector<uint8_t> shadow;
        std::vector<uint8_t> illumination;
        if (lights) {
            Vector worldBase = transform.point(model->points.at(static_cast<size_t>(surf.pBase)));
            for (UObject* light : *lights) {
                if (!lightOnSurfaces(light) || (actor::Location(light) - worldBase).dot(lighting.normal) <= 0) {
                    continue;
                }
                if (lightBakedOnSurfaces(light)) {
                    LightmapClip clip;
                    illuminate(model, surf, lighting, light, {}, 0, clip, illumination);
                    merge(light, clip, illumination, lighting.width, 0, lighting.base);
                    continue;
                }
                SurfaceLighting::Light entry;
                entry.light = light;
                entry.moving = lightMovesOnSurfaces(light);
                lighting.lights.push_back(std::move(entry));
            }
            return lighting;
        }
        if (index.lightActors < 0) {
            return lighting;
        }
        for (size_t slot = static_cast<size_t>(index.lightActors); slot < model->lights.size(); slot++) {
            UObject* light = model->lights[slot];
            if (!light) {
                break;
            }
            objects::ensureLoaded(light);
            if (!lightOnSurfaces(light)) {
                continue;
            }
            int32_t shadowIndex = static_cast<int32_t>(slot) - index.lightActors;
            if (lightBakedOnSurfaces(light)) {
                LightmapClip clip;
                loadShadow(model, index, shadowIndex, shadow);
                illuminate(model, surf, lighting, light, shadow, 0, clip, illumination);
                merge(light, clip, illumination, lighting.width, 0, lighting.base);
                continue;
            }
            SurfaceLighting::Light entry;
            entry.light = light;
            entry.moving = lightMovesOnSurfaces(light);
            if (!entry.moving) {
                loadShadow(model, index, shadowIndex, entry.source);
            }
            lighting.lights.push_back(std::move(entry));
        }
        return lighting;
    }

    void evaluateSurfaceLighting(SurfaceLighting& lighting, UModel* model, const BspSurf& surf, float time, std::vector<uint8_t>& bytes)
    {
        bytes = lighting.base;
        for (SurfaceLighting::Light& entry : lighting.lights) {
            if (!lightOnSurfaces(entry.light)) {
                continue;
            }
            uint8_t effect = actor::LightEffect(entry.light);
            bool spatial = entry.moving || (lightEffectSpatialDynamic(effect) && effect != LE_CloudCast);
            if (spatial || !entry.cached) {
                illuminate(model, surf, lighting, entry.light, entry.source, time, entry.clip, entry.illumination);
                entry.cached = !spatial;
            }
            merge(entry.light, entry.clip, entry.illumination, lighting.width, time, bytes);
        }
    }

    LightmapTexels computeLightmap(UModel* model, const BspSurf& surf, const Vector& ambient, float time)
    {
        SurfaceLighting lighting = prepareSurfaceLighting(model, surf, ambient);
        std::vector<uint8_t> bytes;
        evaluateSurfaceLighting(lighting, model, surf, time, bytes);
        LightmapTexels result;
        result.width = lighting.width;
        result.height = lighting.height;
        size_t count = static_cast<size_t>(result.width * result.height);
        result.colors.resize(count);
        for (size_t i = 0; i < count; i++) {
            result.colors[i] = Vector { static_cast<float>(bytes[i * 3 + 0]), static_cast<float>(bytes[i * 3 + 1]), static_cast<float>(bytes[i * 3 + 2]) } * (1.0f / 127.5f);
        }
        return result;
    }

    LightmapAtlas::LightmapAtlas(int32_t size)
        : PageSize(size)
    {
    }

    bool LightmapAtlas::built() const
    {
        return isBuilt;
    }

    LightmapAtlas::~LightmapAtlas()
    {
        clear();
    }

    void LightmapAtlas::clear()
    {
        if (!textures.empty()) {
            glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
        }
        textures.clear();
        if (!fogTextures.empty()) {
            glDeleteTextures(static_cast<GLsizei>(fogTextures.size()), fogTextures.data());
        }
        fogTextures.clear();
        pages.clear();
        placements.clear();
        variantPlacements.clear();
        variantIndex.clear();
        animated.clear();
        isBuilt = false;
    }

    bool LightmapAtlas::place(int32_t width, int32_t height, LightmapPlacement& placement)
    {
        int32_t paddedWidth = width + 2 * Padding;
        int32_t paddedHeight = height + 2 * Padding;
        if (paddedWidth > PageSize || paddedHeight > PageSize) {
            return false;
        }
        for (int32_t attempt = 0; attempt < 2; attempt++) {
            if (pages.empty() || attempt == 1) {
                Page page;
                page.pixels.assign(static_cast<size_t>(PageSize * PageSize * 4), 255);
                pages.push_back(std::move(page));
            }
            Page& page = pages.back();
            if (page.shelfX + paddedWidth > PageSize) {
                page.shelfX = 0;
                page.shelfY += page.shelfHeight;
                page.shelfHeight = 0;
            }
            if (page.shelfY + paddedHeight <= PageSize) {
                placement.page = static_cast<int32_t>(pages.size() - 1);
                placement.x = page.shelfX + Padding;
                placement.y = page.shelfY + Padding;
                placement.width = width;
                placement.height = height;
                page.shelfX += paddedWidth;
                page.shelfHeight = std::max(page.shelfHeight, paddedHeight);
                return true;
            }
        }
        return false;
    }

    void LightmapAtlas::store(UModel* model, size_t surfIndex, const Vector& ambient, float time, const LightmapTransform& transform, const std::vector<UObject*>* lights, LightmapPlacement& placement, int32_t variant, std::vector<int32_t>* surfAnimated)
    {
        const BspSurf& surf = model->surfs[surfIndex];
        SurfaceLighting lighting = prepareSurfaceLighting(model, surf, ambient, transform, lights);
        if (!place(lighting.width, lighting.height, placement)) {
            return;
        }
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> pixels;
        evaluateSurfaceLighting(lighting, model, surf, time, bytes);
        padPixels(bytes, lighting.width, lighting.height, pixels);
        Page& page = pages[static_cast<size_t>(placement.page)];
        size_t rowBytes = static_cast<size_t>(lighting.width + 2 * Padding) * 4;
        for (int32_t y = 0; y < lighting.height + 2 * Padding; y++) {
            size_t at = static_cast<size_t>(((placement.y - Padding + y) * PageSize + placement.x - Padding) * 4);
            std::copy_n(pixels.data() + static_cast<size_t>(y) * rowBytes, rowBytes, page.pixels.data() + at);
        }
        if (!lighting.lights.empty()) {
            if (surfAnimated) {
                (*surfAnimated)[surfIndex] = static_cast<int32_t>(animated.size());
            }
            Animated entry;
            entry.surf = surfIndex;
            entry.lightMap = static_cast<size_t>(surf.lightMap);
            entry.variant = variant;
            entry.lighting = std::move(lighting);
            entry.bytes = bytes;
            animated.push_back(std::move(entry));
        }
    }

    const LightmapPlacement* LightmapAtlas::placementFor(int32_t lightMap, int32_t zone) const
    {
        if (lightMap < 0 || static_cast<size_t>(lightMap) >= placements.size()) {
            return nullptr;
        }
        if (zone >= 0) {
            auto found = variantIndex.find((static_cast<uint64_t>(lightMap) << 8) | static_cast<uint64_t>(zone & 0xff));
            if (found != variantIndex.end()) {
                return &variantPlacements[found->second].second;
            }
        }
        return &placements[static_cast<size_t>(lightMap)];
    }

    void LightmapAtlas::build(UModel* model, const std::vector<Vector>& surfAmbient, float time, const LightmapTransform& transform, const std::vector<UObject*>* lights, const std::vector<LightmapVariant>* variants)
    {
        clear();
        isBuilt = true;
        placements.assign(model->lightMaps.size(), LightmapPlacement {});
        std::vector<int32_t> surfAnimated(model->surfs.size(), -1);
        for (size_t surfIndex = 0; surfIndex < model->surfs.size(); surfIndex++) {
            const BspSurf& surf = model->surfs[surfIndex];
            if (surf.lightMap < 0 || static_cast<size_t>(surf.lightMap) >= model->lightMaps.size()) {
                continue;
            }
            LightmapPlacement& placement = placements[static_cast<size_t>(surf.lightMap)];
            if (placement.page >= 0) {
                continue;
            }
            Vector ambient = surfIndex < surfAmbient.size() ? surfAmbient[surfIndex] : Vector {};
            store(model, surfIndex, ambient, time, transform, lights, placement, -1, &surfAnimated);
        }
        if (variants) {
            variantPlacements.reserve(variants->size());
            for (const LightmapVariant& variant : *variants) {
                const BspSurf& surf = model->surfs[static_cast<size_t>(variant.surf)];
                if (surf.lightMap < 0 || static_cast<size_t>(surf.lightMap) >= model->lightMaps.size()) {
                    continue;
                }
                uint64_t key = (static_cast<uint64_t>(surf.lightMap) << 8) | variant.zone;
                variantIndex[key] = variantPlacements.size();
                variantPlacements.emplace_back(key, LightmapPlacement {});
                store(model, static_cast<size_t>(variant.surf), variant.ambient, time, transform, lights, variantPlacements.back().second, static_cast<int32_t>(variantPlacements.size() - 1), nullptr);
            }
        }
        for (const BspNode& node : model->nodes) {
            if (node.surf < 0 || static_cast<size_t>(node.surf) >= surfAnimated.size() || surfAnimated[static_cast<size_t>(node.surf)] < 0) {
                continue;
            }
            animated[static_cast<size_t>(surfAnimated[static_cast<size_t>(node.surf)])].zones |= 1ull << (node.zone[1] & 63);
        }
        for (Animated& entry : animated) {
            if (entry.variant < 0 || surfAnimated[entry.surf] < 0) {
                continue;
            }
            entry.zones = 1ull << (variantPlacements[static_cast<size_t>(entry.variant)].first & 63);
        }
        textures.resize(pages.size());
        if (!textures.empty()) {
            glGenTextures(static_cast<GLsizei>(textures.size()), textures.data());
        }
        for (size_t i = 0; i < pages.size(); i++) {
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, PageSize, PageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pages[i].pixels.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            pages[i].pixels.clear();
            pages[i].pixels.shrink_to_fit();
        }
    }

    void LightmapAtlas::animate(UModel* model, float time, uint64_t frame, const std::function<bool(size_t surf, uint64_t zones)>& visible)
    {
        std::vector<uint8_t> bytes;
        for (Animated& entry : animated) {
            if (entry.frame == frame || !visible(entry.surf, entry.zones)) {
                continue;
            }
            entry.frame = frame;
            evaluateSurfaceLighting(entry.lighting, model, model->surfs[entry.surf], time, bytes);
            if (bytes == entry.bytes) {
                continue;
            }
            entry.bytes.swap(bytes);
            const LightmapPlacement& placement = entry.variant >= 0 ? variantPlacements[static_cast<size_t>(entry.variant)].second : placements[entry.lightMap];
            padPixels(entry.bytes, entry.lighting.width, entry.lighting.height, entry.pixels);
            glBindTexture(GL_TEXTURE_2D, textures[static_cast<size_t>(placement.page)]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, placement.x - Padding, placement.y - Padding, entry.lighting.width + 2 * Padding, entry.lighting.height + 2 * Padding, GL_RGBA, GL_UNSIGNED_BYTE, entry.pixels.data());
        }
    }

    bool LightmapAtlas::texCoord(UModel* model, const BspSurf& surf, const Vector& point, float& s, float& t, int32_t& page, int32_t zone) const
    {
        const LightmapPlacement* found = placementFor(surf.lightMap, zone);
        if (!found || found->page < 0) {
            return false;
        }
        const LightmapPlacement& placement = *found;
        if (placement.page < 0) {
            return false;
        }
        const LightMapIndex& index = model->lightMaps[static_cast<size_t>(surf.lightMap)];
        SurfaceFrame frame = frameOf(model, surf);
        float panU = frame.axisU.dot(frame.base) + index.pan.x - 0.5f * index.uScale;
        float panV = frame.axisV.dot(frame.base) + index.pan.y - 0.5f * index.vScale;
        float localS = (frame.axisU.dot(point) - panU) / index.uScale;
        float localT = (frame.axisV.dot(point) - panV) / index.vScale;
        s = (static_cast<float>(placement.x) + localS) / static_cast<float>(PageSize);
        t = (static_cast<float>(placement.y) + localT) / static_cast<float>(PageSize);
        page = placement.page;
        return true;
    }

    void LightmapAtlas::uploadFog(const BspSurf& surf, int32_t width, int32_t height, const std::vector<uint8_t>& bytes)
    {
        if (surf.lightMap < 0 || static_cast<size_t>(surf.lightMap) >= placements.size()) {
            return;
        }
        std::vector<const LightmapPlacement*> targets { &placements[static_cast<size_t>(surf.lightMap)] };
        for (const auto& [key, variant] : variantPlacements) {
            if ((key >> 8) == static_cast<uint64_t>(surf.lightMap)) {
                targets.push_back(&variant);
            }
        }
        if (fogTextures.empty() && !textures.empty()) {
            fogTextures.resize(textures.size());
            glGenTextures(static_cast<GLsizei>(fogTextures.size()), fogTextures.data());
            std::vector<uint8_t> zeros(static_cast<size_t>(PageSize * PageSize * 4), 0);
            for (uint32_t id : fogTextures) {
                glBindTexture(GL_TEXTURE_2D, id);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, PageSize, PageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }
        padPixels(bytes, width, height, fogPixels);
        for (size_t i = 3; i < fogPixels.size(); i += 4) {
            fogPixels[i] = 0;
        }
        for (const LightmapPlacement* placement : targets) {
            if (placement->page < 0 || static_cast<size_t>(placement->page) >= fogTextures.size()) {
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, fogTextures[static_cast<size_t>(placement->page)]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, placement->x - Padding, placement->y - Padding, width + 2 * Padding, height + 2 * Padding, GL_RGBA, GL_UNSIGNED_BYTE, fogPixels.data());
        }
    }

    uint32_t LightmapAtlas::fogTexture(int32_t page) const
    {
        if (page < 0 || static_cast<size_t>(page) >= fogTextures.size()) {
            return 0;
        }
        return fogTextures[static_cast<size_t>(page)];
    }

    uint32_t LightmapAtlas::texture(int32_t page) const
    {
        if (page < 0 || static_cast<size_t>(page) >= textures.size()) {
            return 0;
        }
        return textures[static_cast<size_t>(page)];
    }
}
