#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <vector>

namespace dx {
    namespace bsp {
        enum class Optimization {
            Lame,
            Good,
            Optimal,
        };

        enum CsgOper : uint8_t {
            CsgActive = 0,
            CsgAdd = 1,
            CsgSubtract = 2,
            CsgIntersect = 3,
            CsgDeintersect = 4,
        };

        enum NodePlace {
            NodeBack = 0,
            NodeFront = 1,
            NodePlane = 2,
            NodeRoot = 3,
        };

        enum SplitResult {
            SplitCoplanar = 0,
            SplitFront = 1,
            SplitBack = 2,
            SplitSplit = 3,
        };

        constexpr uint32_t PF_Invisible = 0x00000001;
        constexpr uint32_t PF_Masked = 0x00000002;
        constexpr uint32_t PF_NotSolid = 0x00000008;
        constexpr uint32_t PF_Environment = 0x00000010;
        constexpr uint32_t PF_Semisolid = 0x00000020;
        constexpr uint32_t PF_FakeBackdrop = 0x00000080;
        constexpr uint32_t PF_TwoSided = 0x00000100;
        constexpr uint32_t PF_LowShadowDetail = 0x00008000;
        constexpr uint32_t PF_NoMerge = 0x00010000;
        constexpr uint32_t PF_BrightCorners = 0x00080000;
        constexpr uint32_t PF_SpecialLit = 0x00100000;
        constexpr uint32_t PF_Unlit = 0x00400000;
        constexpr uint32_t PF_HighShadowDetail = 0x00800000;
        constexpr uint32_t PF_Memorized = 0x01000000;
        constexpr uint32_t PF_Selected = 0x02000000;
        constexpr uint32_t PF_Portal = 0x04000000;
        constexpr uint32_t PF_EdProcessed = 0x40000000;
        constexpr uint32_t PF_EdCut = 0x80000000;
        constexpr uint32_t PF_AddLast = PF_Semisolid | PF_NotSolid;
        constexpr uint32_t PF_NoAddToBSP = PF_EdCut | PF_EdProcessed | PF_Selected | PF_Memorized;
        constexpr uint32_t PF_NoShadows = PF_Unlit | PF_Invisible | PF_Environment | PF_FakeBackdrop;

        constexpr uint8_t NF_NotCsg = 0x01;
        constexpr uint8_t NF_ShootThrough = 0x02;
        constexpr uint8_t NF_NotVisBlocking = 0x04;
        constexpr uint8_t NF_BrightCorners = 0x10;
        constexpr uint8_t NF_IsNew = 0x20;
        constexpr uint8_t NF_IsFront = 0x40;
        constexpr uint8_t NF_IsBack = 0x80;

        struct CoordSystem {
            Vector origin;
            Vector xAxis { 1, 0, 0 };
            Vector yAxis { 0, 1, 0 };
            Vector zAxis { 0, 0, 1 };
        };

        struct ModelCoords {
            CoordSystem pointXform;
            CoordSystem vectorXform;
        };

        struct Stats {
            size_t brushes = 0;
            size_t nodes = 0;
            size_t surfs = 0;
            size_t leaves = 0;
            size_t zones = 0;
            size_t portals = 0;
            size_t errors = 0;
        };

        void setCurrentTexture(UObject* texture);
        UObject* currentTexture();
        void ensureLevelPolys(UModel& model);
        int32_t vectorIndex(UModel& model, const Vector& v, bool normal);
        int32_t pointIndex(UModel& model, const Vector& v, bool exact);
        float buildCoords(UObject* brushActor, ModelCoords* coords, ModelCoords* uncoords);
        Vector transformVector(const Vector& v, const CoordSystem& coords);
        void transformPoly(Poly& poly, const ModelCoords& coords, const Vector& preSubtract, const Vector& postAdd, float orientation);
        void reversePoly(Poly& poly);
        int fixPoly(Poly& poly);
        bool calcNormal(Poly& poly);
        int finalizePoly(Poly& poly);
        bool removeColinears(Poly& poly);
        int splitWithPlane(const Poly& poly, const Vector& base, const Vector& normal, Poly* front, Poly* back, bool precise);
        int splitWithPlaneFast(const Poly& poly, const Plane& plane, Poly* front, Poly* back);

        void emptyModel(UModel& model, bool surfInfo, bool polys);
        void buildBound(UModel& model);
        int32_t addNode(UModel& model, int32_t parent, NodePlace place, uint8_t nodeFlags, Poly& poly);
        void build(UModel& model, Optimization opt, int balance, int rebuildSimplePolys);
        void buildFPolys(UModel& model, bool surfLinks, int32_t node);
        void mergeCoplanars(UModel& model, bool remapLinks, bool mergeDisparateTextures);
        int nodeToFPoly(const UModel& model, int32_t node, Poly& poly);
        int brushCSG(UObject* actor, UModel& model, uint32_t polyFlags, uint8_t oper, bool buildBounds);
        void cleanup(UModel& model);
        void refresh(UModel& model, bool noRemapSurfs);
        void optGeom(UModel& model);
        void buildBounds(UModel& model);
        void repartition(UModel& model, int32_t node, int simple);
        void prepMovingBrush(UObject* actor);
        void testVisibility(ULevel* level, UModel& model);
        bool isCsg(const BspNode& node, uint8_t extraFlags);
        int32_t pointLeafZone(const UModel& model, const Vector& point, int32_t* leaf);

        Stats csgRebuild(ULevel* level);
        Stats bspRebuild(ULevel* level, Optimization opt, int balance, bool zones, bool optimizeGeometry);
    }
}
