#pragma once

#include "core/math.h"
#include "core/object.h"

#include <cstdint>
#include <string>
#include <vector>

namespace dx {
    class UPalette : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        std::vector<uint32_t> colors;
    };

    struct TextureMip {
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t ubits = 0;
        uint8_t vbits = 0;
        std::vector<uint8_t> data;
    };

    class UTexture : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        uint8_t format() const;
        UPalette* palette() const;
        bool masked() const;
        uint32_t width() const;
        uint32_t height() const;
        bool decodable() const;
        std::vector<uint8_t> rgba(size_t mip) const;

        std::vector<TextureMip> mips;
        std::vector<TextureMip> compressedMips;
        uint32_t revision = 1;
    };

    struct FontCharacter {
        int32_t u = 0;
        int32_t v = 0;
        int32_t width = 0;
        int32_t height = 0;
    };

    struct FontPage {
        UTexture* texture = nullptr;
        std::vector<FontCharacter> characters;
    };

    class UFont : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;
        const FontCharacter* glyph(uint8_t character, UTexture** texture) const;

        std::vector<FontPage> pages;
        uint32_t charactersPerPage = 256;
    };

    class USound : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        Name format;
        std::vector<uint8_t> bytes;
        float duration = -1;
        void* decoded = nullptr;
    };

    class UMusic : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        Name format;
        std::vector<uint8_t> bytes;
    };

    struct BspNode {
        Plane plane;
        uint64_t zoneMask = 0;
        uint8_t nodeFlags = 0;
        int32_t vertPool = 0;
        int32_t surf = 0;
        int32_t back = -1;
        int32_t front = -1;
        int32_t iPlane = -1;
        int32_t collisionBound = 0;
        int32_t renderBound = 0;
        uint8_t zone[2] = {};
        uint8_t numVertices = 0;
        int32_t leaf[2] = {};
    };

    struct BspSurf {
        UObject* texture = nullptr;
        uint32_t polyFlags = 0;
        int32_t pBase = 0;
        int32_t vNormal = 0;
        int32_t vTextureU = 0;
        int32_t vTextureV = 0;
        int32_t lightMap = -1;
        int32_t brushPoly = -1;
        int16_t panU = 0;
        int16_t panV = 0;
        UObject* actor = nullptr;
    };

    struct BspVert {
        int32_t vertex = 0;
        int32_t side = 0;
    };

    struct LightMapIndex {
        int32_t dataOffset = 0;
        Vector pan;
        int32_t uClamp = 0;
        int32_t vClamp = 0;
        float uScale = 0;
        float vScale = 0;
        int32_t lightActors = -1;
    };

    struct ZoneProperties {
        UObject* zoneActor = nullptr;
        uint64_t connectivity = 0;
        uint64_t visibility = 0;
        float lastRenderTime = 0;
    };

    struct BoundingBox {
        Vector min;
        Vector max;
        bool valid = false;
    };

    struct ConvexLeaf {
        int32_t zone = 0;
        int32_t permeating = 0;
        int32_t volumetric = 0;
        uint64_t visibleZones = 0;
    };

    struct Poly {
        Vector base;
        Vector normal;
        Vector textureU;
        Vector textureV;
        std::vector<Vector> vertices;
        uint32_t polyFlags = 0;
        UObject* actor = nullptr;
        UObject* texture = nullptr;
        Name itemName;
        int32_t link = -1;
        int32_t brushPoly = -1;
        int16_t panU = 0;
        int16_t panV = 0;
    };

    class UPolys : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        std::vector<Poly> polys;
    };

    class UPrimitive : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        BoundingBox bounds;
        Plane sphere;
    };

    class UModel : public UPrimitive {
    public:
        void serialize(Reader& reader, Linker& source) override;

        std::vector<Vector> vectors;
        std::vector<Vector> points;
        std::vector<BspNode> nodes;
        std::vector<BspSurf> surfs;
        std::vector<BspVert> verts;
        int32_t numSharedSides = 0;
        std::vector<ZoneProperties> zones;
        UPolys* polys = nullptr;
        std::vector<LightMapIndex> lightMaps;
        std::vector<uint8_t> lightBits;
        std::vector<BoundingBox> nodeBounds;
        std::vector<int32_t> leafHulls;
        std::vector<ConvexLeaf> leaves;
        std::vector<UObject*> lights;
        int32_t rootOutside = 0;
        int32_t linked = 0;
        uint32_t revision = 1;
    };

    struct ReachSpec {
        int32_t distance = 0;
        UObject* start = nullptr;
        UObject* end = nullptr;
        int32_t collisionRadius = 0;
        int32_t collisionHeight = 0;
        int32_t reachFlags = 0;
        uint8_t pruned = 0;
    };

    class ULevel : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        std::vector<UObject*> actors;
        std::string url;
        UModel* model = nullptr;
        std::vector<ReachSpec> reachSpecs;
        float timeSeconds = 0;
        UObject* textBlocks[16] = {};
        std::vector<std::pair<std::string, std::string>> travelInfo;
    };

    void registerEngineResources();
}
