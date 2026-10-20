#pragma once

#include "engine/bsp.h"
#include "engine/resources.h"

#include <cstddef>
#include <cstdint>

namespace dx {
    namespace csg {
        constexpr uint8_t CsgActive = 0;
        constexpr uint8_t CsgAdd = 1;
        constexpr uint8_t CsgSubtract = 2;
        constexpr uint8_t CsgIntersect = 3;
        constexpr uint8_t CsgDeintersect = 4;

        constexpr uint32_t PolyInvisible = 0x00000001;
        constexpr uint32_t PolyTwoSided = 0x00000100;
        constexpr uint32_t PolyNotSolid = 0x00000008;
        constexpr uint32_t PolySemisolid = 0x00000020;
        constexpr uint32_t PolyPortal = 0x04000000;

        struct RebuildOptions {
            bool geometry = true;
            bool bsp = true;
            bsp::Optimization optimization = bsp::Optimization::Optimal;
            int balance = 15;
            bool zones = true;
            bool optimizeGeometry = true;
        };

        using Stats = bsp::Stats;

        void buildCube(UPolys& polys, float width, float depth, float height, UObject* texture);
        void buildCylinder(UPolys& polys, float radius, float height, int sides, UObject* texture);
        void buildSheet(UPolys& polys, float width, float height, UObject* texture);
        void updateBounds(UModel& model);
        UClass* nativeClass(const char* name);
        UObject* addBrush(ULevel* level, UObject* builder, uint8_t csgOper, uint32_t polyFlags = 0);
        UObject* addMover(ULevel* level, UObject* builder, UClass* moverClass);
        bool brushFromIntersection(ULevel* level, UObject* builder, bool deintersect);
        Stats rebuild(ULevel* level, const RebuildOptions& options = RebuildOptions());
        bool pointSolid(const UModel& model, const Vector& point);
        int32_t pointZone(const UModel& model, const Vector& point);
    }
}
