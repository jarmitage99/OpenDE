#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace dx {
    struct LightmapPlacement {
        int32_t page = -1;
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;
    };

    struct LightmapVariant {
        int32_t surf = 0;
        uint8_t zone = 0;
        Vector ambient;
    };

    struct LightmapTexels {
        int32_t width = 0;
        int32_t height = 0;
        std::vector<Vector> colors;
    };

    struct LightmapClip {
        int32_t minU = 0;
        int32_t maxU = 0;
        int32_t minV = 0;
        int32_t maxV = 0;
    };

    struct LightmapTransform {
        bool identity = true;
        Vector location;
        Coords rotation;
        Vector scale { 1, 1, 1 };
        Vector prePivot;

        Vector point(const Vector& local) const;
        Vector direction(const Vector& local) const;
        Vector local(const Vector& world) const;
    };

    struct SurfaceLighting {
        struct Light {
            UObject* light = nullptr;
            bool moving = false;
            bool cached = false;
            LightmapClip clip;
            std::vector<uint8_t> source;
            std::vector<uint8_t> illumination;
        };

        int32_t width = 0;
        int32_t height = 0;
        Vector normal;
        std::vector<Vector> points;
        std::vector<uint8_t> base;
        std::vector<Light> lights;
        LightmapTransform transform;
    };

    LightmapTexels computeLightmap(UModel* model, const BspSurf& surf, const Vector& ambient, float time = 0);
    SurfaceLighting prepareSurfaceLighting(UModel* model, const BspSurf& surf, const Vector& ambient, const LightmapTransform& transform = {}, const std::vector<UObject*>* lights = nullptr);
    void evaluateSurfaceLighting(SurfaceLighting& lighting, UModel* model, const BspSurf& surf, float time, std::vector<uint8_t>& bytes);
    LightmapClip lightmapClip(UModel* model, const BspSurf& surf, const Vector& lightLocation, float radius);
    Vector lightmapTexelPoint(UModel* model, const BspSurf& surf, int32_t x, int32_t y);

    class LightmapAtlas {
    public:
        explicit LightmapAtlas(int32_t size = 1024);
        ~LightmapAtlas();
        LightmapAtlas(const LightmapAtlas&) = delete;
        LightmapAtlas& operator=(const LightmapAtlas&) = delete;

        void build(UModel* model, const std::vector<Vector>& surfAmbient, float time, const LightmapTransform& transform = {}, const std::vector<UObject*>* lights = nullptr, const std::vector<LightmapVariant>* variants = nullptr);
        bool built() const;
        void animate(UModel* model, float time, uint64_t frame, const std::function<bool(size_t surf, uint64_t zones)>& visible);
        void clear();
        bool texCoord(UModel* model, const BspSurf& surf, const Vector& point, float& s, float& t, int32_t& page, int32_t zone = -1) const;
        uint32_t texture(int32_t page) const;
        void uploadFog(const BspSurf& surf, int32_t width, int32_t height, const std::vector<uint8_t>& bytes);
        uint32_t fogTexture(int32_t page) const;

    private:
        struct Page {
            std::vector<uint8_t> pixels;
            int32_t shelfX = 0;
            int32_t shelfY = 0;
            int32_t shelfHeight = 0;
        };

        struct Animated {
            size_t surf = 0;
            size_t lightMap = 0;
            int32_t variant = -1;
            SurfaceLighting lighting;
            uint64_t zones = 0;
            uint64_t frame = 0;
            std::vector<uint8_t> bytes;
            std::vector<uint8_t> pixels;
        };

        bool place(int32_t width, int32_t height, LightmapPlacement& placement);
        void store(UModel* model, size_t surfIndex, const Vector& ambient, float time, const LightmapTransform& transform, const std::vector<UObject*>* lights, LightmapPlacement& placement, int32_t variant, std::vector<int32_t>* surfAnimated);
        const LightmapPlacement* placementFor(int32_t lightMap, int32_t zone) const;

        int32_t PageSize = 1024;
        bool isBuilt = false;
        std::vector<LightmapPlacement> placements;
        std::vector<std::pair<uint64_t, LightmapPlacement>> variantPlacements;
        std::unordered_map<uint64_t, size_t> variantIndex;
        std::vector<Page> pages;
        std::vector<uint32_t> textures;
        std::vector<uint32_t> fogTextures;
        std::vector<uint8_t> fogPixels;
        std::vector<Animated> animated;
    };
}
