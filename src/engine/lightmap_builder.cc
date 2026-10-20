#include "engine/lightmap_builder.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

namespace dx {
    namespace lightbuild {
        namespace {
            constexpr uint8_t NodeNotVisBlocking = 0x04;
            constexpr uint8_t NodeBrightCorners = 0x10;
            constexpr uint32_t PolyLowShadowDetail = 0x00008000;
            constexpr uint32_t PolyHighShadowDetail = 0x00800000;
            constexpr uint32_t PolyBrightCorners = 0x00080000;
            constexpr uint32_t PolySpecialLit = 0x00100000;
            constexpr uint32_t PolyTwoSided = 0x00000100;
            constexpr uint32_t PolyPortal = 0x04000000;
            constexpr uint32_t PolyNotSolid = 0x00000008;
            constexpr uint32_t PolyInvisible = 0x00000001;
            constexpr uint32_t PolyMasked = 0x00000002;
            constexpr uint32_t PolyNoAddToBsp = 0x80000000 | 0x40000000 | 0x02000000 | 0x01000000;
            constexpr uint32_t PolyNoShadows = 0x00400000 | 0x00000001 | 0x00000010 | 0x00000080;

            bool outOfCorner = false;

            float planeDistance(const BspNode& node, const Vector& p)
            {
                return node.plane.x * p.x + node.plane.y * p.y + node.plane.z * p.z - node.plane.w;
            }

            bool childOutside(const BspNode& node, int child, bool outside, uint8_t extraFlags)
            {
                return child ? (outside || bsp::isCsg(node, extraFlags)) : (outside && !bsp::isCsg(node, extraFlags));
            }

            bool lineCheck(const UModel& model, int32_t index, Vector end, Vector start, bool outside, uint8_t nodeFlags)
            {
                uint8_t csgFlags = nodeFlags & static_cast<uint8_t>(~NodeBrightCorners);
                while (index >= 0) {
                    const BspNode& node = model.nodes[static_cast<size_t>(index)];
                    float dist1 = planeDistance(node, start);
                    float dist2 = planeDistance(node, end);
                    if (dist1 > -0.001f && dist2 > -0.001f) {
                        outside = outside || bsp::isCsg(node, csgFlags);
                        index = node.front;
                    } else if (dist1 < 0.001f && dist2 < 0.001f) {
                        outside = outside && !bsp::isCsg(node, csgFlags);
                        index = node.back;
                    } else {
                        Vector middle = start + (start - end) * (dist1 / (dist2 - dist1));
                        int frontFirst = dist1 > 0.0f ? 1 : 0;
                        int32_t first = frontFirst ? node.front : node.back;
                        if (!lineCheck(model, first, middle, start, childOutside(node, frontFirst, outside, csgFlags), nodeFlags)) {
                            return false;
                        }
                        outside = childOutside(node, 1 - frontFirst, outside, csgFlags);
                        index = frontFirst ? node.back : node.front;
                        start = middle;
                    }
                }
                if (!outside) {
                    if (outOfCorner || !(nodeFlags & NodeBrightCorners)) {
                        return false;
                    }
                    return true;
                }
                outOfCorner = true;
                return true;
            }

            bool modelLineCheck(const UModel& model, const Vector& end, const Vector& start, uint8_t nodeFlags)
            {
                if (model.nodes.empty()) {
                    return true;
                }
                outOfCorner = false;
                return lineCheck(model, 0, end, start, model.rootOutside != 0, nodeFlags);
            }

            bool sceneLight(UObject* a)
            {
                return a && !a->destroyed && a->data && actor::LightType(a) != 0 && (actor::bStatic(a) || actor::bNoDelete(a));
            }

            float worldLightRadius(UObject* light)
            {
                return 25.0f * (static_cast<float>(actor::LightRadius(light)) + 1.0f);
            }

            int32_t appFloor(float value)
            {
                return static_cast<int32_t>(std::floor(value));
            }

            void setupIndex(LightMapIndex& index, uint32_t polyFlags, float minU, float minV, float maxU, float maxV)
            {
                float scale = 32.0f;
                if ((polyFlags & (PolyHighShadowDetail | PolyLowShadowDetail)) == (PolyHighShadowDetail | PolyLowShadowDetail)) {
                    scale = 128.0f;
                } else if (polyFlags & PolyHighShadowDetail) {
                    scale = 16.0f;
                } else if (polyFlags & PolyLowShadowDetail) {
                    scale = 64.0f;
                }
                index.uScale = scale;
                index.vScale = scale;
                index.dataOffset = 0;
                index.pan.z = 0.0f;
                for (;;) {
                    float extent = maxU - minU;
                    index.uClamp = std::max(2, 1 + appFloor(static_cast<float>((static_cast<double>(extent) - 0.25) / static_cast<double>(index.uScale))));
                    float uScale = static_cast<float>((static_cast<double>(extent) + 0.25) / static_cast<double>(index.uClamp - 1));
                    index.pan.x = static_cast<float>(static_cast<double>(minU) - 0.125);
                    if (index.uClamp <= 256) {
                        index.uScale = uScale;
                        break;
                    }
                    index.uScale *= 2.0f;
                }
                for (;;) {
                    float extent = maxV - minV;
                    index.vClamp = std::max(2, 1 + appFloor(static_cast<float>((static_cast<double>(extent) - 0.25) / static_cast<double>(index.vScale))));
                    float vScale = static_cast<float>((static_cast<double>(extent) + 0.25) / static_cast<double>(index.vClamp - 1));
                    index.pan.y = static_cast<float>(static_cast<double>(minV) - 0.125);
                    if (index.vClamp <= 256) {
                        index.vScale = vScale;
                        break;
                    }
                    index.vScale *= 2.0f;
                }
            }

            void buildSurfList(UModel& model, int32_t nodeIndex)
            {
                const BspNode& node = model.nodes[static_cast<size_t>(nodeIndex)];
                BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
                if (node.numVertices) {
                    if (!(surf.polyFlags & PolyNoShadows)) {
                        if (surf.lightMap == -1) {
                            surf.lightMap = static_cast<int32_t>(model.lightMaps.size());
                            model.lightMaps.push_back(LightMapIndex());
                        }
                    }
                }
                int32_t children[3] = { node.front, node.back, node.iPlane };
                for (int32_t child : children) {
                    if (child >= 0) {
                        buildSurfList(model, child);
                    }
                }
            }

            struct DynamicSurf {
                UObject* mover = nullptr;
                int32_t poly = -1;
                int32_t surf = -1;
            };

            struct TraceWorld {
                UModel model;
                std::vector<DynamicSurf> dynamicSurfs;
                Vector polyNormal;
                int32_t addSurf = -1;

                void addFragment(int32_t parent, int32_t coplanarParent, int isFront, const Poly& poly)
                {
                    int zoneFront = isFront;
                    int32_t zoneParentIndex = parent;
                    bool different = false;
                    if (coplanarParent >= 0) {
                        different = parent != coplanarParent;
                        parent = coplanarParent;
                        isFront = 2;
                    }
                    int32_t firstParent = parent;
                    if (isFront == 2) {
                        while (model.nodes[static_cast<size_t>(parent)].iPlane >= 0) {
                            parent = model.nodes[static_cast<size_t>(parent)].iPlane;
                        }
                    }
                    BspNode node;
                    node.surf = addSurf;
                    node.collisionBound = -1;
                    node.renderBound = -1;
                    node.zoneMask = model.nodes[static_cast<size_t>(parent)].zoneMask;
                    node.numVertices = static_cast<uint8_t>(poly.vertices.size());
                    node.nodeFlags = 0x20;
                    if (poly.polyFlags & PolyNotSolid) {
                        node.nodeFlags |= 0x01;
                    }
                    if (poly.polyFlags & (PolyInvisible | PolyPortal)) {
                        node.nodeFlags |= NodeNotVisBlocking;
                    }
                    if (poly.polyFlags & PolyMasked) {
                        node.nodeFlags |= 0x02;
                    }
                    node.plane = { poly.normal.x, poly.normal.y, poly.normal.z, poly.base.dot(poly.normal) };
                    const BspNode& parentNode = model.nodes[static_cast<size_t>(parent)];
                    const BspNode& zoneParent = model.nodes[static_cast<size_t>(zoneParentIndex)];
                    if (coplanarParent < 0 || different) {
                        node.zone[0] = zoneParent.zone[zoneFront];
                        node.zone[1] = zoneParent.zone[zoneFront];
                        node.leaf[0] = zoneParent.leaf[zoneFront];
                        node.leaf[1] = zoneParent.leaf[zoneFront];
                    } else {
                        int flipped = node.plane.x * parentNode.plane.x + node.plane.y * parentNode.plane.y + node.plane.z * parentNode.plane.z < 0.0f ? 1 : 0;
                        node.zone[0] = parentNode.zone[flipped];
                        node.leaf[0] = parentNode.leaf[flipped];
                        node.zone[1] = parentNode.zone[1 - flipped];
                        node.leaf[1] = parentNode.leaf[1 - flipped];
                    }
                    model.nodes[static_cast<size_t>(firstParent)].zoneMask |= (1ull << (node.zone[0] & 63)) | (1ull << (node.zone[1] & 63));
                    node.front = -1;
                    node.back = -1;
                    node.iPlane = -1;
                    node.vertPool = static_cast<int32_t>(model.verts.size());
                    for (const Vector& v : poly.vertices) {
                        BspVert vert;
                        vert.side = -1;
                        vert.vertex = static_cast<int32_t>(model.points.size());
                        model.points.push_back(v);
                        model.verts.push_back(vert);
                    }
                    int32_t index = static_cast<int32_t>(model.nodes.size());
                    model.nodes.push_back(node);
                    if (isFront == 2) {
                        model.nodes[static_cast<size_t>(parent)].iPlane = index;
                    } else if (isFront == 1) {
                        model.nodes[static_cast<size_t>(parent)].front = index;
                    } else {
                        model.nodes[static_cast<size_t>(parent)].back = index;
                    }
                }

                void filterPoly(int32_t iNode, int32_t coplanarParent, Poly poly, bool outside)
                {
                    for (;;) {
                        if (static_cast<int>(poly.vertices.size()) >= 14) {
                            Poly half = poly;
                            int n = static_cast<int>(poly.vertices.size());
                            int m = n / 2;
                            half.vertices.assign(poly.vertices.begin() + m, poly.vertices.end());
                            half.vertices.push_back(poly.vertices[0]);
                            poly.vertices.resize(static_cast<size_t>(m + 1));
                            filterPoly(iNode, coplanarParent, half, outside);
                        }
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        const BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
                        Vector base = model.points[static_cast<size_t>(surf.pBase)];
                        Vector normal = model.vectors[static_cast<size_t>(surf.vNormal)];
                        Poly front;
                        Poly back;
                        int split = bsp::splitWithPlaneFast(poly, { normal.x, normal.y, normal.z, base.dot(normal) }, &front, &back);
                        if (split == bsp::SplitCoplanar) {
                            if (normal.dot(polyNormal) >= 0.0f) {
                                coplanarParent = iNode;
                            }
                            split = bsp::SplitFront;
                        }
                        if (split == bsp::SplitFront) {
                            outside = outside || bsp::isCsg(node, 0);
                            if (node.front >= 0) {
                                iNode = node.front;
                                continue;
                            }
                            if (outside) {
                                addFragment(iNode, coplanarParent, 1, poly);
                            }
                            return;
                        }
                        if (split == bsp::SplitSplit) {
                            bool frontOutside = outside || bsp::isCsg(node, 0);
                            if (node.front >= 0) {
                                filterPoly(node.front, coplanarParent, front, frontOutside);
                            } else if (frontOutside) {
                                addFragment(iNode, coplanarParent, 1, front);
                            }
                            poly = back;
                        }
                        const BspNode& current = model.nodes[static_cast<size_t>(iNode)];
                        outside = outside && !bsp::isCsg(current, 0);
                        if (current.back >= 0) {
                            iNode = current.back;
                            continue;
                        }
                        if (outside) {
                            addFragment(iNode, coplanarParent, 0, poly);
                        }
                        return;
                    }
                }

                void addMover(UObject* mover, bool addNodes)
                {
                    UModel* brush = cast<UModel>(actor::Brush(mover));
                    if (!brush || !brush->polys) {
                        return;
                    }
                    bsp::ModelCoords coords;
                    float orientation = bsp::buildCoords(mover, &coords, nullptr);
                    for (size_t i = 0; i < brush->polys->polys.size(); i++) {
                        Poly poly = brush->polys->polys[i];
                        bsp::transformPoly(poly, coords, actor::PrePivot(mover), actor::Location(mover), orientation);
                        BspSurf surf;
                        surf.pBase = static_cast<int32_t>(model.points.size());
                        model.points.push_back(poly.base);
                        surf.vNormal = static_cast<int32_t>(model.vectors.size());
                        model.vectors.push_back(poly.normal);
                        surf.vTextureU = static_cast<int32_t>(model.vectors.size());
                        model.vectors.push_back(poly.textureU);
                        surf.vTextureV = static_cast<int32_t>(model.vectors.size());
                        model.vectors.push_back(poly.textureV);
                        surf.lightMap = brush->polys->polys[i].brushPoly;
                        surf.texture = poly.texture;
                        surf.panU = poly.panU;
                        surf.panV = poly.panV;
                        surf.brushPoly = static_cast<int32_t>(i);
                        surf.actor = mover;
                        surf.polyFlags = poly.polyFlags & ~PolyNoAddToBsp;
                        if (actor::bSpecialLit(mover)) {
                            surf.polyFlags |= PolySpecialLit;
                        }
                        int32_t surfIndex = static_cast<int32_t>(model.surfs.size());
                        model.surfs.push_back(surf);
                        dynamicSurfs.push_back({ mover, static_cast<int32_t>(i), surfIndex });
                        if (addNodes && !model.nodes.empty() && poly.vertices.size() >= 3) {
                            polyNormal = poly.normal;
                            addSurf = surfIndex;
                            filterPoly(0, -1, poly, true);
                        }
                    }
                    for (BspNode& node : model.nodes) {
                        node.nodeFlags &= static_cast<uint8_t>(~0x20);
                    }
                }
            };

            void placeMover(UObject* mover, uint8_t key)
            {
                int index = std::clamp<int>(key, 0, 7);
                actor::Location(mover) = mover::BasePos(mover) + mover::KeyPos(mover, index);
                Rotator base = mover::BaseRot(mover);
                Rotator keyRotation = mover::KeyRot(mover, index);
                actor::Rotation(mover) = { base.pitch + keyRotation.pitch, base.yaw + keyRotation.yaw, base.roll + keyRotation.roll };
            }

            bool sampleVisible(const UModel& traceModel, const Vector& light, const Vector& point)
            {
                return modelLineCheck(traceModel, light, point, NodeNotVisBlocking);
            }

            bool surfVisible(const UModel& traceModel, int32_t surfIndex, const Vector& light)
            {
                const BspSurf& surf = traceModel.surfs[static_cast<size_t>(surfIndex)];
                Vector normal = traceModel.vectors[static_cast<size_t>(surf.vNormal)];
                for (const BspNode& node : traceModel.nodes) {
                    if (node.surf != surfIndex || node.numVertices < 3) {
                        continue;
                    }
                    Vector center;
                    for (int i = 0; i < node.numVertices; i++) {
                        center += traceModel.points[static_cast<size_t>(traceModel.verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                    }
                    center = center / static_cast<float>(node.numVertices);
                    float side = (light - center).dot(normal) >= 0.0f ? 1.0f : -1.0f;
                    if (sampleVisible(traceModel, light, center + normal * side)) {
                        return true;
                    }
                    for (int i = 0; i < node.numVertices; i++) {
                        Vector v = traceModel.points[static_cast<size_t>(traceModel.verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                        Vector sample = v + (center - v) * 0.05f + normal * side;
                        if (sampleVisible(traceModel, light, sample)) {
                            return true;
                        }
                    }
                }
                return false;
            }

            void raytrace(const UModel& traceModel, UModel& target, LightMapIndex& index, uint32_t polyFlags, const Vector& base, const Vector& normal, const Vector& textureU, const Vector& textureV, const std::vector<UObject*>& lights, Stats& stats)
            {
                Vector newBase = base + normal * 4.0f;
                int32_t byteSize = ((index.uClamp + 7) >> 3) * index.vClamp;
                float determinant = textureU.dot(textureV.cross(normal));
                Vector axisX = textureV.cross(normal) / determinant;
                Vector axisY = normal.cross(textureU) / determinant;
                index.dataOffset = static_cast<int32_t>(target.lightBits.size());
                index.lightActors = static_cast<int32_t>(target.lights.size());
                std::vector<uint8_t> data(static_cast<size_t>(byteSize));
                for (UObject* light : lights) {
                    bool didHit = false;
                    Vector location = actor::Location(light);
                    float radius = worldLightRadius(light);
                    float sqRadius = radius * radius;
                    float uScale = index.uScale;
                    float vScale = index.vScale;
                    Vector vertex0 = newBase + axisX * index.pan.x + axisY * index.pan.y;
                    uint8_t nodeFlags = NodeNotVisBlocking;
                    if (polyFlags & PolyBrightCorners) {
                        uScale -= 0.5f / static_cast<float>(index.uClamp - 1);
                        vScale -= 0.5f / static_cast<float>(index.vClamp - 1);
                        vertex0 += axisX * 0.25f + axisY * 0.25f;
                        nodeFlags |= NodeBrightCorners;
                    }
                    Vector vertexDU = axisX * uScale;
                    Vector vertexDV = axisY * vScale;
                    size_t count = 0;
                    for (int32_t v = 0; v < index.vClamp; v++) {
                        Vector vertex = vertex0;
                        for (int32_t u = 0; u < index.uClamp; u += 8) {
                            uint8_t bits = 0;
                            uint8_t mask = 1;
                            bool previous = false;
                            for (int32_t subU = u; subU < index.uClamp && mask; subU++, mask = static_cast<uint8_t>(mask << 1)) {
                                if ((location - vertex).sizeSquared() < sqRadius) {
                                    previous = modelLineCheck(traceModel, location, vertex, nodeFlags);
                                    if (previous) {
                                        bits |= mask;
                                        didHit = true;
                                    }
                                }
                                vertex += vertexDU;
                            }
                            if (previous) {
                                for (; mask; mask = static_cast<uint8_t>(mask << 1)) {
                                    bits |= mask;
                                }
                            }
                            data[count++] = bits;
                        }
                        vertex0 += vertexDV;
                    }
                    if (didHit) {
                        target.lights.push_back(light);
                        target.lightBits.insert(target.lightBits.end(), data.begin(), data.end());
                        stats.lightLinks++;
                    }
                }
                target.lights.push_back(nullptr);
            }
        }

        Stats rebuild(ULevel* level, const std::unordered_set<UObject*>* selected)
        {
            Stats stats;
            if (!level || !level->model) {
                return stats;
            }
            UModel& model = *level->model;
            for (UObject* a : level->actors) {
                if (a) {
                    objects::ensureLoaded(a);
                }
            }
            objects::flushLoads();
            if (model.nodes.empty()) {
                return stats;
            }
            UClass* moverClass = objects::findClass("Engine.Mover");
            std::vector<UObject*> movers;
            for (UObject* a : level->actors) {
                if (a && !a->destroyed && a->data && moverClass && a->isA(moverClass) && actor::Brush(a)) {
                    movers.push_back(a);
                }
            }
            model.lightMaps.clear();
            model.lightBits.clear();
            for (BspSurf& surf : model.surfs) {
                surf.lightMap = -1;
            }
            for (UObject* mover : movers) {
                if (mover::WorldRaytraceKey(mover) != 255) {
                    mover::WorldRaytraceKey(mover) = static_cast<uint8_t>(std::clamp<int>(mover::WorldRaytraceKey(mover), 0, 7));
                    placeMover(mover, mover::WorldRaytraceKey(mover));
                }
                UModel* brush = cast<UModel>(actor::Brush(mover));
                objects::ensureLoaded(brush);
                objects::flushLoads();
                if (brush && brush->polys) {
                    objects::ensureLoaded(brush->polys);
                    objects::flushLoads();
                    brush->lightMaps.clear();
                    for (Poly& poly : brush->polys->polys) {
                        if (!(poly.polyFlags & PolyNoShadows)) {
                            poly.brushPoly = static_cast<int32_t>(brush->lightMaps.size());
                            LightMapIndex index;
                            index.lightActors = -1;
                            brush->lightMaps.push_back(index);
                        } else {
                            poly.brushPoly = -1;
                        }
                    }
                }
            }
            TraceWorld world;
            world.model.nodes = model.nodes;
            world.model.surfs = model.surfs;
            world.model.verts = model.verts;
            world.model.points = model.points;
            world.model.vectors = model.vectors;
            world.model.rootOutside = model.rootOutside;
            for (UObject* mover : movers) {
                world.addMover(mover, true);
            }
            buildSurfList(model, 0);
            for (size_t i = 0; i < model.surfs.size(); i++) {
                world.model.surfs[i].lightMap = model.surfs[i].lightMap;
            }
            std::vector<std::vector<UObject*>> surfLights(world.model.surfs.size());
            UObject* levelInfo = level->actors.empty() ? nullptr : level->actors[0];
            for (UObject* a : level->actors) {
                if (a && a->data) {
                    actor::bLightChanged(a) = 0;
                }
                if (!sceneLight(a)) {
                    continue;
                }
                bool doLight = !selected || selected->count(a);
                UObject* zone = region(a).zone;
                if (!doLight && zone && zone != levelInfo) {
                    doLight = selected->count(zone) != 0;
                }
                if (!doLight) {
                    continue;
                }
                actor::bNoDelete(a) = 1;
                stats.lights++;
                Vector location = actor::Location(a);
                float radius = worldLightRadius(a);
                bool specialLit = actor::bSpecialLit(a) != 0;
                for (size_t i = 0; i < world.model.surfs.size(); i++) {
                    const BspSurf& surf = world.model.surfs[i];
                    if (surf.lightMap == -1) {
                        continue;
                    }
                    if (specialLit != ((surf.polyFlags & PolySpecialLit) != 0)) {
                        continue;
                    }
                    Vector normal = world.model.vectors[static_cast<size_t>(surf.vNormal)];
                    float planeDot = (location - world.model.points[static_cast<size_t>(surf.pBase)]).dot(normal);
                    if (std::abs(planeDot) > radius) {
                        continue;
                    }
                    if (planeDot < -1.0f && !(surf.polyFlags & (PolyTwoSided | PolyPortal))) {
                        continue;
                    }
                    if (!surfVisible(world.model, static_cast<int32_t>(i), location)) {
                        continue;
                    }
                    surfLights[i].push_back(a);
                }
            }
            for (LightMapIndex& index : model.lightMaps) {
                index.lightActors = -1;
            }
            for (size_t surfIndex = 0; surfIndex < model.surfs.size(); surfIndex++) {
                BspSurf& surf = model.surfs[surfIndex];
                if (surf.lightMap == -1) {
                    continue;
                }
                LightMapIndex& index = model.lightMaps[static_cast<size_t>(surf.lightMap)];
                Vector base = model.points[static_cast<size_t>(surf.pBase)];
                Vector normal = model.vectors[static_cast<size_t>(surf.vNormal)];
                Vector textureU = model.vectors[static_cast<size_t>(surf.vTextureU)];
                Vector textureV = model.vectors[static_cast<size_t>(surf.vTextureV)];
                float minU = 10000000.0f;
                float minV = 10000000.0f;
                float maxU = -10000000.0f;
                float maxV = -10000000.0f;
                for (const BspNode& node : model.nodes) {
                    if (node.surf != static_cast<int32_t>(surfIndex) || node.numVertices == 0) {
                        continue;
                    }
                    for (int32_t i = 0; i < node.numVertices; i++) {
                        Vector vertex = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)] - base;
                        float u = vertex.dot(textureU);
                        float v = vertex.dot(textureV);
                        minU = std::min(u, minU);
                        maxU = std::max(u, maxU);
                        minV = std::min(v, minV);
                        maxV = std::max(v, maxV);
                    }
                }
                setupIndex(index, surf.polyFlags, minU, minV, maxU, maxV);
                index.lightActors = -1;
                stats.surfaces++;
                stats.texels += static_cast<size_t>(index.uClamp * index.vClamp);
                if (!surfLights[surfIndex].empty()) {
                    raytrace(world.model, model, index, surf.polyFlags, base, normal, textureU, textureV, surfLights[surfIndex], stats);
                }
            }
            for (UObject* mover : movers) {
                UModel* brush = cast<UModel>(actor::Brush(mover));
                if (!brush || !brush->polys) {
                    continue;
                }
                uint8_t key = static_cast<uint8_t>(std::clamp<int>(mover::BrushRaytraceKey(mover), 0, 7));
                mover::BrushRaytraceKey(mover) = key;
                placeMover(mover, key);
                TraceWorld moverWorld;
                moverWorld.model.nodes = model.nodes;
                moverWorld.model.surfs = model.surfs;
                moverWorld.model.verts = model.verts;
                moverWorld.model.points = model.points;
                moverWorld.model.vectors = model.vectors;
                moverWorld.model.rootOutside = model.rootOutside;
                for (UObject* other : movers) {
                    moverWorld.addMover(other, other != mover);
                }
                bool dynamicLightMover = mover::bDynamicLightMover(mover) != 0;
                for (const DynamicSurf& dynamicSurf : moverWorld.dynamicSurfs) {
                    if (dynamicSurf.mover != mover) {
                        continue;
                    }
                    Poly& poly = brush->polys->polys[static_cast<size_t>(dynamicSurf.poly)];
                    if (poly.brushPoly == -1) {
                        continue;
                    }
                    LightMapIndex& index = brush->lightMaps[static_cast<size_t>(poly.brushPoly)];
                    float minU = 10000000.0f;
                    float minV = 10000000.0f;
                    float maxU = -10000000.0f;
                    float maxV = -10000000.0f;
                    for (const Vector& v : poly.vertices) {
                        Vector vertex = v - poly.base;
                        float u = vertex.dot(poly.textureU);
                        float w = vertex.dot(poly.textureV);
                        minU = std::min(u, minU);
                        maxU = std::max(u, maxU);
                        minV = std::min(w, minV);
                        maxV = std::max(w, maxV);
                    }
                    setupIndex(index, poly.polyFlags | (dynamicLightMover ? PolyLowShadowDetail : 0), minU, minV, maxU, maxV);
                    stats.surfaces++;
                    if (dynamicLightMover) {
                        continue;
                    }
                    size_t worldSurf = 0;
                    for (const DynamicSurf& candidate : world.dynamicSurfs) {
                        if (candidate.mover == mover && candidate.poly == dynamicSurf.poly) {
                            worldSurf = static_cast<size_t>(candidate.surf);
                        }
                    }
                    const std::vector<UObject*>& lights = worldSurf < surfLights.size() ? surfLights[worldSurf] : std::vector<UObject*>();
                    if (lights.empty()) {
                        continue;
                    }
                    const BspSurf& surf = moverWorld.model.surfs[static_cast<size_t>(dynamicSurf.surf)];
                    raytrace(moverWorld.model, *brush, index, surf.polyFlags, moverWorld.model.points[static_cast<size_t>(surf.pBase)], moverWorld.model.vectors[static_cast<size_t>(surf.vNormal)], moverWorld.model.vectors[static_cast<size_t>(surf.vTextureU)], moverWorld.model.vectors[static_cast<size_t>(surf.vTextureV)], lights, stats);
                }
            }
            for (UObject* a : level->actors) {
                if (a && a->data) {
                    actor::bDynamicLight(a) = 0;
                }
            }
            for (UObject* mover : movers) {
                placeMover(mover, mover::KeyNum(mover));
                mover::SavedPos(mover) = {};
                mover::SavedRot(mover) = {};
            }
            return stats;
        }
    }
}
