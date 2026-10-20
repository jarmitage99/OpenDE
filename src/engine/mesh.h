#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <vector>

namespace dx {
    struct AnimNotify {
        float time = 0;
        Name function;
    };

    struct AnimSequence {
        Name name;
        Name group;
        int32_t startFrame = 0;
        int32_t numFrames = 0;
        float rate = 0;
        std::vector<AnimNotify> notifys;
    };

    struct MeshTri {
        uint16_t vertex[3] = {};
        uint8_t uv[3][2] = {};
        uint32_t polyFlags = 0;
        int32_t textureIndex = 0;
    };

    struct MeshFace {
        uint16_t wedge[3] = {};
        uint16_t material = 0;
    };

    struct MeshWedge {
        uint16_t vertex = 0;
        uint8_t u = 0;
        uint8_t v = 0;
    };

    struct MeshMaterial {
        uint32_t polyFlags = 0;
        int32_t textureIndex = 0;
    };

    struct MeshTriangle {
        int32_t vertex[3] = {};
        float u[3] = {};
        float v[3] = {};
        uint32_t polyFlags = 0;
        int32_t textureIndex = 0;
    };

    class UMesh;

    struct MeshPoseCache {
        const UMesh* mesh = nullptr;
        Name sequence;
        float frame = 0;
        float tweenIndicator = 1;
        int32_t lodVerts = 0;
        std::vector<Vector> verts;
    };

    struct MeshAnimState {
        Name sequence;
        float frame = 0;
        Name blendSequences[4];
        float blendFrames[4] = {};
    };

    struct MeshPoseCaches {
        MeshPoseCache main;
        MeshPoseCache blends[4];
    };

    class UMesh : public UPrimitive {
    public:
        void serialize(Reader& reader, Linker& source) override;

        const AnimSequence* findSequence(Name sequence) const;
        virtual std::vector<MeshTriangle> triangles() const;
        void framePose(const AnimSequence* sequence, float animFrame, std::vector<Vector>& positions) const;
        void framePose(Name sequence, float animFrame, std::vector<Vector>& positions) const;
        int32_t tweenFrame(Name sequence, float animFrame, MeshPoseCache& cache, int32_t requested) const;
        int32_t animatedPose(const MeshAnimState& state, MeshPoseCaches& caches, std::vector<Vector>& positions, int32_t requested) const;

        std::vector<Vector> verts;
        std::vector<MeshTri> tris;
        std::vector<AnimSequence> sequences;
        BoundingBox meshBounds;
        Plane meshSphere;
        std::vector<int32_t> vertLinks;
        std::vector<UObject*> textures;
        std::vector<BoundingBox> frameBounds;
        std::vector<Plane> frameSpheres;
        int32_t frameVerts = 0;
        int32_t animFrames = 0;
        uint32_t andFlags = 0;
        uint32_t orFlags = 0;
        Vector scale;
        Vector origin;
        Rotator rotOrigin;
        std::vector<float> textureLod;
    };

    class ULodMesh : public UMesh {
    public:
        void serialize(Reader& reader, Linker& source) override;
        std::vector<MeshTriangle> triangles() const override;
        void lodTriangles(int32_t vertexSubset, bool morph, int32_t nonMorph, const std::vector<float>& alphas, std::vector<MeshTriangle>& result) const;
        int32_t poseVertex(int32_t wedgeVertex) const;

        std::vector<uint16_t> collapsePointThus;
        std::vector<uint16_t> faceLevel;
        std::vector<MeshFace> faces;
        std::vector<uint16_t> collapseWedgeThus;
        std::vector<MeshWedge> wedges;
        std::vector<MeshMaterial> materials;
        std::vector<MeshFace> specialFaces;
        uint32_t modelVerts = 0;
        uint32_t specialVerts = 0;
        float meshScaleMax = 0;
        float lodHysteresis = 0;
        float lodStrength = 0;
        uint32_t lodMinVerts = 0;
        float lodMorph = 0;
        float lodZDisplace = 0;
        std::vector<uint16_t> remapAnimVerts;
        uint32_t oldFrameVerts = 0;
    };

    struct FireSpark {
        uint8_t type = 0;
        uint8_t heat = 0;
        uint8_t x = 0;
        uint8_t y = 0;
        uint8_t a = 0;
        uint8_t b = 0;
        uint8_t c = 0;
        uint8_t d = 0;
    };

    class UFireTexture : public UTexture {
    public:
        void serialize(Reader& reader, Linker& source) override;

        std::vector<FireSpark> sparks;
    };

    void registerMeshResources();
}
