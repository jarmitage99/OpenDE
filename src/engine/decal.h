#pragma once

#include "core/math.h"
#include "core/object.h"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace dx {
    class UModel;
    class World;

    struct DecalPiece {
        int32_t surf = -1;
        std::array<Vector, 4> vertices;
        std::vector<int32_t> nodes;
    };

    struct DecalRegistry {
        UModel* model = nullptr;
        std::vector<std::vector<int32_t>> surfNodes;
        std::unordered_map<UObject*, std::vector<DecalPiece>> pieces;
    };

    int32_t decalFindSurf(UModel* model, int32_t node, const Vector& point);
    void clipDecalPolygon(UModel* model, int32_t node, std::vector<Vector>& polygon);
    void clipDecalToNode(UModel* model, int32_t node, std::vector<Vector>& polygon);
    void clipDecalSide(UModel* model, int32_t node, std::vector<Vector>& polygon, float inside);
    bool attachDecal(World& world, UObject* decal, float traceDistance, Vector decalDir);
    void detachDecal(World& world, UObject* decal);
}
