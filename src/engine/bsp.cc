#include "engine/bsp.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>

namespace dx {
    namespace bsp {
        namespace {
            constexpr float ThreshPointsAreSame = 0.002f;
            constexpr float ThreshPointsAreNear = 0.015f;
            constexpr float ThreshNormalsAreSame = 0.00002f;
            constexpr float ThreshVectorsAreNear = 0.0004f;
            constexpr float ThreshSplitPolyWithPlane = 0.25f;
            constexpr float ThreshSplitPolyPrecisely = 0.01f;
            constexpr float ThreshZeroNormSquared = 0.01f * 0.01f;
            constexpr float FloatNormalThresh = 0.0001f;
            constexpr float ThreshOptGeomCoplanar = 0.25f;
            constexpr float ThreshOptGeomCosidal = 0.25f;
            constexpr double SmallNumber = 1.e-8;
            constexpr int MaxVertices = 16;
            constexpr int VertexThreshold = MaxVertices - 2;
            constexpr int MaxNodeVertices = 16;
            constexpr float WorldMax = 65536.0f;

            UObject* currentTextureObject = nullptr;
            bool fastRebuild = false;
            size_t errors = 0;

            const std::array<float, 16384>& trig()
            {
                static std::array<float, 16384> table = [] {
                    std::array<float, 16384> values {};
                    for (int i = 0; i < 16384; i++) {
                        values[static_cast<size_t>(i)] = static_cast<float>(std::sin(static_cast<double>(static_cast<float>(i)) * 2.0 * 3.141592654 / 16384.0));
                    }
                    return values;
                }();
                return table;
            }

            float sinTab(int32_t i)
            {
                return trig()[static_cast<size_t>((i >> 2) & 16383)];
            }

            float cosTab(int32_t i)
            {
                return trig()[static_cast<size_t>(((i + 16384) >> 2) & 16383)];
            }

            Vector safeNormal(const Vector& v)
            {
                float squareSum = v.x * v.x + v.y * v.y + v.z * v.z;
                if (static_cast<double>(squareSum) < SmallNumber) {
                    return {};
                }
                float size = std::sqrt(squareSum);
                float scale = static_cast<float>(1.0 / static_cast<double>(size));
                return { v.x * scale, v.y * scale, v.z * scale };
            }

            bool normalize(Vector& v)
            {
                float squareSum = v.x * v.x + v.y * v.y + v.z * v.z;
                if (static_cast<double>(squareSum) >= SmallNumber) {
                    float scale = static_cast<float>(1.0 / std::sqrt(static_cast<double>(squareSum)));
                    v.x *= scale;
                    v.y *= scale;
                    v.z *= scale;
                    return true;
                }
                return false;
            }

            bool pointsAreSame(const Vector& p, const Vector& q)
            {
                float temp = p.x - q.x;
                if (temp > -ThreshPointsAreSame && temp < ThreshPointsAreSame) {
                    temp = p.y - q.y;
                    if (temp > -ThreshPointsAreSame && temp < ThreshPointsAreSame) {
                        temp = p.z - q.z;
                        if (temp > -ThreshPointsAreSame && temp < ThreshPointsAreSame) {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool pointsAreNear(const Vector& a, const Vector& b, float dist)
            {
                if (std::abs(a.x - b.x) >= dist) {
                    return false;
                }
                if (std::abs(a.y - b.y) >= dist) {
                    return false;
                }
                if (std::abs(a.z - b.z) >= dist) {
                    return false;
                }
                return true;
            }

            float pointPlaneDist(const Vector& point, const Vector& base, const Vector& normal)
            {
                return (point - base).dot(normal);
            }

            Vector linePlaneIntersection(const Vector& p1, const Vector& p2, const Vector& origin, const Vector& normal)
            {
                return p1 + (p2 - p1) * ((origin - p1).dot(normal) / (p2 - p1).dot(normal));
            }

            Vector linePlaneIntersection(const Vector& p1, const Vector& p2, const Plane& plane)
            {
                Vector n { plane.x, plane.y, plane.z };
                return p1 + (p2 - p1) * ((plane.w - p1.dot(n)) / (p2 - p1).dot(n));
            }

            Plane makePlane(const Vector& base, const Vector& normal)
            {
                return { normal.x, normal.y, normal.z, base.dot(normal) };
            }

            Plane makePlane(const Vector& a, const Vector& b, const Vector& c)
            {
                Vector n = safeNormal((b - a).cross(c - a));
                return { n.x, n.y, n.z, a.dot(n) };
            }

            float planeDot(const Plane& plane, const Vector& p)
            {
                return plane.x * p.x + plane.y * p.y + plane.z * p.z - plane.w;
            }

            Vector transformPointBy(const Vector& v, const CoordSystem& c)
            {
                Vector t = v - c.origin;
                return { t.dot(c.xAxis), t.dot(c.yAxis), t.dot(c.zAxis) };
            }

            CoordSystem& mulCoords(CoordSystem& self, const CoordSystem& t)
            {
                self.origin = transformPointBy(self.origin, t);
                self.xAxis = transformVector(self.xAxis, t);
                self.yAxis = transformVector(self.yAxis, t);
                self.zAxis = transformVector(self.zAxis, t);
                return self;
            }

            CoordSystem axes(const Vector& x, const Vector& y, const Vector& z)
            {
                CoordSystem c;
                c.xAxis = x;
                c.yAxis = y;
                c.zAxis = z;
                return c;
            }

            CoordSystem& mulRotator(CoordSystem& self, const Rotator& rot)
            {
                mulCoords(self, axes({ cosTab(rot.yaw), sinTab(rot.yaw), 0.0f }, { -sinTab(rot.yaw), cosTab(rot.yaw), 0.0f }, { 0.0f, 0.0f, 1.0f }));
                mulCoords(self, axes({ cosTab(rot.pitch), 0.0f, sinTab(rot.pitch) }, { 0.0f, 1.0f, 0.0f }, { -sinTab(rot.pitch), 0.0f, cosTab(rot.pitch) }));
                mulCoords(self, axes({ 1.0f, 0.0f, 0.0f }, { 0.0f, cosTab(rot.roll), -sinTab(rot.roll) }, { 0.0f, sinTab(rot.roll), cosTab(rot.roll) }));
                return self;
            }

            CoordSystem& divRotator(CoordSystem& self, const Rotator& rot)
            {
                mulCoords(self, axes({ 1.0f, -0.0f, 0.0f }, { -0.0f, cosTab(rot.roll), sinTab(rot.roll) }, { 0.0f, -sinTab(rot.roll), cosTab(rot.roll) }));
                mulCoords(self, axes({ cosTab(rot.pitch), 0.0f, -sinTab(rot.pitch) }, { 0.0f, 1.0f, -0.0f }, { sinTab(rot.pitch), 0.0f, cosTab(rot.pitch) }));
                mulCoords(self, axes({ cosTab(rot.yaw), -sinTab(rot.yaw), -0.0f }, { sinTab(rot.yaw), cosTab(rot.yaw), 0.0f }, { -0.0f, 0.0f, 1.0f }));
                return self;
            }

            struct Scale {
                Vector scale;
                float sheerRate = 0;
                uint8_t sheerAxis = 0;
            };

            Scale readScale(uint8_t* data)
            {
                Scale s;
                std::memcpy(&s.scale, data, sizeof(Vector));
                std::memcpy(&s.sheerRate, data + sizeof(Vector), sizeof(float));
                s.sheerAxis = data[sizeof(Vector) + sizeof(float)];
                return s;
            }

            float sheerSnap(float sheer)
            {
                if (sheer < -0.65f) {
                    return sheer + 0.15f;
                } else if (sheer > 0.65f) {
                    return sheer - 0.15f;
                } else if (sheer < -0.55f) {
                    return -0.50f;
                } else if (sheer > 0.55f) {
                    return 0.50f;
                } else if (sheer < -0.05f) {
                    return sheer + 0.05f;
                } else if (sheer > 0.05f) {
                    return sheer - 0.05f;
                }
                return 0.0f;
            }

            void applySheer(CoordSystem& temp, uint8_t axis, float sheer)
            {
                switch (axis) {
                case 1:
                    temp.xAxis.y = sheer;
                    break;
                case 2:
                    temp.xAxis.z = sheer;
                    break;
                case 3:
                    temp.yAxis.x = sheer;
                    break;
                case 4:
                    temp.yAxis.z = sheer;
                    break;
                case 5:
                    temp.zAxis.x = sheer;
                    break;
                case 6:
                    temp.zAxis.y = sheer;
                    break;
                default:
                    break;
                }
            }

            CoordSystem& mulScale(CoordSystem& self, const Scale& s)
            {
                CoordSystem temp;
                applySheer(temp, s.sheerAxis, sheerSnap(s.sheerRate));
                mulCoords(self, temp);
                self.xAxis = self.xAxis * s.scale;
                self.yAxis = self.yAxis * s.scale;
                self.zAxis = self.zAxis * s.scale;
                self.origin.x /= s.scale.x;
                self.origin.y /= s.scale.y;
                self.origin.z /= s.scale.z;
                return self;
            }

            CoordSystem& divScale(CoordSystem& self, const Scale& s)
            {
                self.xAxis = { self.xAxis.x / s.scale.x, self.xAxis.y / s.scale.y, self.xAxis.z / s.scale.z };
                self.yAxis = { self.yAxis.x / s.scale.x, self.yAxis.y / s.scale.y, self.yAxis.z / s.scale.z };
                self.zAxis = { self.zAxis.x / s.scale.x, self.zAxis.y / s.scale.y, self.zAxis.z / s.scale.z };
                self.origin.x *= s.scale.x;
                self.origin.y *= s.scale.y;
                self.origin.z *= s.scale.z;
                CoordSystem temp;
                applySheer(temp, s.sheerAxis, -sheerSnap(s.sheerRate));
                mulCoords(self, temp);
                return self;
            }

            CoordSystem transpose(const CoordSystem& c)
            {
                CoordSystem result;
                result.origin = -transformVector(c.origin, c);
                result.xAxis = { c.xAxis.x, c.yAxis.x, c.zAxis.x };
                result.yAxis = { c.xAxis.y, c.yAxis.y, c.zAxis.y };
                result.zAxis = { c.xAxis.z, c.yAxis.z, c.zAxis.z };
                return result;
            }

            float sign(float value)
            {
                if (value > 0) {
                    return 1.0f;
                }
                if (value < 0) {
                    return -1.0f;
                }
                return 0.0f;
            }

            void splitInHalf(Poly& poly, Poly& other)
            {
                int n = static_cast<int>(poly.vertices.size());
                int m = n / 2;
                other = poly;
                int otherCount = (n - m) + 1;
                int count = m + 1;
                other.vertices.resize(static_cast<size_t>(otherCount));
                for (int i = 0; i < otherCount - 1; i++) {
                    other.vertices[static_cast<size_t>(i)] = poly.vertices[static_cast<size_t>(i + m)];
                }
                other.vertices[static_cast<size_t>(otherCount - 1)] = poly.vertices[0];
                poly.vertices.resize(static_cast<size_t>(count));
                poly.polyFlags |= PF_EdCut;
                other.polyFlags |= PF_EdCut;
            }

            Poly emptyPoly()
            {
                Poly poly;
                poly.link = -1;
                poly.brushPoly = -1;
                return poly;
            }

            int32_t addThing(std::vector<Vector>& table, const Vector& v, float thresh, bool check)
            {
                if (check) {
                    for (size_t i = 0; i < table.size(); i++) {
                        const Vector& t = table[i];
                        float temp = v.x - t.x;
                        if (temp > -thresh && temp < thresh) {
                            temp = v.y - t.y;
                            if (temp > -thresh && temp < thresh) {
                                temp = v.z - t.z;
                                if (temp > -thresh && temp < thresh) {
                                    return static_cast<int32_t>(i);
                                }
                            }
                        }
                    }
                }
                table.push_back(v);
                return static_cast<int32_t>(table.size() - 1);
            }

            float findNearestVertex(const UModel& model, const Vector& source, Vector& dest, float minRadius, int32_t iNode, int32_t& pVertex)
            {
                float resultRadius = -1.0f;
                while (iNode >= 0) {
                    const BspNode* node = &model.nodes[static_cast<size_t>(iNode)];
                    int32_t iBack = node->back;
                    float planeDist = node->plane.x * source.x + node->plane.y * source.y + node->plane.z * source.z - node->plane.w;
                    if (planeDist >= -minRadius && node->front >= 0) {
                        float tempRadius = findNearestVertex(model, source, dest, minRadius, node->front, pVertex);
                        if (tempRadius >= 0.0f) {
                            resultRadius = tempRadius;
                            minRadius = tempRadius;
                        }
                    }
                    if (planeDist > -minRadius && planeDist <= minRadius) {
                        while (iNode >= 0) {
                            node = &model.nodes[static_cast<size_t>(iNode)];
                            const BspSurf& surf = model.surfs[static_cast<size_t>(node->surf)];
                            const Vector& base = model.points[static_cast<size_t>(surf.pBase)];
                            float tempRadiusSquared = (source - base).sizeSquared();
                            if (tempRadiusSquared < minRadius * minRadius) {
                                pVertex = surf.pBase;
                                resultRadius = minRadius = std::sqrt(tempRadiusSquared);
                                dest = base;
                            }
                            for (int b = 0; b < node->numVertices; b++) {
                                int32_t index = model.verts[static_cast<size_t>(node->vertPool + b)].vertex;
                                const Vector& vertex = model.points[static_cast<size_t>(index)];
                                float squared = (source - vertex).sizeSquared();
                                if (squared < minRadius * minRadius) {
                                    pVertex = index;
                                    resultRadius = minRadius = std::sqrt(squared);
                                    dest = vertex;
                                }
                            }
                            iNode = node->iPlane;
                        }
                    }
                    if (planeDist > minRadius) {
                        break;
                    }
                    iNode = iBack;
                }
                return resultRadius;
            }

            int32_t addVector(UModel& model, const Vector& v, bool normal)
            {
                return addThing(model.vectors, v, normal ? ThreshNormalsAreSame : ThreshVectorsAreNear, true);
            }

            int32_t addPoint(UModel& model, const Vector& v, bool exact)
            {
                float thresh = exact ? ThreshPointsAreSame : ThreshPointsAreNear;
                Vector temp;
                int32_t pVertex = -1;
                float nearest = model.nodes.empty() ? -1.0f : findNearestVertex(model, v, temp, thresh, 0, pVertex);
                if (nearest >= 0.0f && nearest <= thresh) {
                    return pVertex;
                }
                return addThing(model.points, v, thresh, !fastRebuild);
            }

            UPolys* ensurePolys(UModel& model)
            {
                if (!model.polys) {
                    UObject* outer = model.outer ? model.outermost() : nullptr;
                    UClass* cls = objects::findClass("Engine.Polys");
                    if (!cls) {
                        cls = objects::intrinsicClass(Name("Polys"));
                    }
                    UPolys* polys = outer ? cast<UPolys>(objects::newObject(cls, outer, Name(), RF_Transactional | 0x00070000)) : nullptr;
                    if (polys) {
                        polys->loaded = true;
                    }
                    model.polys = polys;
                }
                return model.polys;
            }

            Poly* findBestSplit(std::vector<Poly*>& list, Optimization opt, int balance)
            {
                int numPolys = static_cast<int>(list.size());
                if (numPolys == 1) {
                    return list[0];
                }
                int inc = 1;
                if (opt == Optimization::Optimal) {
                    inc = 1;
                } else if (opt == Optimization::Good) {
                    inc = std::max(1, numPolys / 20);
                } else {
                    inc = std::max(1, numPolys / 4);
                }
                int i = 0;
                for (i = 0; i < numPolys; i++) {
                    if (!(list[static_cast<size_t>(i)]->polyFlags & PF_AddLast)) {
                        break;
                    }
                }
                bool allSemiSolids = i >= numPolys;
                Poly* best = nullptr;
                float bestScore = 0;
                for (i = 0; i < numPolys; i += inc) {
                    int splits = 0;
                    int front = 0;
                    int back = 0;
                    int coplanar = 0;
                    int index = i - 1;
                    Poly* poly = nullptr;
                    do {
                        index++;
                        poly = list[static_cast<size_t>(index)];
                    } while (index < (i + inc) && index < numPolys && (poly->polyFlags & PF_AddLast) && !allSemiSolids);
                    if (index >= i + inc || index >= numPolys) {
                        continue;
                    }
                    for (int j = 0; j < numPolys; j += inc) {
                        if (j == index) {
                            continue;
                        }
                        Poly* other = list[static_cast<size_t>(j)];
                        switch (splitWithPlaneFast(*other, makePlane(poly->base, poly->normal), nullptr, nullptr)) {
                        case SplitCoplanar:
                            coplanar++;
                            break;
                        case SplitFront:
                            front++;
                            break;
                        case SplitBack:
                            back++;
                            break;
                        case SplitSplit:
                            if (!(poly->polyFlags & PF_Portal)) {
                                splits++;
                            } else {
                                splits += 16;
                            }
                            break;
                        }
                    }
                    float score = static_cast<float>(balance * std::abs(front - back) + (100 - balance) * splits);
                    if (score < bestScore || !best) {
                        best = poly;
                        bestScore = score;
                    }
                }
                return best;
            }

            void splitPolyList(UModel& model, int32_t parent, NodePlace place, std::vector<Poly*>& list, Optimization opt, int balance, int rebuildSimplePolys, std::deque<Poly>& arena)
            {
                std::vector<Poly*> frontList;
                std::vector<Poly*> backList;
                Poly* splitPoly = findBestSplit(list, opt, balance);
                if (rebuildSimplePolys) {
                    splitPoly->link = static_cast<int32_t>(model.surfs.size());
                }
                int32_t ourNode = addNode(model, parent, place, 0, *splitPoly);
                int32_t planeNode = ourNode;
                for (Poly* poly : list) {
                    if (poly == splitPoly) {
                        continue;
                    }
                    Poly splitFront;
                    Poly splitBack;
                    switch (splitWithPlane(*poly, splitPoly->base, splitPoly->normal, &splitFront, &splitBack, false)) {
                    case SplitCoplanar:
                        if (rebuildSimplePolys) {
                            poly->link = static_cast<int32_t>(model.surfs.size()) - 1;
                        }
                        planeNode = addNode(model, planeNode, NodePlane, 0, *poly);
                        break;
                    case SplitFront:
                        frontList.push_back(poly);
                        break;
                    case SplitBack:
                        backList.push_back(poly);
                        break;
                    case SplitSplit: {
                        Poly& frontPoly = arena.emplace_back(std::move(splitFront));
                        Poly& backPoly = arena.emplace_back(std::move(splitBack));
                        frontList.push_back(&frontPoly);
                        backList.push_back(&backPoly);
                        if (static_cast<int>(frontPoly.vertices.size()) >= VertexThreshold) {
                            Poly& half = arena.emplace_back();
                            splitInHalf(frontPoly, half);
                            frontList.push_back(&half);
                        }
                        if (static_cast<int>(backPoly.vertices.size()) >= VertexThreshold) {
                            Poly& half = arena.emplace_back();
                            splitInHalf(backPoly, half);
                            backList.push_back(&half);
                        }
                        break;
                    }
                    }
                }
                if (!frontList.empty()) {
                    splitPolyList(model, ourNode, NodeFront, frontList, opt, balance, rebuildSimplePolys, arena);
                }
                if (!backList.empty()) {
                    splitPolyList(model, ourNode, NodeBack, backList, opt, balance, rebuildSimplePolys, arena);
                }
            }

            bool tryToMerge(Poly& poly1, Poly& poly2)
            {
                int n1 = static_cast<int>(poly1.vertices.size());
                int n2 = static_cast<int>(poly2.vertices.size());
                if (n1 + n2 > MaxVertices) {
                    return false;
                }
                int start1 = 0;
                int start2 = 0;
                bool found = false;
                for (start1 = 0; start1 < n1 && !found; start1++) {
                    for (start2 = 0; start2 < n2; start2++) {
                        if (pointsAreSame(poly1.vertices[static_cast<size_t>(start1)], poly2.vertices[static_cast<size_t>(start2)])) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
                int end1 = start1;
                int end2 = start2;
                int test1 = start1 + 1;
                if (test1 >= n1) {
                    test1 = 0;
                }
                int test2 = start2 - 1;
                if (test2 < 0) {
                    test2 = n2 - 1;
                }
                if (pointsAreSame(poly1.vertices[static_cast<size_t>(test1)], poly2.vertices[static_cast<size_t>(test2)])) {
                    end1 = test1;
                    start2 = test2;
                } else {
                    test1 = start1 - 1;
                    if (test1 < 0) {
                        test1 = n1 - 1;
                    }
                    test2 = start2 + 1;
                    if (test2 >= n2) {
                        test2 = 0;
                    }
                    if (pointsAreSame(poly1.vertices[static_cast<size_t>(test1)], poly2.vertices[static_cast<size_t>(test2)])) {
                        start1 = test1;
                        end2 = test2;
                    } else {
                        return false;
                    }
                }
                Poly merged = poly1;
                merged.vertices.clear();
                int vertex = end1;
                for (int i = 0; i < n1; i++) {
                    merged.vertices.push_back(poly1.vertices[static_cast<size_t>(vertex)]);
                    if (++vertex >= n1) {
                        vertex = 0;
                    }
                }
                vertex = end2;
                for (int i = 0; i < n2 - 2; i++) {
                    if (++vertex >= n2) {
                        vertex = 0;
                    }
                    merged.vertices.push_back(poly2.vertices[static_cast<size_t>(vertex)]);
                }
                if (removeColinears(merged)) {
                    if (static_cast<int>(merged.vertices.size()) <= MaxNodeVertices) {
                        poly1 = merged;
                        poly2.vertices.clear();
                        return true;
                    }
                    return false;
                }
                return false;
            }

            void mergeCoplanarList(std::vector<Poly>& polys, const std::vector<int32_t>& list)
            {
                bool mergeAgain = true;
                while (mergeAgain) {
                    mergeAgain = false;
                    for (size_t i = 0; i < list.size(); i++) {
                        Poly& poly1 = polys[static_cast<size_t>(list[i])];
                        if (!poly1.vertices.empty()) {
                            for (size_t j = i + 1; j < list.size(); j++) {
                                Poly& poly2 = polys[static_cast<size_t>(list[j])];
                                if (!poly2.vertices.empty()) {
                                    if (tryToMerge(poly1, poly2)) {
                                        mergeAgain = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void makeEdPolys(UModel& model, int32_t iNode, std::vector<Poly>& out)
            {
                Poly temp;
                if (nodeToFPoly(model, iNode, temp) >= 3) {
                    out.push_back(temp);
                }
                const BspNode node = model.nodes[static_cast<size_t>(iNode)];
                if (node.front >= 0) {
                    makeEdPolys(model, node.front, out);
                }
                if (node.back >= 0) {
                    makeEdPolys(model, node.back, out);
                }
                if (node.iPlane >= 0) {
                    makeEdPolys(model, node.iPlane, out);
                }
            }

            void cleanupNodes(UModel& model, int32_t iNode, int32_t iParent)
            {
                model.nodes[static_cast<size_t>(iNode)].nodeFlags &= static_cast<uint8_t>(~(NF_IsNew | NF_IsFront | NF_IsBack));
                if (model.nodes[static_cast<size_t>(iNode)].front >= 0) {
                    cleanupNodes(model, model.nodes[static_cast<size_t>(iNode)].front, iNode);
                }
                if (model.nodes[static_cast<size_t>(iNode)].back >= 0) {
                    cleanupNodes(model, model.nodes[static_cast<size_t>(iNode)].back, iNode);
                }
                if (model.nodes[static_cast<size_t>(iNode)].iPlane >= 0) {
                    cleanupNodes(model, model.nodes[static_cast<size_t>(iNode)].iPlane, iNode);
                }
                BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                if (node.numVertices == 0 && node.iPlane >= 0) {
                    BspNode& planeNode = model.nodes[static_cast<size_t>(node.iPlane)];
                    float d = node.plane.x * planeNode.plane.x + node.plane.y * planeNode.plane.y + node.plane.z * planeNode.plane.z;
                    if (d >= 0.0f) {
                        planeNode.front = node.front;
                        planeNode.back = node.back;
                    } else {
                        planeNode.front = node.back;
                        planeNode.back = node.front;
                    }
                    if (iParent < 0) {
                        node = planeNode;
                        planeNode.numVertices = 0;
                    } else {
                        BspNode& parentNode = model.nodes[static_cast<size_t>(iParent)];
                        if (parentNode.front == iNode) {
                            parentNode.front = node.iPlane;
                        } else if (parentNode.back == iNode) {
                            parentNode.back = node.iPlane;
                        } else if (parentNode.iPlane == iNode) {
                            parentNode.iPlane = node.iPlane;
                        }
                    }
                } else if (node.numVertices == 0 && (node.front < 0 || node.back < 0)) {
                    int32_t replacement = -1;
                    if (node.front >= 0) {
                        replacement = node.front;
                    } else if (node.back >= 0) {
                        replacement = node.back;
                    }
                    if (iParent < 0) {
                        if (replacement < 0) {
                            model.nodes.clear();
                        } else {
                            node = model.nodes[static_cast<size_t>(replacement)];
                        }
                    } else {
                        BspNode& parentNode = model.nodes[static_cast<size_t>(iParent)];
                        if (parentNode.front == iNode) {
                            parentNode.front = replacement;
                        } else if (parentNode.back == iNode) {
                            parentNode.back = replacement;
                        } else if (parentNode.iPlane == iNode) {
                            parentNode.iPlane = replacement;
                        }
                    }
                }
            }

            enum Filter {
                FOutside = 0,
                FInside = 1,
                FCoplanarOutside = 2,
                FCoplanarInside = 3,
                FCospatialFacingIn = 4,
                FCospatialFacingOut = 5,
            };

            struct CsgContext {
                UModel* model = nullptr;
                int32_t node = -1;
                int32_t lastCoplanar = -1;
                size_t numNodes = 0;
                int discarded = 0;
            };

            CsgContext context;

            using FilterFunc = void (*)(UModel& model, int32_t iNode, Poly& poly, Filter filter, NodePlace place);

            void addBrushToWorld(UModel& model, int32_t iNode, Poly& poly, Filter filter, NodePlace place)
            {
                if (filter == FOutside || filter == FCoplanarOutside) {
                    addNode(model, iNode, place, NF_IsNew, poly);
                } else if (filter == FCospatialFacingOut && !(poly.polyFlags & PF_Semisolid)) {
                    addNode(model, iNode, place, NF_IsNew, poly);
                }
            }

            void discardWorldPoly()
            {
                context.discarded++;
                BspNode& node = context.model->nodes[static_cast<size_t>(context.node)];
                if (node.numVertices) {
                    node.numVertices = 0;
                }
            }

            void addWorldToBrush(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FOutside || filter == FCoplanarOutside) {
                    if (poly.polyFlags & PF_EdCut) {
                        addNode(*context.model, context.lastCoplanar, NodePlane, NF_IsNew, poly);
                    }
                } else {
                    discardWorldPoly();
                }
            }

            void subtractBrushFromWorld(UModel& model, int32_t iNode, Poly& poly, Filter filter, NodePlace place)
            {
                if (filter == FCoplanarInside || filter == FInside) {
                    reversePoly(poly);
                    addNode(model, iNode, place, NF_IsNew, poly);
                    reversePoly(poly);
                }
            }

            void subtractWorldToBrush(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FOutside || filter == FCoplanarOutside || filter == FCospatialFacingIn) {
                    if (poly.polyFlags & PF_EdCut) {
                        addNode(*context.model, context.lastCoplanar, NodePlane, NF_IsNew, poly);
                    }
                } else {
                    discardWorldPoly();
                }
            }

            void keepPoly(Poly& poly, bool reversed)
            {
                if (fixPoly(poly) >= 3) {
                    if (reversed) {
                        reversePoly(poly);
                    }
                    context.model->polys->polys.push_back(poly);
                    if (reversed) {
                        reversePoly(poly);
                    }
                }
            }

            void intersectBrushWithWorld(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FInside || filter == FCoplanarInside) {
                    keepPoly(poly, false);
                }
            }

            void intersectWorldWithBrush(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FInside || filter == FCoplanarInside || filter == FCospatialFacingOut) {
                    keepPoly(poly, false);
                }
            }

            void deintersectBrushWithWorld(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FOutside || filter == FCoplanarOutside) {
                    keepPoly(poly, false);
                }
            }

            void deintersectWorldWithBrush(UModel&, int32_t, Poly& poly, Filter filter, NodePlace)
            {
                if (filter == FCoplanarInside || filter == FInside || filter == FCospatialFacingIn) {
                    keepPoly(poly, true);
                }
            }

            struct CoplanarInfo {
                int32_t originalNode = -1;
                int32_t backNode = -1;
                bool backNodeOutside = false;
                bool frontLeafOutside = false;
                bool processingBack = false;
            };

            void filterEdPoly(FilterFunc func, UModel& model, int32_t iNode, Poly& poly, CoplanarInfo info, bool outside);

            void filterLeaf(FilterFunc func, UModel& model, int32_t iNode, Poly& poly, CoplanarInfo info, bool leafOutside, NodePlace place)
            {
                if (info.originalNode < 0) {
                    func(model, iNode, poly, leafOutside ? FOutside : FInside, place);
                    return;
                }
                if (!info.processingBack) {
                    info.frontLeafOutside = leafOutside;
                    if (info.backNode >= 0) {
                        info.processingBack = true;
                        filterEdPoly(func, model, info.backNode, poly, info, info.backNodeOutside);
                        return;
                    }
                    leafOutside = info.backNodeOutside;
                }
                Filter type = FCoplanarInside;
                if (!leafOutside && !info.frontLeafOutside) {
                    type = FCoplanarInside;
                } else if (leafOutside && info.frontLeafOutside) {
                    type = FCoplanarOutside;
                } else if (!leafOutside && info.frontLeafOutside) {
                    type = FCospatialFacingOut;
                } else {
                    type = FCospatialFacingIn;
                }
                func(model, info.originalNode, poly, type, NodePlane);
            }

            bool nodeIsCsg(const UModel& model, int32_t iNode)
            {
                return isCsg(model.nodes[static_cast<size_t>(iNode)], 0);
            }

            void filterEdPoly(FilterFunc func, UModel& model, int32_t iNode, Poly& polyIn, CoplanarInfo info, bool outside)
            {
                Poly poly = polyIn;
                for (;;) {
                    if (static_cast<int>(poly.vertices.size()) >= VertexThreshold) {
                        Poly temp;
                        splitInHalf(poly, temp);
                        filterEdPoly(func, model, iNode, temp, info, outside);
                    }
                    Poly frontPoly;
                    Poly backPoly;
                    const BspSurf& surf = model.surfs[static_cast<size_t>(model.nodes[static_cast<size_t>(iNode)].surf)];
                    Vector base = model.points[static_cast<size_t>(surf.pBase)];
                    Vector normal = model.vectors[static_cast<size_t>(surf.vNormal)];
                    int result = splitWithPlane(poly, base, normal, &frontPoly, &backPoly, false);
                    bool goFront = result == SplitFront;
                    if (result == SplitCoplanar && info.originalNode >= 0) {
                        errors++;
                        goFront = true;
                    }
                    if (goFront) {
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        outside = outside || isCsg(node, 0);
                        if (node.front < 0) {
                            filterLeaf(func, model, iNode, poly, info, outside, NodeFront);
                            return;
                        }
                        iNode = node.front;
                        continue;
                    }
                    if (result == SplitBack) {
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        outside = outside && !isCsg(node, 0);
                        if (node.back < 0) {
                            filterLeaf(func, model, iNode, poly, info, outside, NodeBack);
                            return;
                        }
                        iNode = node.back;
                        continue;
                    }
                    if (result == SplitCoplanar) {
                        info.originalNode = iNode;
                        info.backNode = -1;
                        info.processingBack = false;
                        info.backNodeOutside = outside;
                        bool newFrontOutside = outside;
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        int32_t ourFront = -1;
                        int32_t ourBack = -1;
                        if (node.plane.x * poly.normal.x + node.plane.y * poly.normal.y + node.plane.z * poly.normal.z >= 0.0f) {
                            ourFront = node.front;
                            ourBack = node.back;
                            if (isCsg(node, 0)) {
                                info.backNodeOutside = false;
                                newFrontOutside = true;
                            }
                        } else {
                            ourFront = node.back;
                            ourBack = node.front;
                            if (isCsg(node, 0)) {
                                info.backNodeOutside = true;
                                newFrontOutside = false;
                            }
                        }
                        if (ourFront < 0 && ourBack < 0) {
                            info.processingBack = true;
                            info.frontLeafOutside = newFrontOutside;
                            filterLeaf(func, model, iNode, poly, info, info.backNodeOutside, NodePlane);
                            return;
                        }
                        if (ourFront < 0 && ourBack >= 0) {
                            info.processingBack = true;
                            info.backNode = ourBack;
                            info.frontLeafOutside = newFrontOutside;
                            iNode = ourBack;
                            outside = info.backNodeOutside;
                            continue;
                        }
                        info.processingBack = false;
                        info.backNode = ourBack;
                        iNode = ourFront;
                        outside = newFrontOutside;
                        continue;
                    }
                    bool newFrontOutside = outside;
                    bool newBackOutside = outside;
                    if (nodeIsCsg(model, iNode)) {
                        newFrontOutside = true;
                        newBackOutside = false;
                    }
                    int32_t front = model.nodes[static_cast<size_t>(iNode)].front;
                    if (front < 0) {
                        filterLeaf(func, model, iNode, frontPoly, info, newFrontOutside, NodeFront);
                    } else {
                        filterEdPoly(func, model, front, frontPoly, info, newFrontOutside);
                    }
                    int32_t back = model.nodes[static_cast<size_t>(iNode)].back;
                    if (back < 0) {
                        filterLeaf(func, model, iNode, backPoly, info, newBackOutside, NodeBack);
                    } else {
                        filterEdPoly(func, model, back, backPoly, info, newBackOutside);
                    }
                    return;
                }
            }

            void bspFilterFPoly(FilterFunc func, UModel& model, Poly& poly)
            {
                CoplanarInfo info;
                if (model.nodes.empty()) {
                    func(model, 0, poly, model.rootOutside ? FOutside : FInside, NodeRoot);
                } else {
                    filterEdPoly(func, model, 0, poly, info, model.rootOutside != 0);
                }
            }

            void filterWorldThroughBrush(UModel& model, UModel& brush, uint8_t oper, int32_t iNode, const Plane* sphere)
            {
                while (iNode >= 0) {
                    int32_t iSurf = model.nodes[static_cast<size_t>(iNode)].surf;
                    if (model.nodes[static_cast<size_t>(iNode)].nodeFlags & NF_IsNew) {
                        return;
                    }
                    bool doFront = true;
                    bool doBack = true;
                    if (sphere) {
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        float dist = node.plane.x * sphere->x + node.plane.y * sphere->y + node.plane.z * sphere->z - node.plane.w;
                        doFront = dist >= -sphere->w;
                        doBack = dist <= sphere->w;
                    }
                    Poly temp;
                    if (doFront && doBack && nodeToFPoly(model, iNode, temp) > 0) {
                        temp.actor = model.surfs[static_cast<size_t>(iSurf)].actor;
                        temp.brushPoly = model.surfs[static_cast<size_t>(iSurf)].brushPoly;
                        if (oper == CsgAdd || oper == CsgSubtract) {
                            context.node = iNode;
                            context.model = &model;
                            context.discarded = 0;
                            context.numNodes = model.nodes.size();
                            context.lastCoplanar = iNode;
                            while (model.nodes[static_cast<size_t>(context.lastCoplanar)].iPlane >= 0) {
                                context.lastCoplanar = model.nodes[static_cast<size_t>(context.lastCoplanar)].iPlane;
                            }
                            bspFilterFPoly(oper == CsgAdd ? addWorldToBrush : subtractWorldToBrush, brush, temp);
                            if (context.discarded == 0) {
                                model.nodes[static_cast<size_t>(context.lastCoplanar)].iPlane = -1;
                                model.nodes.resize(context.numNodes);
                            } else {
                                BspNode& node = model.nodes[static_cast<size_t>(context.node)];
                                if (node.numVertices) {
                                    node.numVertices = 0;
                                }
                            }
                        } else if (oper == CsgIntersect) {
                            bspFilterFPoly(intersectWorldWithBrush, brush, temp);
                        } else if (oper == CsgDeintersect) {
                            bspFilterFPoly(deintersectWorldWithBrush, brush, temp);
                        }
                    }
                    if (doFront && model.nodes[static_cast<size_t>(iNode)].front >= 0) {
                        filterWorldThroughBrush(model, brush, oper, model.nodes[static_cast<size_t>(iNode)].front, sphere);
                    }
                    if (doBack && model.nodes[static_cast<size_t>(iNode)].back >= 0) {
                        filterWorldThroughBrush(model, brush, oper, model.nodes[static_cast<size_t>(iNode)].back, sphere);
                    }
                    iNode = model.nodes[static_cast<size_t>(iNode)].iPlane;
                }
            }

            void tagReferencedNodes(const UModel& model, std::vector<int32_t>& nodeRef, std::vector<int32_t>& polyRef, int32_t iNode)
            {
                const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                nodeRef[static_cast<size_t>(iNode)] = 0;
                polyRef[static_cast<size_t>(node.surf)] = 0;
                if (node.front >= 0) {
                    tagReferencedNodes(model, nodeRef, polyRef, node.front);
                }
                if (node.back >= 0) {
                    tagReferencedNodes(model, nodeRef, polyRef, node.back);
                }
                if (node.iPlane >= 0) {
                    tagReferencedNodes(model, nodeRef, polyRef, node.iPlane);
                }
            }

            struct PointVert {
                int32_t node = -1;
                int32_t vertex = 0;
                int32_t next = -1;
            };

            struct PointVertList {
                UModel* model = nullptr;
                std::vector<int32_t> index;
                std::vector<PointVert> entries;

                void alloc(UModel& m)
                {
                    model = &m;
                    index.assign(m.points.size(), -1);
                    entries.clear();
                }

                void addNode(int32_t iNode)
                {
                    const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                    for (int i = 0; i < node.numVertices; i++) {
                        int32_t pVertex = model->verts[static_cast<size_t>(node.vertPool + i)].vertex;
                        PointVert entry;
                        entry.node = iNode;
                        entry.vertex = i;
                        entry.next = index[static_cast<size_t>(pVertex)];
                        entries.push_back(entry);
                        index[static_cast<size_t>(pVertex)] = static_cast<int32_t>(entries.size() - 1);
                    }
                }

                void addAllNodes()
                {
                    for (size_t i = 0; i < model->nodes.size(); i++) {
                        addNode(static_cast<int32_t>(i));
                    }
                }

                void removeNode(int32_t iNode)
                {
                    const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                    for (int i = 0; i < node.numVertices; i++) {
                        int32_t pVertex = model->verts[static_cast<size_t>(node.vertPool + i)].vertex;
                        int32_t* link = &index[static_cast<size_t>(pVertex)];
                        while (*link >= 0) {
                            if (entries[static_cast<size_t>(*link)].node == iNode) {
                                *link = entries[static_cast<size_t>(*link)].next;
                                if (*link < 0) {
                                    break;
                                }
                            } else {
                                link = &entries[static_cast<size_t>(*link)].next;
                            }
                        }
                    }
                }
            };

            void addPointToNode(UModel& model, PointVertList* pointVerts, int32_t iNode, int vertexNumber, int32_t pVertex)
            {
                if (model.nodes[static_cast<size_t>(iNode)].numVertices + 1 >= MaxNodeVertices) {
                    return;
                }
                if (pointVerts) {
                    pointVerts->removeNode(iNode);
                }
                BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                int32_t oldVert = node.vertPool;
                node.vertPool = static_cast<int32_t>(model.verts.size());
                model.verts.resize(model.verts.size() + static_cast<size_t>(node.numVertices) + 1);
                for (int i = 0; i < vertexNumber; i++) {
                    model.verts[static_cast<size_t>(node.vertPool + i)] = model.verts[static_cast<size_t>(oldVert + i)];
                }
                for (int i = vertexNumber; i < node.numVertices; i++) {
                    model.verts[static_cast<size_t>(node.vertPool + i + 1)] = model.verts[static_cast<size_t>(oldVert + i)];
                }
                model.verts[static_cast<size_t>(node.vertPool + vertexNumber)].vertex = pVertex;
                model.verts[static_cast<size_t>(node.vertPool + vertexNumber)].side = -1;
                node.numVertices++;
                if (pointVerts) {
                    pointVerts->addNode(iNode);
                }
            }

            int distributePoint(UModel& model, PointVertList* pointVerts, int32_t iNode, int32_t pVertex)
            {
                int count = 0;
                const Vector point = model.points[static_cast<size_t>(pVertex)];
                const BspNode& start = model.nodes[static_cast<size_t>(iNode)];
                float dist = start.plane.x * point.x + start.plane.y * point.y + start.plane.z * point.z - start.plane.w;
                if (dist < ThreshOptGeomCoplanar) {
                    if (model.nodes[static_cast<size_t>(iNode)].back >= 0) {
                        count += distributePoint(model, pointVerts, model.nodes[static_cast<size_t>(iNode)].back, pVertex);
                    }
                }
                if (dist > -ThreshOptGeomCoplanar) {
                    if (model.nodes[static_cast<size_t>(iNode)].front >= 0) {
                        count += distributePoint(model, pointVerts, model.nodes[static_cast<size_t>(iNode)].front, pVertex);
                    }
                }
                if (dist > -ThreshOptGeomCoplanar && dist < ThreshOptGeomCoplanar) {
                    for (; iNode >= 0; iNode = model.nodes[static_cast<size_t>(iNode)].iPlane) {
                        const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                        int n = node.numVertices;
                        bool contains = false;
                        for (int i = 0; i < n; i++) {
                            if (model.verts[static_cast<size_t>(node.vertPool + i)].vertex == pVertex) {
                                contains = true;
                                break;
                            }
                        }
                        if (contains) {
                            continue;
                        }
                        int foundSide = -1;
                        bool skippedColinear = false;
                        bool skippedInside = false;
                        int i = 0;
                        for (i = 0; i < n; i++) {
                            int j = i > 0 ? i - 1 : n - 1;
                            const Vector& vi = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                            const Vector& vj = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + j)].vertex)];
                            Vector side = vi - vj;
                            Vector sidePlaneNormal = side.cross({ node.plane.x, node.plane.y, node.plane.z });
                            float sizeSquared = sidePlaneNormal.sizeSquared();
                            if (sizeSquared > 0.001f * 0.001f) {
                                float sideDist = (point - vi).dot(sidePlaneNormal) / std::sqrt(sizeSquared);
                                if (sideDist >= ThreshOptGeomCosidal) {
                                    break;
                                } else if (sideDist > -ThreshOptGeomCosidal) {
                                    Vector midPoint = (vi + vj) * 0.5f;
                                    Vector midDist = point - midPoint;
                                    if (midDist.sizeSquared() <= 0.501f * 0.501f * side.sizeSquared()) {
                                        foundSide = i;
                                    } else {
                                        skippedColinear = true;
                                    }
                                } else {
                                    skippedInside = true;
                                }
                            } else {
                                errors++;
                            }
                        }
                        if (i == n && foundSide >= 0) {
                            addPointToNode(model, pointVerts, iNode, foundSide, pVertex);
                            count++;
                        } else if (skippedColinear || skippedInside) {
                            errors++;
                        }
                    }
                }
                return count;
            }

            void mergeNearPoints(UModel& model, float dist)
            {
                std::vector<int32_t> remap(model.points.size());
                for (size_t i = 0; i < model.points.size(); i++) {
                    remap[i] = static_cast<int32_t>(i);
                    const Vector& point = model.points[i];
                    for (size_t j = 0; j < i; j++) {
                        if ((model.points[j] - point).sizeSquared() < dist * dist) {
                            remap[i] = static_cast<int32_t>(j);
                            break;
                        }
                    }
                }
                for (BspVert& vert : model.verts) {
                    if (vert.vertex >= 0 && static_cast<size_t>(vert.vertex) < model.points.size()) {
                        vert.vertex = remap[static_cast<size_t>(vert.vertex)];
                    }
                }
                for (BspSurf& surf : model.surfs) {
                    if (surf.pBase >= 0 && static_cast<size_t>(surf.pBase) < model.points.size()) {
                        surf.pBase = remap[static_cast<size_t>(surf.pBase)];
                    }
                }
                for (BspNode& node : model.nodes) {
                    int k = 0;
                    for (int j = 0; j < node.numVertices; j++) {
                        BspVert a = model.verts[static_cast<size_t>(node.vertPool + j)];
                        BspVert b = model.verts[static_cast<size_t>(node.vertPool + (j ? j - 1 : node.numVertices - 1))];
                        if (a.vertex != b.vertex) {
                            model.verts[static_cast<size_t>(node.vertPool + k++)] = a;
                        }
                    }
                    node.numVertices = static_cast<uint8_t>(k >= 3 ? k : 0);
                }
            }

            void boxAdd(BoundingBox& box, const Vector& p)
            {
                if (box.valid) {
                    box.min = { std::min(box.min.x, p.x), std::min(box.min.y, p.y), std::min(box.min.z, p.z) };
                    box.max = { std::max(box.max.x, p.x), std::max(box.max.y, p.y), std::max(box.max.z, p.z) };
                } else {
                    box.min = p;
                    box.max = p;
                    box.valid = true;
                }
            }

            void boxAdd(BoundingBox& box, const BoundingBox& other)
            {
                if (box.valid && other.valid) {
                    box.min = { std::min(box.min.x, other.min.x), std::min(box.min.y, other.min.y), std::min(box.min.z, other.min.z) };
                    box.max = { std::max(box.max.x, other.max.x), std::max(box.max.y, other.max.y), std::max(box.max.z, other.max.z) };
                } else {
                    box = other;
                }
            }

            Poly buildInfiniteFPoly(const UModel& model, int32_t iNode)
            {
                const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                const BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
                Vector base = model.points[static_cast<size_t>(surf.pBase)];
                Vector normal = model.vectors[static_cast<size_t>(surf.vNormal)];
                Vector axis1 = std::abs(normal.z) > std::abs(normal.x) && std::abs(normal.z) > std::abs(normal.y) ? Vector { 1, 0, 0 } : Vector { 0, 0, 1 };
                axis1 = safeNormal(axis1 - normal * axis1.dot(normal));
                Vector axis2 = axis1.cross(normal);
                Poly poly = emptyPoly();
                poly.normal = normal;
                poly.base = base;
                poly.vertices = {
                    base + axis1 * WorldMax + axis2 * WorldMax,
                    base - axis1 * WorldMax + axis2 * WorldMax,
                    base - axis1 * WorldMax - axis2 * WorldMax,
                    base + axis1 * WorldMax - axis2 * WorldMax,
                };
                return poly;
            }

            int splitWithNode(const UModel& model, const Poly& poly, int32_t iNode, Poly* front, Poly* back, bool precise)
            {
                const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                const BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
                return splitWithPlane(poly, model.points[static_cast<size_t>(surf.pBase)], model.vectors[static_cast<size_t>(surf.vNormal)], front, back, precise);
            }

            void updateConvolutionWithPolys(UModel& model, int32_t iNode, const std::vector<Poly*>& list)
            {
                BoundingBox box;
                model.nodes[static_cast<size_t>(iNode)].collisionBound = static_cast<int32_t>(model.leafHulls.size());
                for (size_t i = 0; i < list.size(); i++) {
                    if (list[i]->brushPoly != -1) {
                        size_t j = 0;
                        for (j = 0; j < i; j++) {
                            if (list[j]->brushPoly == list[i]->brushPoly) {
                                break;
                            }
                        }
                        if (j >= i) {
                            model.leafHulls.push_back(list[i]->brushPoly);
                        }
                    }
                    for (const Vector& v : list[i]->vertices) {
                        boxAdd(box, v);
                    }
                }
                model.leafHulls.push_back(-1);
                float values[6] = { box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z };
                for (float value : values) {
                    int32_t raw = 0;
                    std::memcpy(&raw, &value, sizeof(raw));
                    model.leafHulls.push_back(raw);
                }
            }

            void splitPartitioner(const UModel& model, const std::vector<Poly*>& list, std::vector<Poly*>& frontList, std::vector<Poly*>& backList, size_t n, Poly infinite, std::deque<Poly>& arena)
            {
                Poly frontPoly;
                Poly backPoly;
                while (n < list.size()) {
                    if (static_cast<int>(infinite.vertices.size()) >= VertexThreshold) {
                        Poly half;
                        splitInHalf(infinite, half);
                        splitPartitioner(model, list, frontList, backList, n, half, arena);
                    }
                    Poly* poly = list[n];
                    switch (splitWithPlane(infinite, poly->base, poly->normal, &frontPoly, &backPoly, false)) {
                    case SplitCoplanar:
                        break;
                    case SplitFront:
                        return;
                    case SplitSplit:
                        infinite = backPoly;
                        break;
                    case SplitBack:
                        break;
                    }
                    n++;
                }
                Poly& reversed = arena.emplace_back(infinite);
                reversePoly(reversed);
                reversed.brushPoly |= 0x40000000;
                frontList.push_back(&reversed);
                Poly& plain = arena.emplace_back(infinite);
                backList.push_back(&plain);
            }

            void filterBound(UModel& model, BoundingBox* parentBound, int32_t iNode, const std::vector<Poly*>& list, bool outside, std::deque<Poly>& arena)
            {
                const BspNode node = model.nodes[static_cast<size_t>(iNode)];
                const BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
                Vector base = model.points[static_cast<size_t>(surf.pBase)];
                Vector normal = model.vectors[static_cast<size_t>(surf.vNormal)];
                BoundingBox bound;
                bound.min = { 65536.0f, 65536.0f, 65536.0f };
                bound.max = { -65536.0f, -65536.0f, -65536.0f };
                bound.valid = true;
                std::vector<Poly*> frontList;
                std::vector<Poly*> backList;
                for (Poly* poly : list) {
                    Poly splitFront;
                    Poly splitBack;
                    switch (splitWithPlane(*poly, base, normal, &splitFront, &splitBack, false)) {
                    case SplitCoplanar:
                        frontList.push_back(poly);
                        backList.push_back(poly);
                        break;
                    case SplitFront:
                        frontList.push_back(poly);
                        break;
                    case SplitBack:
                        backList.push_back(poly);
                        break;
                    case SplitSplit: {
                        Poly& frontPoly = arena.emplace_back(std::move(splitFront));
                        Poly& backPoly = arena.emplace_back(std::move(splitBack));
                        if (static_cast<int>(frontPoly.vertices.size()) >= VertexThreshold) {
                            Poly& half = arena.emplace_back();
                            splitInHalf(frontPoly, half);
                            frontList.push_back(&half);
                        }
                        frontList.push_back(&frontPoly);
                        if (static_cast<int>(backPoly.vertices.size()) >= VertexThreshold) {
                            Poly& half = arena.emplace_back();
                            splitInHalf(backPoly, half);
                            backList.push_back(&half);
                        }
                        backList.push_back(&backPoly);
                        break;
                    }
                    }
                }
                if (!frontList.empty() && !backList.empty()) {
                    Poly infinite = buildInfiniteFPoly(model, iNode);
                    infinite.brushPoly = iNode;
                    splitPartitioner(model, list, frontList, backList, 0, infinite, arena);
                }
                bool csg = isCsg(node, 0);
                if (!frontList.empty()) {
                    if (node.front >= 0) {
                        filterBound(model, &bound, node.front, frontList, outside || csg, arena);
                    } else if (outside || csg) {
                        for (Poly* poly : frontList) {
                            for (const Vector& v : poly->vertices) {
                                boxAdd(bound, v);
                            }
                        }
                    } else {
                        updateConvolutionWithPolys(model, iNode, frontList);
                    }
                }
                if (!backList.empty()) {
                    if (node.back >= 0) {
                        filterBound(model, &bound, node.back, backList, outside && !csg, arena);
                    } else if (outside && !csg) {
                        for (Poly* poly : backList) {
                            for (const Vector& v : poly->vertices) {
                                boxAdd(bound, v);
                            }
                        }
                    } else {
                        updateConvolutionWithPolys(model, iNode, backList);
                    }
                }
                BspNode& target = model.nodes[static_cast<size_t>(iNode)];
                if (target.renderBound == -1 && (target.front >= 0 || target.back >= 0)) {
                    target.renderBound = static_cast<int32_t>(model.nodeBounds.size());
                    model.nodeBounds.push_back(bound);
                }
                if (parentBound) {
                    boxAdd(*parentBound, bound);
                }
            }

            uint64_t buildZoneMasks(UModel& model, int32_t iNode)
            {
                BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                uint64_t mask = 0;
                if (node.zone[0] != 0) {
                    mask |= 1ull << node.zone[0];
                }
                if (node.zone[1] != 0) {
                    mask |= 1ull << node.zone[1];
                }
                int32_t front = node.front;
                int32_t back = node.back;
                int32_t plane = node.iPlane;
                if (front >= 0) {
                    mask |= buildZoneMasks(model, front);
                }
                if (back >= 0) {
                    mask |= buildZoneMasks(model, back);
                }
                if (plane >= 0) {
                    mask |= buildZoneMasks(model, plane);
                }
                model.nodes[static_cast<size_t>(iNode)].zoneMask = mask;
                return mask;
            }

            bool childOutside(const BspNode& node, int child, bool outside, uint8_t extraFlags)
            {
                return child ? (outside || isCsg(node, extraFlags)) : (outside && !isCsg(node, extraFlags));
            }

            float worldLightRadius(UObject* a)
            {
                return 25.0f * (static_cast<float>(actor::LightRadius(a)) + 1.0f);
            }

            float worldVolumetricRadius(UObject* a)
            {
                return 25.0f * (static_cast<float>(actor::VolumeRadius(a)) + 1.0f);
            }

            struct Portal {
                Poly poly;
                int32_t frontLeaf = -1;
                int32_t backLeaf = -1;
                int32_t node = -1;
                Portal* globalNext = nullptr;
                Portal* frontLeafNext = nullptr;
                Portal* backLeafNext = nullptr;
                Portal* nodeNext = nullptr;
                int32_t zonePortalSurf = -1;

                int32_t neighborOf(int32_t leaf) const
                {
                    return frontLeaf == leaf ? backLeaf : frontLeaf;
                }

                Portal* next(int32_t leaf) const
                {
                    return frontLeaf == leaf ? frontLeafNext : backLeafNext;
                }

                Poly facingOutOf(int32_t leaf) const
                {
                    Poly result = poly;
                    if (leaf == frontLeaf) {
                        reversePoly(result);
                    }
                    return result;
                }
            };

            class Visibility {
            public:
                using PortalFunc = void (Visibility::*)(Poly&, int32_t, int32_t, int32_t, int32_t);

                ULevel* level = nullptr;
                UModel* model = nullptr;
                std::vector<int32_t> clips;
                std::deque<Portal> portalStore;
                Portal* firstPortal = nullptr;
                std::vector<Portal*> nodePortals;
                std::vector<Portal*> leafPortals;
                std::vector<std::vector<UObject*>> leafLights;
                int32_t zonePortalSurf = -1;
                int numPortals = 0;
                int numZonePortals = 0;

                static constexpr int32_t ClipBackFlag = 0x40000000;

                void tagZonePortalFragment(Poly& poly, int32_t frontLeaf, int32_t backLeaf, int32_t generatingNode, int32_t generatingBase)
                {
                    int32_t newNode = addNode(*model, generatingNode, NodePlane, static_cast<uint8_t>(model->nodes[static_cast<size_t>(generatingNode)].nodeFlags | NF_IsNew), poly);
                    const BspNode& base = model->nodes[static_cast<size_t>(generatingBase)];
                    int backward = poly.normal.x * base.plane.x + poly.normal.y * base.plane.y + poly.normal.z * base.plane.z < 0.0f ? 1 : 0;
                    BspNode& node = model->nodes[static_cast<size_t>(newNode)];
                    node.zone[backward ^ 0] = static_cast<uint8_t>(backLeaf < 0 ? 0 : model->leaves[static_cast<size_t>(backLeaf)].zone);
                    node.zone[backward ^ 1] = static_cast<uint8_t>(frontLeaf < 0 ? 0 : model->leaves[static_cast<size_t>(frontLeaf)].zone);
                }

                void blockPortal(Poly&, int32_t frontLeaf, int32_t backLeaf, int32_t, int32_t)
                {
                    if (frontLeaf >= 0 && backLeaf >= 0) {
                        for (Portal* portal = firstPortal; portal; portal = portal->globalNext) {
                            if ((portal->frontLeaf == frontLeaf && portal->backLeaf == backLeaf) || (portal->frontLeaf == backLeaf && portal->backLeaf == frontLeaf)) {
                                portal->zonePortalSurf = zonePortalSurf;
                            }
                        }
                    }
                }

                void addPortal(Poly& poly, int32_t frontLeaf, int32_t backLeaf, int32_t generatingNode, int32_t)
                {
                    if (frontLeaf >= 0 && backLeaf >= 0) {
                        Portal& portal = portalStore.emplace_back();
                        portal.poly = poly;
                        portal.frontLeaf = frontLeaf;
                        portal.backLeaf = backLeaf;
                        portal.node = generatingNode;
                        portal.globalNext = firstPortal;
                        portal.nodeNext = nodePortals[static_cast<size_t>(generatingNode)];
                        portal.frontLeafNext = leafPortals[static_cast<size_t>(frontLeaf)];
                        portal.backLeafNext = leafPortals[static_cast<size_t>(backLeaf)];
                        firstPortal = &portal;
                        leafPortals[static_cast<size_t>(frontLeaf)] = &portal;
                        leafPortals[static_cast<size_t>(backLeaf)] = &portal;
                        nodePortals[static_cast<size_t>(generatingNode)] = &portal;
                        numPortals++;
                    }
                }

                void filterThroughSubtree(int pass, int32_t generatingNode, int32_t generatingBase, int32_t parentLeaf, int32_t iNode, Poly poly, PortalFunc func, int32_t backLeaf)
                {
                    while (iNode >= 0) {
                        if (static_cast<int>(poly.vertices.size()) > VertexThreshold) {
                            Poly half;
                            splitInHalf(poly, half);
                            filterThroughSubtree(pass, generatingNode, generatingBase, parentLeaf, iNode, half, func, backLeaf);
                        }
                        Poly front;
                        Poly back;
                        int split = splitWithNode(*model, poly, iNode, &front, &back, true);
                        if (split == SplitFront || split == SplitSplit) {
                            filterThroughSubtree(pass, generatingNode, generatingBase, model->nodes[static_cast<size_t>(iNode)].leaf[1], model->nodes[static_cast<size_t>(iNode)].front, split == SplitFront ? poly : front, func, backLeaf);
                        }
                        if (split != SplitBack && split != SplitSplit) {
                            return;
                        }
                        if (split == SplitSplit) {
                            poly = back;
                        }
                        parentLeaf = model->nodes[static_cast<size_t>(iNode)].leaf[0];
                        iNode = model->nodes[static_cast<size_t>(iNode)].back;
                    }
                    if (pass == 0) {
                        filterThroughSubtree(1, generatingNode, generatingBase, model->nodes[static_cast<size_t>(generatingBase)].leaf[1], model->nodes[static_cast<size_t>(generatingBase)].front, poly, func, parentLeaf);
                    } else {
                        (this->*func)(poly, parentLeaf, backLeaf, generatingNode, generatingBase);
                    }
                }

                void makePortalsClip(int32_t iNode, Poly poly, size_t clip, PortalFunc func)
                {
                    while (clip < clips.size()) {
                        int32_t clipNode = clips[clip] & ~ClipBackFlag;
                        if (static_cast<int>(poly.vertices.size()) >= VertexThreshold) {
                            Poly temp;
                            splitInHalf(poly, temp);
                            makePortalsClip(iNode, temp, clip, func);
                        }
                        Poly front;
                        Poly back;
                        int split = splitWithNode(*model, poly, clipNode, &front, &back, true);
                        bool isBack = (clips[clip] & ClipBackFlag) != 0;
                        if ((split == SplitFront && isBack) || (split == SplitBack && !isBack) || split == SplitCoplanar) {
                            return;
                        }
                        if (split == SplitSplit) {
                            poly = isBack ? back : front;
                        }
                        clip++;
                    }
                    filterThroughSubtree(0, iNode, iNode, model->nodes[static_cast<size_t>(iNode)].leaf[0], model->nodes[static_cast<size_t>(iNode)].back, poly, func, -1);
                }

                void makePortals(int32_t iNode)
                {
                    int32_t originalNode = iNode;
                    Poly poly = buildInfiniteFPoly(*model, iNode);
                    makePortalsClip(iNode, poly, 0, &Visibility::addPortal);
                    if (model->nodes[static_cast<size_t>(iNode)].front >= 0) {
                        clips.push_back(iNode);
                        makePortals(model->nodes[static_cast<size_t>(iNode)].front);
                        clips.pop_back();
                    }
                    if (model->nodes[static_cast<size_t>(iNode)].back >= 0) {
                        clips.push_back(iNode | ClipBackFlag);
                        makePortals(model->nodes[static_cast<size_t>(iNode)].back);
                        clips.pop_back();
                    }
                    while (iNode >= 0) {
                        int32_t surf = model->nodes[static_cast<size_t>(iNode)].surf;
                        int32_t nextNode = model->nodes[static_cast<size_t>(iNode)].iPlane;
                        Poly nodePoly;
                        if ((model->surfs[static_cast<size_t>(surf)].polyFlags & PF_Portal) && nodeToFPoly(*model, iNode, nodePoly)) {
                            numZonePortals++;
                            zonePortalSurf = surf;
                            filterThroughSubtree(0, iNode, originalNode, model->nodes[static_cast<size_t>(originalNode)].leaf[0], model->nodes[static_cast<size_t>(originalNode)].back, nodePoly, &Visibility::blockPortal, -1);
                        }
                        iNode = nextNode;
                    }
                }

                void assignLeaves(int32_t iNode, bool outside)
                {
                    for (int isFront = 0; isFront < 2; isFront++) {
                        const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                        int32_t child = isFront ? node.front : node.back;
                        bool childOut = childOutside(node, isFront, outside, NF_NotVisBlocking);
                        if (child >= 0) {
                            assignLeaves(child, childOut);
                        } else if (childOut) {
                            ConvexLeaf leaf;
                            leaf.zone = static_cast<int32_t>(model->leaves.size());
                            leaf.permeating = -1;
                            leaf.volumetric = -1;
                            leaf.visibleZones = ~0ull;
                            model->nodes[static_cast<size_t>(iNode)].leaf[isFront] = static_cast<int32_t>(model->leaves.size());
                            model->leaves.push_back(leaf);
                        }
                    }
                }

                void formZonesFromLeaves()
                {
                    for (Portal* portal = firstPortal; portal; portal = portal->globalNext) {
                        if (portal->zonePortalSurf == -1) {
                            int32_t original = model->leaves[static_cast<size_t>(portal->frontLeaf)].zone;
                            int32_t replacement = model->leaves[static_cast<size_t>(portal->backLeaf)].zone;
                            for (ConvexLeaf& leaf : model->leaves) {
                                if (leaf.zone == original) {
                                    leaf.zone = replacement;
                                }
                            }
                        }
                    }
                    int32_t numZones = 0;
                    for (size_t i = 0; i < model->leaves.size(); i++) {
                        if (model->leaves[i].zone >= numZones) {
                            for (size_t j = i + 1; j < model->leaves.size(); j++) {
                                if (model->leaves[j].zone == model->leaves[i].zone) {
                                    model->leaves[j].zone = numZones;
                                }
                            }
                            model->leaves[i].zone = numZones++;
                        }
                    }
                    for (ConvexLeaf& leaf : model->leaves) {
                        leaf.zone = (leaf.zone % 63) + 1;
                    }
                }

                void assignAllZones(int32_t iNode, bool outside)
                {
                    int32_t originalNode = iNode;
                    if (model->nodes[static_cast<size_t>(originalNode)].front >= 0) {
                        assignAllZones(model->nodes[static_cast<size_t>(originalNode)].front, outside || isCsg(model->nodes[static_cast<size_t>(originalNode)], NF_NotVisBlocking));
                    }
                    if (model->nodes[static_cast<size_t>(originalNode)].back >= 0) {
                        assignAllZones(model->nodes[static_cast<size_t>(originalNode)].back, outside && !isCsg(model->nodes[static_cast<size_t>(originalNode)], NF_NotVisBlocking));
                    }
                    while (iNode >= 0) {
                        Poly poly;
                        if (!(model->nodes[static_cast<size_t>(iNode)].nodeFlags & NF_IsNew) && nodeToFPoly(*model, iNode, poly)) {
                            size_t originalNumNodes = model->nodes.size();
                            filterThroughSubtree(0, iNode, originalNode, model->nodes[static_cast<size_t>(originalNode)].leaf[0], model->nodes[static_cast<size_t>(originalNode)].back, poly, &Visibility::tagZonePortalFragment, -1);
                            if (model->nodes.size() > originalNumNodes) {
                                bool canMerge = true;
                                uint8_t zones[2] = { 0, 0 };
                                for (size_t i = originalNumNodes; i < model->nodes.size(); i++) {
                                    for (int j = 0; j < 2; j++) {
                                        if (model->nodes[i].zone[j] != 0) {
                                            zones[j] = model->nodes[i].zone[j];
                                        }
                                    }
                                }
                                for (size_t i = originalNumNodes; i < model->nodes.size(); i++) {
                                    for (int j = 0; j < 2; j++) {
                                        if (model->nodes[i].zone[j] != 0 && model->nodes[i].zone[j] != zones[j]) {
                                            canMerge = false;
                                        }
                                    }
                                }
                                if (canMerge) {
                                    for (size_t i = originalNumNodes; i < model->nodes.size(); i++) {
                                        model->nodes[i].numVertices = 0;
                                    }
                                    model->nodes[static_cast<size_t>(iNode)].zone[0] = zones[0];
                                    model->nodes[static_cast<size_t>(iNode)].zone[1] = zones[1];
                                } else {
                                    model->nodes[static_cast<size_t>(iNode)].numVertices = 0;
                                    for (size_t i = originalNumNodes; i < model->nodes.size(); i++) {
                                        if (model->nodes[i].zone[0] == 0 && model->nodes[i].zone[1] == 0) {
                                            model->nodes[i].numVertices = 0;
                                        }
                                    }
                                }
                            }
                        }
                        iNode = model->nodes[static_cast<size_t>(iNode)].iPlane;
                    }
                }

                void buildConnectivity()
                {
                    for (size_t i = 0; i < 64; i++) {
                        model->zones[i].connectivity = 1ull << i;
                    }
                    for (const BspNode& node : model->nodes) {
                        if (model->surfs[static_cast<size_t>(node.surf)].polyFlags & PF_Portal) {
                            model->zones[node.zone[1] & 63].connectivity |= 1ull << (node.zone[0] & 63);
                            model->zones[node.zone[0] & 63].connectivity |= 1ull << (node.zone[1] & 63);
                        }
                    }
                }

                PointRegion pointRegion(UObject* levelInfo, const Vector& location) const
                {
                    PointRegion result;
                    result.zone = levelInfo;
                    result.iLeaf = -1;
                    result.zoneNumber = 0;
                    if (model->nodes.empty()) {
                        return result;
                    }
                    int32_t iNode = 0;
                    int32_t parent = 0;
                    int isFront = 0;
                    while (iNode >= 0) {
                        const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                        isFront = node.plane.x * location.x + node.plane.y * location.y + node.plane.z * location.z - node.plane.w >= 0.0f ? 1 : 0;
                        parent = iNode;
                        iNode = isFront ? node.front : node.back;
                    }
                    result.iLeaf = model->nodes[static_cast<size_t>(parent)].leaf[isFront];
                    result.zoneNumber = model->nodes[static_cast<size_t>(parent)].zone[isFront];
                    UObject* zoneActor = model->zones[result.zoneNumber & 63].zoneActor;
                    result.zone = zoneActor ? zoneActor : levelInfo;
                    return result;
                }

                void buildZoneInfo()
                {
                    for (ZoneProperties& zone : model->zones) {
                        zone.zoneActor = nullptr;
                    }
                    UObject* levelInfo = level->actors.empty() ? nullptr : level->actors[0];
                    UClass* zoneClass = objects::findClass("Engine.ZoneInfo");
                    UClass* levelInfoClass = objects::findClass("Engine.LevelInfo");
                    UClass* warpClass = objects::findClass("Engine.WarpZoneInfo");
                    for (UObject* a : level->actors) {
                        if (a && !a->destroyed && a->data) {
                            PointRegion& initial = dx::region(a);
                            initial.zone = levelInfo;
                            initial.iLeaf = -1;
                            initial.zoneNumber = 0;
                        }
                    }
                    for (UObject* a : level->actors) {
                        if (!a || a->destroyed || !a->data || !zoneClass || !a->isA(zoneClass) || (levelInfoClass && a->isA(levelInfoClass))) {
                            continue;
                        }
                        PointRegion region = pointRegion(levelInfo, actor::Location(a));
                        dx::region(a) = region;
                        if (region.zoneNumber == 0 || model->zones[region.zoneNumber & 63].zoneActor) {
                            continue;
                        }
                        model->zones[region.zoneNumber & 63].zoneActor = a;
                        if (zoneinfo::bReverbZone(a) && zoneinfo::bRaytraceReverb(a)) {
                            raytraceReverb(a);
                        }
                        if (warpClass && a->isA(warpClass) && region.iLeaf >= 0 && leafPortals[static_cast<size_t>(region.iLeaf)] && leafPortals[static_cast<size_t>(region.iLeaf)]->zonePortalSurf != -1) {
                            Portal* portal = leafPortals[static_cast<size_t>(region.iLeaf)];
                            BoundingBox box;
                            for (const BspNode& node : model->nodes) {
                                if (node.surf == portal->zonePortalSurf) {
                                    for (int j = 0; j < node.numVertices; j++) {
                                        boxAdd(box, model->points[static_cast<size_t>(model->verts[static_cast<size_t>(j + node.vertPool)].vertex)]);
                                    }
                                }
                            }
                            const BspSurf& surf = model->surfs[static_cast<size_t>(portal->zonePortalSurf)];
                            Coords& warp = warpzoneinfo::WarpCoords(a);
                            Vector center = box.min / 2.0f + box.max / 2.0f;
                            warp.origin = center - portal->poly.normal * (center - portal->poly.vertices[0]).dot(portal->poly.normal);
                            warp.zAxis = safeNormal(model->vectors[static_cast<size_t>(surf.vNormal)]);
                            warp.xAxis = safeNormal(model->vectors[static_cast<size_t>(surf.vTextureU)]);
                            warp.yAxis = safeNormal(model->vectors[static_cast<size_t>(surf.vTextureV)]);
                            warpzoneinfo::iWarpZone(a) = model->leaves[static_cast<size_t>(portal->neighborOf(region.iLeaf))].zone;
                        }
                    }
                    for (UObject* a : level->actors) {
                        if (a && !a->destroyed && a->data) {
                            dx::region(a) = pointRegion(levelInfo, actor::Location(a));
                        }
                    }
                }

                void raytraceReverb(UObject* a)
                {
                    const int numTraces = 256;
                    const float maxDist = 16384.0f;
                    std::vector<Vector> samples;
                    Vector origin = actor::Location(a);
                    for (int i = 0; i < numTraces; i++) {
                        Vector direction;
                        do {
                            direction.x = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
                            direction.y = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
                            direction.z = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
                        } while (direction.sizeSquared() > 1.0f);
                        direction = direction.normal();
                        float time = traceTime(origin + direction * maxDist, origin);
                        samples.push_back({ time * maxDist / zoneinfo::SpeedOfSound(a), 1.0f, 0.0f });
                    }
                    while (samples.size() > 6) {
                        size_t bestI = 0;
                        size_t bestJ = 1;
                        double bestCost = 10000000000000.0;
                        for (size_t i = 0; i < samples.size(); i++) {
                            for (size_t j = 0; j < i; j++) {
                                double cost = static_cast<double>((samples[i].x - samples[j].x) * (samples[i].x - samples[j].x) * (samples[i].y + samples[j].y));
                                if (cost < bestCost) {
                                    bestCost = cost;
                                    bestI = i;
                                    bestJ = j;
                                }
                            }
                        }
                        Vector first = samples[bestI];
                        Vector second = samples[bestJ];
                        samples[bestI] = { (first.x * first.y + second.x * second.y) / (first.y + second.y), first.y + second.y, 0.0f };
                        samples.erase(samples.begin() + static_cast<std::ptrdiff_t>(bestJ));
                    }
                    for (size_t i = 0; i < samples.size(); i++) {
                        for (size_t j = 0; j < i; j++) {
                            if (samples[i].x < samples[j].x) {
                                std::swap(samples[i], samples[j]);
                            }
                        }
                    }
                    for (size_t i = 0; i < samples.size(); i++) {
                        zoneinfo::Delay(a, static_cast<int>(i)) = static_cast<uint8_t>(std::clamp(samples[i].x * 1000.0f, 0.0f, 255.0f));
                        zoneinfo::Gain(a, static_cast<int>(i)) = static_cast<uint8_t>(std::clamp(samples[i].y * 255.0f / static_cast<float>(numTraces), 0.0f, 255.0f));
                    }
                }

                float traceTime(const Vector& end, const Vector& start)
                {
                    float hitTime = 1.0f;
                    Vector hitLocation;
                    if (!zeroExtentTrace(end, start, hitLocation)) {
                        Vector v = end - start;
                        hitTime = (hitLocation - start).dot(v) / v.dot(v);
                        hitTime = std::clamp(hitTime - 0.5f / v.size(), 0.0f, 1.0f);
                    }
                    return hitTime;
                }

                bool zeroExtentTrace(Vector end, Vector start, Vector& hit)
                {
                    return lineWalk(0, end, start, model->rootOutside != 0, hit);
                }

                bool lineWalk(int32_t iNode, Vector end, Vector start, bool outside, Vector& hit)
                {
                    while (iNode >= 0) {
                        const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                        float dist1 = node.plane.x * start.x + node.plane.y * start.y + node.plane.z * start.z - node.plane.w;
                        float dist2 = node.plane.x * end.x + node.plane.y * end.y + node.plane.z * end.z - node.plane.w;
                        if (dist1 > -0.001f && dist2 > -0.001f) {
                            outside = outside || isCsg(node, 0);
                            iNode = node.front;
                        } else if (dist1 < 0.001f && dist2 < 0.001f) {
                            outside = outside && !isCsg(node, 0);
                            iNode = node.back;
                        } else {
                            Vector middle = start + (start - end) * (dist1 / (dist2 - dist1));
                            int frontFirst = dist1 > 0.0f ? 1 : 0;
                            if (!lineWalk(frontFirst ? node.front : node.back, middle, start, childOutside(node, frontFirst, outside, 0), hit)) {
                                return false;
                            }
                            outside = childOutside(node, 1 - frontFirst, outside, 0);
                            iNode = frontFirst ? node.back : node.front;
                            start = middle;
                        }
                    }
                    if (!outside) {
                        hit = start;
                    }
                    return outside;
                }

                int actorVisibility(UObject* a, int32_t iLeaf, const Poly* clipper)
                {
                    Vector location = actor::Location(a);
                    float radius = worldLightRadius(a);
                    if (iLeaf < 0) {
                        int32_t iNode = 0;
                        int32_t parent = 0;
                        int isFront = 0;
                        while (iNode >= 0) {
                            const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                            isFront = node.plane.x * location.x + node.plane.y * location.y + node.plane.z * location.z - node.plane.w > 0.0f ? 1 : 0;
                            parent = iNode;
                            iNode = isFront ? node.front : node.back;
                        }
                        iLeaf = model->nodes[static_cast<size_t>(parent)].leaf[isFront];
                        if (iLeaf < 0) {
                            return 0;
                        }
                    } else {
                        bool near = false;
                        for (Portal* portal = leafPortals[static_cast<size_t>(iLeaf)]; portal && !near; portal = portal->next(iLeaf)) {
                            Poly poly = portal->facingOutOf(iLeaf);
                            for (const Vector& v : poly.vertices) {
                                if ((v - location).sizeSquared() < radius * radius) {
                                    near = true;
                                    break;
                                }
                            }
                        }
                        if (!near) {
                            return 0;
                        }
                    }
                    int count = 0;
                    std::vector<UObject*>& list = leafLights[static_cast<size_t>(iLeaf)];
                    if (std::find(list.begin(), list.end(), a) == list.end()) {
                        list.insert(list.begin(), a);
                        count++;
                    }
                    for (Portal* portal = leafPortals[static_cast<size_t>(iLeaf)]; portal; portal = portal->next(iLeaf)) {
                        Poly poly = portal->facingOutOf(iLeaf);
                        float planeDot = (location - poly.base).dot(poly.normal);
                        if (planeDot < 0.0f && planeDot > -radius) {
                            int32_t otherLeaf = portal->neighborOf(iLeaf);
                            bool oblivion = false;
                            if (clipper) {
                                int n = static_cast<int>(clipper->vertices.size());
                                for (int i = 0, j = n - 1; i < n; j = i++) {
                                    if (static_cast<int>(poly.vertices.size()) >= VertexThreshold) {
                                        break;
                                    }
                                    Poly front;
                                    Poly back;
                                    int split = splitWithPlaneFast(poly, makePlane(location, clipper->vertices[static_cast<size_t>(i)], clipper->vertices[static_cast<size_t>(j)]), &front, &back);
                                    if (split == SplitBack) {
                                        oblivion = true;
                                        break;
                                    } else if (split == SplitSplit) {
                                        poly = front;
                                    }
                                }
                            }
                            if (!oblivion && !poly.vertices.empty()) {
                                count += actorVisibility(a, otherLeaf, &poly);
                            }
                        }
                    }
                    return count;
                }

                void filterVolumetricLight(UObject* a, int32_t iNode, int32_t parent, int isFront)
                {
                    if (iNode >= 0) {
                        const BspNode& node = model->nodes[static_cast<size_t>(iNode)];
                        Vector location = actor::Location(a);
                        float dist = node.plane.x * location.x + node.plane.y * location.y + node.plane.z * location.z - node.plane.w;
                        float radius = worldVolumetricRadius(a);
                        int32_t front = node.front;
                        int32_t back = node.back;
                        if (dist > -radius) {
                            filterVolumetricLight(a, front, iNode, 1);
                        }
                        if (dist < radius) {
                            filterVolumetricLight(a, back, iNode, 0);
                        }
                    } else if (model->nodes[static_cast<size_t>(parent)].leaf[isFront] >= 0) {
                        std::vector<UObject*>& list = leafLights[static_cast<size_t>(model->nodes[static_cast<size_t>(parent)].leaf[isFront])];
                        list.insert(list.begin(), a);
                    }
                }

                void run()
                {
                    for (BspNode& node : model->nodes) {
                        node.leaf[0] = -1;
                        node.leaf[1] = -1;
                        node.zone[0] = 0;
                        node.zone[1] = 0;
                    }
                    for (BspSurf& surf : model->surfs) {
                        surf.lightMap = -1;
                    }
                    model->leaves.clear();
                    model->lights.clear();
                    assignLeaves(0, model->rootOutside != 0);
                    leafPortals.assign(model->leaves.size(), nullptr);
                    leafLights.assign(model->leaves.size(), {});
                    nodePortals.assign(model->nodes.size() * 2 + 256, nullptr);
                    makePortals(0);
                    formZonesFromLeaves();
                    assignAllZones(0, model->rootOutside != 0);
                    cleanup(*model);
                    refresh(*model, true);
                    buildBounds(*model);
                    buildZoneMasks(*model, 0);
                    buildConnectivity();
                    buildZoneInfo();
                    for (UObject* a : level->actors) {
                        if (a && !a->destroyed && a->data && actor::LightType(a) != 0 && (actor::bStatic(a) || actor::bNoDelete(a))) {
                            actorVisibility(a, -1, nullptr);
                        }
                    }
                    for (size_t i = 0; i < model->leaves.size(); i++) {
                        if (!leafLights[i].empty()) {
                            model->leaves[i].permeating = static_cast<int32_t>(model->lights.size());
                            model->lights.insert(model->lights.end(), leafLights[i].begin(), leafLights[i].end());
                            model->lights.push_back(nullptr);
                        }
                    }
                    for (std::vector<UObject*>& list : leafLights) {
                        list.clear();
                    }
                    for (UObject* a : level->actors) {
                        if (!a || a->destroyed || !a->data) {
                            continue;
                        }
                        UObject* zone = region(a).zone;
                        if (zone && zone->data && zoneinfo::bFogZone(zone) && actor::LightType(a) != 0 && actor::VolumeRadius(a) && actor::VolumeBrightness(a) && (actor::bStatic(a) || actor::bNoDelete(a))) {
                            filterVolumetricLight(a, 0, 0, 0);
                        }
                    }
                    for (size_t i = 0; i < model->leaves.size(); i++) {
                        if (!leafLights[i].empty()) {
                            model->leaves[i].volumetric = static_cast<int32_t>(model->lights.size());
                            model->lights.insert(model->lights.end(), leafLights[i].begin(), leafLights[i].end());
                            model->lights.push_back(nullptr);
                        }
                    }
                    int32_t numZones = 1;
                    for (const ConvexLeaf& leaf : model->leaves) {
                        numZones = std::max(numZones, leaf.zone + 1);
                    }
                    for (size_t i = static_cast<size_t>(std::min(numZones, 64)); i < model->zones.size(); i++) {
                        model->zones[i] = ZoneProperties();
                    }
                }
            };

            std::vector<UObject*> staticBrushes(ULevel* level)
            {
                std::vector<UObject*> result;
                UClass* brushClass = objects::findClass("Engine.Brush");
                UObject* builder = level->actors.size() > 1 ? level->actors[1] : nullptr;
                for (UObject* a : level->actors) {
                    if (!a || a->destroyed || !a->data || a == builder || !brushClass || !a->isA(brushClass) || !actor::Brush(a) || !actor::bStatic(a)) {
                        continue;
                    }
                    result.push_back(a);
                }
                return result;
            }

            void enlistLeaves(const UModel& model, std::vector<int32_t>& fronts, std::vector<int32_t>& backs, int32_t iNode)
            {
                const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                if (node.front < 0) {
                    fronts.push_back(iNode);
                } else {
                    enlistLeaves(model, fronts, backs, node.front);
                }
                if (node.back < 0) {
                    backs.push_back(iNode);
                } else {
                    enlistLeaves(model, fronts, backs, node.back);
                }
            }
        }

        void setCurrentTexture(UObject* texture)
        {
            currentTextureObject = texture;
        }

        UObject* currentTexture()
        {
            return currentTextureObject;
        }

        void ensureLevelPolys(UModel& model)
        {
            ensurePolys(model);
        }

        int32_t vectorIndex(UModel& model, const Vector& v, bool normal)
        {
            return addVector(model, v, normal);
        }

        int32_t pointIndex(UModel& model, const Vector& v, bool exact)
        {
            return addPoint(model, v, exact);
        }

        bool isCsg(const BspNode& node, uint8_t extraFlags)
        {
            return node.numVertices > 0 && !(node.nodeFlags & (NF_IsNew | NF_NotCsg | extraFlags));
        }

        Vector transformVector(const Vector& v, const CoordSystem& coords)
        {
            return { v.dot(coords.xAxis), v.dot(coords.yAxis), v.dot(coords.zAxis) };
        }

        float buildCoords(UObject* brushActor, ModelCoords* coords, ModelCoords* uncoords)
        {
            Scale mainScale = readScale(brush::MainScale(brushActor));
            Scale postScale = readScale(brush::PostScale(brushActor));
            Rotator rotation = actor::Rotation(brushActor);
            CoordSystem forward;
            mulScale(forward, postScale);
            mulRotator(forward, rotation);
            mulScale(forward, mainScale);
            CoordSystem backward;
            divScale(backward, mainScale);
            divRotator(backward, rotation);
            divScale(backward, postScale);
            if (coords) {
                coords->pointXform = forward;
                coords->vectorXform = transpose(backward);
            }
            if (uncoords) {
                uncoords->pointXform = backward;
                uncoords->vectorXform = transpose(forward);
            }
            return sign(mainScale.scale.x * mainScale.scale.y * mainScale.scale.z) * sign(postScale.scale.x * postScale.scale.y * postScale.scale.z);
        }

        void transformPoly(Poly& poly, const ModelCoords& coords, const Vector& preSubtract, const Vector& postAdd, float orientation)
        {
            poly.textureU = transformVector(poly.textureU, coords.vectorXform);
            poly.textureV = transformVector(poly.textureV, coords.vectorXform);
            poly.base = transformVector(poly.base - preSubtract, coords.pointXform) + postAdd;
            for (Vector& v : poly.vertices) {
                v = transformVector(v - preSubtract, coords.pointXform) + postAdd;
            }
            if (orientation < 0.0f) {
                std::reverse(poly.vertices.begin(), poly.vertices.end());
            }
            poly.normal = safeNormal(transformVector(poly.normal, coords.vectorXform));
        }

        void reversePoly(Poly& poly)
        {
            poly.normal = poly.normal * -1.0f;
            std::reverse(poly.vertices.begin(), poly.vertices.end());
        }

        int fixPoly(Poly& poly)
        {
            int n = static_cast<int>(poly.vertices.size());
            int j = 0;
            int prev = n - 1;
            for (int i = 0; i < n; i++) {
                if (!pointsAreSame(poly.vertices[static_cast<size_t>(i)], poly.vertices[static_cast<size_t>(prev)])) {
                    if (j != i) {
                        poly.vertices[static_cast<size_t>(j)] = poly.vertices[static_cast<size_t>(i)];
                    }
                    prev = j;
                    j++;
                }
            }
            if (j >= 3) {
                poly.vertices.resize(static_cast<size_t>(j));
            } else {
                poly.vertices.clear();
            }
            return static_cast<int>(poly.vertices.size());
        }

        bool calcNormal(Poly& poly)
        {
            poly.normal = {};
            for (size_t i = 2; i < poly.vertices.size(); i++) {
                poly.normal += (poly.vertices[i - 1] - poly.vertices[0]).cross(poly.vertices[i] - poly.vertices[0]);
            }
            if (poly.normal.sizeSquared() < ThreshZeroNormSquared) {
                return false;
            }
            normalize(poly.normal);
            return true;
        }

        int finalizePoly(Poly& poly)
        {
            fixPoly(poly);
            if (poly.vertices.size() < 3) {
                return -1;
            }
            if (poly.normal.zero()) {
                if (!calcNormal(poly)) {
                    return -1;
                }
            }
            if (poly.textureU.zero() && poly.textureV.zero()) {
                for (size_t i = 1; i < poly.vertices.size(); i++) {
                    poly.textureU = safeNormal((poly.vertices[0] - poly.vertices[i]).cross(poly.normal));
                    poly.textureV = safeNormal(poly.normal.cross(poly.textureU));
                    if (poly.textureU.sizeSquared() != 0 && poly.textureV.sizeSquared() != 0) {
                        break;
                    }
                }
            }
            return 0;
        }

        bool removeColinears(Poly& poly)
        {
            std::vector<Vector>& v = poly.vertices;
            std::vector<Vector> sidePlaneNormal(v.size() + 1);
            for (int i = 0; i < static_cast<int>(v.size()); i++) {
                int j = i - 1;
                if (j < 0) {
                    j = static_cast<int>(v.size()) - 1;
                }
                Vector side = v[static_cast<size_t>(i)] - v[static_cast<size_t>(j)];
                sidePlaneNormal[static_cast<size_t>(i)] = side.cross(poly.normal);
                if (!normalize(sidePlaneNormal[static_cast<size_t>(i)])) {
                    v.erase(v.begin() + i);
                    if (v.size() < 3) {
                        v.clear();
                        return false;
                    }
                    i--;
                }
            }
            for (int i = 0; i < static_cast<int>(v.size()); i++) {
                int j = i + 1;
                if (j >= static_cast<int>(v.size())) {
                    j = 0;
                }
                if (pointsAreNear(sidePlaneNormal[static_cast<size_t>(i)], sidePlaneNormal[static_cast<size_t>(j)], FloatNormalThresh)) {
                    v.erase(v.begin() + i);
                    sidePlaneNormal.erase(sidePlaneNormal.begin() + i);
                    if (v.size() < 3) {
                        v.clear();
                        return false;
                    }
                    i--;
                } else {
                    for (int k = 0; k < static_cast<int>(v.size()); k++) {
                        if (k != i) {
                            switch (splitWithPlane(poly, v[static_cast<size_t>(i)], sidePlaneNormal[static_cast<size_t>(i)], nullptr, nullptr, false)) {
                            case SplitFront:
                            case SplitSplit:
                                return false;
                            default:
                                break;
                            }
                        }
                    }
                }
            }
            return true;
        }

        int splitWithPlane(const Poly& poly, const Vector& base, const Vector& normal, Poly* frontPoly, Poly* backPoly, bool precise)
        {
            enum Status {
                Front,
                Back,
                Either,
            };

            float thresh = precise ? ThreshSplitPolyPrecisely : ThreshSplitPolyWithPlane;
            const std::vector<Vector>& vertex = poly.vertices;
            int n = static_cast<int>(vertex.size());
            float dist = 0;
            float maxDist = 0;
            float minDist = 0;
            Status prevStatus = Either;
            for (int i = 0; i < n; i++) {
                dist = pointPlaneDist(vertex[static_cast<size_t>(i)], base, normal);
                if (i == 0 || dist > maxDist) {
                    maxDist = dist;
                }
                if (i == 0 || dist < minDist) {
                    minDist = dist;
                }
                if (dist > thresh) {
                    prevStatus = Front;
                } else if (dist < -thresh) {
                    prevStatus = Back;
                }
            }
            if (maxDist < thresh && minDist > -thresh) {
                return SplitCoplanar;
            } else if (maxDist < thresh) {
                return SplitBack;
            } else if (minDist > -thresh) {
                return SplitFront;
            }
            if (!frontPoly) {
                return SplitSplit;
            }
            Poly front = poly;
            front.polyFlags |= PF_EdCut;
            front.vertices.clear();
            Poly back = poly;
            back.polyFlags |= PF_EdCut;
            back.vertices.clear();
            int j = n - 1;
            for (int i = 0; i < n; i++) {
                float prevDist = dist;
                dist = pointPlaneDist(vertex[static_cast<size_t>(i)], base, normal);
                Status status = prevStatus;
                if (dist > thresh) {
                    status = Front;
                } else if (dist < -thresh) {
                    status = Back;
                }
                if (status != prevStatus) {
                    if (dist >= -thresh && dist < thresh) {
                        if (prevStatus == Front) {
                            front.vertices.push_back(vertex[static_cast<size_t>(i)]);
                            back.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        } else {
                            back.vertices.push_back(vertex[static_cast<size_t>(i)]);
                            front.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        }
                    } else if (prevDist >= -thresh && prevDist < thresh) {
                        if (status == Front) {
                            front.vertices.push_back(vertex[static_cast<size_t>(j)]);
                            front.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        } else {
                            back.vertices.push_back(vertex[static_cast<size_t>(j)]);
                            back.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        }
                    } else {
                        Vector intersection = linePlaneIntersection(vertex[static_cast<size_t>(j)], vertex[static_cast<size_t>(i)], base, normal);
                        if (prevStatus == Front) {
                            front.vertices.push_back(intersection);
                            back.vertices.push_back(intersection);
                            back.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        } else {
                            back.vertices.push_back(intersection);
                            front.vertices.push_back(intersection);
                            front.vertices.push_back(vertex[static_cast<size_t>(i)]);
                        }
                    }
                } else {
                    if (status == Front) {
                        front.vertices.push_back(vertex[static_cast<size_t>(i)]);
                    } else {
                        back.vertices.push_back(vertex[static_cast<size_t>(i)]);
                    }
                }
                j = i;
                prevStatus = status;
            }
            *frontPoly = front;
            *backPoly = back;
            if (fixPoly(*frontPoly) < 3) {
                return SplitBack;
            } else if (fixPoly(*backPoly) < 3) {
                return SplitFront;
            }
            return SplitSplit;
        }

        int splitWithPlaneFast(const Poly& poly, const Plane& plane, Poly* frontPoly, Poly* backPoly)
        {
            const std::vector<Vector>& vertex = poly.vertices;
            int n = static_cast<int>(vertex.size());
            std::vector<int> status(static_cast<size_t>(n));
            bool front = false;
            bool back = false;
            for (int i = 0; i < n; i++) {
                float dist = planeDot(plane, vertex[static_cast<size_t>(i)]);
                if (dist >= 0.0f) {
                    status[static_cast<size_t>(i)] = 0;
                    if (dist > ThreshSplitPolyWithPlane) {
                        front = true;
                    }
                } else {
                    status[static_cast<size_t>(i)] = 1;
                    if (dist < -ThreshSplitPolyWithPlane) {
                        back = true;
                    }
                }
            }
            if (!front) {
                return back ? SplitBack : SplitCoplanar;
            }
            if (!back) {
                return SplitFront;
            }
            if (frontPoly) {
                Poly f = poly;
                Poly b = poly;
                f.vertices.clear();
                b.vertices.clear();
                int prevStatus = status[static_cast<size_t>(n - 1)];
                size_t w = static_cast<size_t>(n - 1);
                for (int i = 0; i < n; i++) {
                    int s = status[static_cast<size_t>(i)];
                    const Vector& v = vertex[static_cast<size_t>(i)];
                    if (s != prevStatus) {
                        Vector intersection = linePlaneIntersection(vertex[w], v, plane);
                        f.vertices.push_back(intersection);
                        b.vertices.push_back(intersection);
                        if (prevStatus == 0) {
                            b.vertices.push_back(v);
                        } else {
                            f.vertices.push_back(v);
                        }
                    } else if (s == 0) {
                        f.vertices.push_back(v);
                    } else {
                        b.vertices.push_back(v);
                    }
                    prevStatus = s;
                    w = static_cast<size_t>(i);
                }
                *frontPoly = f;
                *backPoly = b;
            }
            return SplitSplit;
        }

        void emptyModel(UModel& model, bool surfInfo, bool polys)
        {
            model.nodes.clear();
            model.nodeBounds.clear();
            model.leafHulls.clear();
            model.leaves.clear();
            model.lights.clear();
            model.lightMaps.clear();
            model.lightBits.clear();
            if (polys && model.polys) {
                model.polys->polys.clear();
            }
            if (surfInfo) {
                model.vectors.clear();
                model.points.clear();
                model.surfs.clear();
            }
            model.verts.clear();
            model.numSharedSides = 4;
        }

        void buildBound(UModel& model)
        {
            std::vector<Vector> points;
            if (model.polys) {
                for (const Poly& poly : model.polys->polys) {
                    points.insert(points.end(), poly.vertices.begin(), poly.vertices.end());
                }
            }
            BoundingBox box;
            for (const Vector& p : points) {
                boxAdd(box, p);
            }
            model.bounds = box;
            if (!points.empty()) {
                Vector center = (box.min + box.max) / 2.0f;
                float w = 0;
                for (const Vector& p : points) {
                    float d = (p - center).sizeSquared();
                    if (d > w) {
                        w = d;
                    }
                }
                model.sphere = { center.x, center.y, center.z, static_cast<float>(static_cast<double>(std::sqrt(w)) * 1.001) };
            }
        }

        int32_t addNode(UModel& model, int32_t parent, NodePlace place, uint8_t nodeFlags, Poly& poly)
        {
            if (place == NodePlane) {
                while (model.nodes[static_cast<size_t>(parent)].iPlane >= 0) {
                    parent = model.nodes[static_cast<size_t>(parent)].iPlane;
                }
            }
            int32_t surfIndex = poly.link;
            if (poly.link == static_cast<int32_t>(model.surfs.size())) {
                BspSurf surf;
                surf.pBase = addPoint(model, poly.base, true);
                surf.vNormal = addVector(model, poly.normal, true);
                surf.vTextureU = addVector(model, poly.textureU, false);
                surf.vTextureV = addVector(model, poly.textureV, false);
                surf.texture = poly.texture;
                surf.lightMap = -1;
                surf.panU = poly.panU;
                surf.panV = poly.panV;
                surf.polyFlags = poly.polyFlags & ~PF_NoAddToBSP;
                surf.actor = poly.actor;
                surf.brushPoly = poly.brushPoly;
                model.surfs.push_back(surf);
            }
            const BspSurf& surf = model.surfs[static_cast<size_t>(surfIndex)];
            if (surf.polyFlags & PF_NotSolid) {
                nodeFlags |= NF_NotCsg;
            }
            if (surf.polyFlags & (PF_Invisible | PF_Portal)) {
                nodeFlags |= NF_NotVisBlocking;
            }
            if (surf.polyFlags & PF_Masked) {
                nodeFlags |= NF_ShootThrough;
            }
            int n = static_cast<int>(poly.vertices.size());
            if (n > MaxNodeVertices) {
                Poly poly1 = poly;
                poly1.vertices.resize(static_cast<size_t>(MaxNodeVertices));
                Poly poly2 = poly;
                poly2.vertices.clear();
                poly2.vertices.push_back(poly.vertices[0]);
                for (int i = MaxNodeVertices - 1; i < n; i++) {
                    poly2.vertices.push_back(poly.vertices[static_cast<size_t>(i)]);
                }
                int32_t iNode = addNode(model, parent, place, nodeFlags, poly1);
                addNode(model, iNode, NodePlane, nodeFlags, poly2);
                return iNode;
            }
            int32_t iNode = static_cast<int32_t>(model.nodes.size());
            model.nodes.emplace_back();
            BspNode node;
            node.surf = poly.link;
            node.nodeFlags = nodeFlags;
            node.renderBound = -1;
            node.collisionBound = -1;
            node.zoneMask = place != NodeRoot ? model.nodes[static_cast<size_t>(parent)].zoneMask : ~0ull;
            node.plane = { poly.normal.x, poly.normal.y, poly.normal.z, poly.base.dot(poly.normal) };
            node.vertPool = static_cast<int32_t>(model.verts.size());
            model.verts.resize(model.verts.size() + static_cast<size_t>(n));
            node.front = -1;
            node.back = -1;
            node.iPlane = -1;
            if (place == NodeRoot) {
                node.leaf[0] = -1;
                node.leaf[1] = -1;
                node.zone[0] = 0;
                node.zone[1] = 0;
            } else if (place == NodeFront || place == NodeBack) {
                int zoneFront = place == NodeFront ? 1 : 0;
                const BspNode& p = model.nodes[static_cast<size_t>(parent)];
                node.leaf[0] = p.leaf[zoneFront];
                node.leaf[1] = p.leaf[zoneFront];
                node.zone[0] = p.zone[zoneFront];
                node.zone[1] = p.zone[zoneFront];
            } else {
                const BspNode& p = model.nodes[static_cast<size_t>(parent)];
                int flipped = node.plane.x * p.plane.x + node.plane.y * p.plane.y + node.plane.z * p.plane.z < 0.0f ? 1 : 0;
                node.leaf[0] = p.leaf[flipped];
                node.leaf[1] = p.leaf[1 - flipped];
                node.zone[0] = p.zone[flipped];
                node.zone[1] = p.zone[1 - flipped];
            }
            if (place == NodeFront) {
                model.nodes[static_cast<size_t>(parent)].front = iNode;
            } else if (place == NodeBack) {
                model.nodes[static_cast<size_t>(parent)].back = iNode;
            } else if (place == NodePlane) {
                model.nodes[static_cast<size_t>(parent)].iPlane = iNode;
            }
            node.numVertices = 0;
            model.nodes[static_cast<size_t>(iNode)] = node;
            for (int i = 0; i < n; i++) {
                int32_t pVertex = addPoint(model, poly.vertices[static_cast<size_t>(i)], false);
                BspNode& current = model.nodes[static_cast<size_t>(iNode)];
                if (current.numVertices == 0 || model.verts[static_cast<size_t>(current.vertPool + current.numVertices - 1)].vertex != pVertex) {
                    BspVert& vert = model.verts[static_cast<size_t>(current.vertPool + current.numVertices)];
                    vert.side = -1;
                    vert.vertex = pVertex;
                    current.numVertices++;
                }
            }
            BspNode& done = model.nodes[static_cast<size_t>(iNode)];
            if (done.numVertices >= 2 && model.verts[static_cast<size_t>(done.vertPool)].vertex == model.verts[static_cast<size_t>(done.vertPool + done.numVertices - 1)].vertex) {
                done.numVertices--;
            }
            if (done.numVertices < 3) {
                errors++;
                done.numVertices = 0;
            }
            return iNode;
        }

        void build(UModel& model, Optimization opt, int balance, int rebuildSimplePolys)
        {
            if (rebuildSimplePolys == 1) {
                emptyModel(model, true, false);
            } else if (rebuildSimplePolys == 0) {
                for (BspNode& node : model.nodes) {
                    node.numVertices = 0;
                }
                refresh(model, true);
                emptyModel(model, false, false);
            }
            if (!model.polys || model.polys->polys.empty()) {
                return;
            }
            std::vector<Poly*> list;
            for (Poly& poly : model.polys->polys) {
                if (!poly.vertices.empty()) {
                    list.push_back(&poly);
                }
            }
            if (list.empty()) {
                return;
            }
            std::deque<Poly> arena;
            splitPolyList(model, -1, NodeRoot, list, opt, balance, rebuildSimplePolys, arena);
            if (rebuildSimplePolys == 0) {
                refresh(model, true);
                buildBounds(model);
            }
        }

        int nodeToFPoly(const UModel& model, int32_t iNode, Poly& poly)
        {
            const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
            const BspSurf& surf = model.surfs[static_cast<size_t>(node.surf)];
            poly = emptyPoly();
            poly.base = model.points[static_cast<size_t>(surf.pBase)];
            poly.normal = model.vectors[static_cast<size_t>(surf.vNormal)];
            poly.polyFlags = surf.polyFlags & ~(PF_EdCut | PF_EdProcessed | PF_Selected | PF_Memorized);
            poly.link = node.surf;
            poly.texture = surf.texture;
            poly.actor = surf.actor;
            poly.brushPoly = surf.brushPoly;
            poly.panU = surf.panU;
            poly.panV = surf.panV;
            UModel* master = surf.actor && surf.actor->data ? cast<UModel>(actor::Brush(surf.actor)) : nullptr;
            if (master && master->polys && surf.brushPoly >= 0 && static_cast<size_t>(surf.brushPoly) < master->polys->polys.size()) {
                poly.itemName = master->polys->polys[static_cast<size_t>(surf.brushPoly)].itemName;
            }
            poly.textureU = model.vectors[static_cast<size_t>(surf.vTextureU)];
            poly.textureV = model.vectors[static_cast<size_t>(surf.vTextureV)];
            for (int i = 0; i < node.numVertices; i++) {
                poly.vertices.push_back(model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)]);
            }
            if (poly.vertices.size() < 3) {
                poly.vertices.clear();
            }
            removeColinears(poly);
            return static_cast<int>(poly.vertices.size());
        }

        void buildFPolys(UModel& model, bool surfLinks, int32_t iNode)
        {
            UPolys* polys = ensurePolys(model);
            if (!polys) {
                return;
            }
            polys->polys.clear();
            if (!model.nodes.empty()) {
                makeEdPolys(model, iNode, polys->polys);
            }
            if (!surfLinks) {
                for (size_t i = 0; i < polys->polys.size(); i++) {
                    polys->polys[i].link = static_cast<int32_t>(i);
                }
            }
        }

        void mergeCoplanars(UModel& model, bool remapLinks, bool mergeDisparateTextures)
        {
            if (!model.polys) {
                return;
            }
            std::vector<Poly>& polys = model.polys->polys;
            for (Poly& poly : polys) {
                poly.polyFlags &= ~PF_EdProcessed;
            }
            for (size_t i = 0; i < polys.size(); i++) {
                Poly& edPoly = polys[i];
                if (edPoly.vertices.empty() || (edPoly.polyFlags & PF_EdProcessed)) {
                    continue;
                }
                std::vector<int32_t> list;
                list.push_back(static_cast<int32_t>(i));
                edPoly.polyFlags |= PF_EdProcessed;
                for (size_t j = i + 1; j < polys.size(); j++) {
                    Poly& other = polys[j];
                    if (other.link == edPoly.link) {
                        float dist = (other.base - edPoly.base).dot(edPoly.normal);
                        if (dist > -0.001f && dist < 0.001f && other.normal.dot(edPoly.normal) > 0.9999f && (mergeDisparateTextures || (pointsAreNear(other.textureU, edPoly.textureU, ThreshVectorsAreNear) && pointsAreNear(other.textureV, edPoly.textureV, ThreshVectorsAreNear)))) {
                            other.polyFlags |= PF_EdProcessed;
                            list.push_back(static_cast<int32_t>(j));
                        }
                    }
                }
                if (list.size() > 1) {
                    mergeCoplanarList(polys, list);
                }
            }
            std::vector<int32_t> remap(polys.size(), 0);
            size_t j = 0;
            for (size_t i = 0; i < polys.size(); i++) {
                if (!polys[i].vertices.empty()) {
                    remap[i] = static_cast<int32_t>(j);
                    if (j != i) {
                        polys[j] = polys[i];
                    }
                    j++;
                }
            }
            polys.resize(j);
            if (remapLinks) {
                for (Poly& poly : polys) {
                    if (poly.link != -1) {
                        poly.link = remap[static_cast<size_t>(poly.link)];
                    }
                }
            }
        }

        void cleanup(UModel& model)
        {
            if (!model.nodes.empty()) {
                cleanupNodes(model, 0, -1);
            }
        }

        int brushCSG(UObject* brushActor, UModel& model, uint32_t polyFlags, uint8_t oper, bool buildBoundsFlag)
        {
            errors = 0;
            UModel* brush = cast<UModel>(actor::Brush(brushActor));
            if (!brush) {
                return 0;
            }
            objects::ensureLoaded(brush);
            objects::flushLoads();
            if (brush->polys) {
                objects::ensureLoaded(brush->polys);
                objects::flushLoads();
            }
            uint32_t notPolyFlags = 0;
            if (oper != CsgAdd) {
                notPolyFlags |= PF_Semisolid | PF_NotSolid;
            }
            UModel tempModel;
            UPolys tempPolys;
            tempModel.polys = &tempPolys;
            tempModel.rootOutside = 1;
            emptyModel(tempModel, true, true);
            ModelCoords coords;
            ModelCoords uncoords;
            float orientation = buildCoords(brushActor, &coords, &uncoords);
            Vector prePivot = actor::PrePivot(brushActor);
            Vector location = actor::Location(brushActor);
            std::vector<Poly> brushPolys = brush->polys ? brush->polys->polys : std::vector<Poly>();
            for (size_t i = 0; i < brushPolys.size(); i++) {
                Poly dest = brushPolys[i];
                dest.actor = brushActor;
                dest.brushPoly = static_cast<int32_t>(i);
                dest.polyFlags = (dest.polyFlags | polyFlags) & ~notPolyFlags;
                if (dest.link == -1) {
                    dest.link = static_cast<int32_t>(i);
                }
                transformPoly(dest, coords, prePivot, location, orientation);
                if (!dest.vertices.empty()) {
                    float error = (dest.vertices[0] - dest.base).dot(dest.normal);
                    if (std::abs(error) > 0.0001f) {
                        dest.base += dest.normal * error;
                    }
                }
                if (!dest.texture) {
                    dest.texture = currentTextureObject;
                }
                tempPolys.polys.push_back(dest);
            }
            int numPolysFromBrush = 0;
            if (oper == CsgIntersect || oper == CsgDeintersect) {
                emptyModel(*brush, true, true);
                if (!brush->polys) {
                    return 0;
                }
                for (size_t i = 0; i < tempPolys.polys.size(); i++) {
                    Poly edPoly = tempPolys.polys[i];
                    context.model = brush;
                    bspFilterFPoly(oper == CsgIntersect ? intersectBrushWithWorld : deintersectBrushWithWorld, model, edPoly);
                }
                numPolysFromBrush = static_cast<int>(brush->polys->polys.size());
            } else {
                for (size_t i = 0; i < brushPolys.size(); i++) {
                    Poly edPoly = tempPolys.polys[i];
                    edPoly.polyFlags &= ~PF_EdCut;
                    if (edPoly.link == static_cast<int32_t>(i)) {
                        edPoly.link = static_cast<int32_t>(model.surfs.size());
                        tempPolys.polys[i].link = edPoly.link;
                    } else {
                        edPoly.link = tempPolys.polys[static_cast<size_t>(edPoly.link)].link;
                    }
                    bspFilterFPoly(oper == CsgAdd ? addBrushToWorld : subtractBrushFromWorld, model, edPoly);
                }
            }
            if (!model.nodes.empty() && !(polyFlags & (PF_NotSolid | PF_Semisolid))) {
                build(tempModel, Optimization::Lame, 0, 1);
                context.model = brush;
                buildBound(tempModel);
                filterWorldThroughBrush(model, tempModel, oper, 0, &tempModel.sphere);
            }
            if (oper == CsgIntersect || oper == CsgDeintersect) {
                std::vector<Poly>& polys = brush->polys->polys;
                for (int i = numPolysFromBrush - 1; i >= 0; i--) {
                    Poly& dest = polys[static_cast<size_t>(i)];
                    int j = 0;
                    for (j = 0; j < i; j++) {
                        if (dest.link == polys[static_cast<size_t>(j)].link) {
                            dest.link = j;
                            break;
                        }
                    }
                    if (j >= i) {
                        dest.link = i;
                    }
                }
                for (int i = static_cast<int>(polys.size()) - 1; i >= numPolysFromBrush; i--) {
                    Poly& dest = polys[static_cast<size_t>(i)];
                    int j = 0;
                    for (j = numPolysFromBrush; j < i; j++) {
                        if (dest.link == polys[static_cast<size_t>(j)].link) {
                            dest.link = j;
                            break;
                        }
                    }
                    if (j >= i) {
                        dest.link = i;
                    }
                }
                brush->linked = 1;
                for (size_t i = 0; i < polys.size(); i++) {
                    Poly& dest = polys[i];
                    transformPoly(dest, uncoords, location, prePivot, orientation);
                    fixPoly(dest);
                    dest.actor = nullptr;
                    dest.brushPoly = static_cast<int32_t>(i);
                }
            }
            if (oper == CsgAdd || oper == CsgSubtract) {
                cleanup(model);
                if (buildBoundsFlag) {
                    buildBounds(model);
                }
            }
            if (oper == CsgIntersect || oper == CsgDeintersect) {
                mergeCoplanars(*brush, true, false);
                buildBound(*brush);
            }
            return 1 + static_cast<int>(errors);
        }

        void refresh(UModel& model, bool noRemapSurfs)
        {
            std::vector<int32_t> nodeRef(model.nodes.size(), -1);
            std::vector<int32_t> polyRef(model.surfs.size(), -1);
            if (!model.nodes.empty()) {
                tagReferencedNodes(model, nodeRef, polyRef, 0);
            }
            if (noRemapSurfs) {
                std::fill(polyRef.begin(), polyRef.end(), 0);
            }
            int32_t n = 0;
            for (size_t i = 0; i < model.surfs.size(); i++) {
                if (polyRef[i] != -1) {
                    model.surfs[static_cast<size_t>(n)] = model.surfs[i];
                    polyRef[i] = n++;
                }
            }
            model.surfs.resize(static_cast<size_t>(n));
            n = 0;
            for (size_t i = 0; i < model.nodes.size(); i++) {
                if (nodeRef[i] != -1) {
                    model.nodes[static_cast<size_t>(n)] = model.nodes[i];
                    nodeRef[i] = n++;
                }
            }
            model.nodes.resize(static_cast<size_t>(n));
            for (BspNode& node : model.nodes) {
                node.surf = polyRef[static_cast<size_t>(node.surf)];
                if (node.front != -1) {
                    node.front = nodeRef[static_cast<size_t>(node.front)];
                }
                if (node.back != -1) {
                    node.back = nodeRef[static_cast<size_t>(node.back)];
                }
                if (node.iPlane != -1) {
                    node.iPlane = nodeRef[static_cast<size_t>(node.iPlane)];
                }
            }
            std::vector<int32_t> vectorRef(model.vectors.size(), -1);
            std::vector<int32_t> pointRef(model.points.size(), -1);
            for (const BspSurf& surf : model.surfs) {
                vectorRef[static_cast<size_t>(surf.vNormal)] = 0;
                vectorRef[static_cast<size_t>(surf.vTextureU)] = 0;
                vectorRef[static_cast<size_t>(surf.vTextureV)] = 0;
                pointRef[static_cast<size_t>(surf.pBase)] = 0;
            }
            for (const BspNode& node : model.nodes) {
                for (int b = 0; b < node.numVertices; b++) {
                    pointRef[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + b)].vertex)] = 0;
                }
            }
            n = 0;
            for (size_t i = 0; i < model.points.size(); i++) {
                if (pointRef[i] != -1) {
                    model.points[static_cast<size_t>(n)] = model.points[i];
                    pointRef[i] = n++;
                }
            }
            model.points.resize(static_cast<size_t>(n));
            n = 0;
            for (size_t i = 0; i < model.vectors.size(); i++) {
                if (vectorRef[i] != -1) {
                    model.vectors[static_cast<size_t>(n)] = model.vectors[i];
                    vectorRef[i] = n++;
                }
            }
            model.vectors.resize(static_cast<size_t>(n));
            for (BspSurf& surf : model.surfs) {
                surf.vNormal = vectorRef[static_cast<size_t>(surf.vNormal)];
                surf.vTextureU = vectorRef[static_cast<size_t>(surf.vTextureU)];
                surf.vTextureV = vectorRef[static_cast<size_t>(surf.vTextureV)];
                surf.pBase = pointRef[static_cast<size_t>(surf.pBase)];
            }
            for (const BspNode& node : model.nodes) {
                for (int b = 0; b < node.numVertices; b++) {
                    BspVert& vert = model.verts[static_cast<size_t>(node.vertPool + b)];
                    vert.vertex = pointRef[static_cast<size_t>(vert.vertex)];
                }
            }
        }

        void optGeom(UModel& model)
        {
            mergeNearPoints(model, 0.25f);
            refresh(model, false);
            PointVertList pointVerts;
            pointVerts.alloc(model);
            pointVerts.addAllNodes();
            model.numSharedSides = 4;
            for (BspVert& vert : model.verts) {
                vert.side = -1;
            }
            for (size_t iNode = 0; iNode < model.nodes.size(); iNode++) {
                for (int thisVert = 0; thisVert < model.nodes[iNode].numVertices; thisVert++) {
                    const BspNode& node = model.nodes[iNode];
                    int prevVert = thisVert > 0 ? thisVert - 1 : node.numVertices - 1;
                    int32_t pThis = model.verts[static_cast<size_t>(node.vertPool + thisVert)].vertex;
                    int32_t pPrev = model.verts[static_cast<size_t>(node.vertPool + prevVert)].vertex;
                    bool shared = false;
                    for (int32_t pv1 = pointVerts.index[static_cast<size_t>(pThis)]; pv1 >= 0 && !shared; pv1 = pointVerts.entries[static_cast<size_t>(pv1)].next) {
                        for (int32_t pv2 = pointVerts.index[static_cast<size_t>(pPrev)]; pv2 >= 0; pv2 = pointVerts.entries[static_cast<size_t>(pv2)].next) {
                            if (pointVerts.entries[static_cast<size_t>(pv1)].node == pointVerts.entries[static_cast<size_t>(pv2)].node && pointVerts.entries[static_cast<size_t>(pv1)].node != static_cast<int32_t>(iNode)) {
                                shared = true;
                                break;
                            }
                        }
                    }
                    if (shared) {
                        continue;
                    }
                    distributePoint(model, &pointVerts, 0, pThis);
                    distributePoint(model, &pointVerts, 0, pPrev);
                }
            }
            pointVerts.alloc(model);
            pointVerts.addAllNodes();
            for (size_t iNode = 0; iNode < model.nodes.size(); iNode++) {
                const BspNode node = model.nodes[iNode];
                for (int thisVert = 0; thisVert < node.numVertices; thisVert++) {
                    if (model.verts[static_cast<size_t>(node.vertPool + thisVert)].side != -1) {
                        continue;
                    }
                    int prevVert = thisVert > 0 ? thisVert - 1 : node.numVertices - 1;
                    int32_t pThis = model.verts[static_cast<size_t>(node.vertPool + thisVert)].vertex;
                    int32_t pPrev = model.verts[static_cast<size_t>(node.vertPool + prevVert)].vertex;
                    bool linked = false;
                    for (int32_t pv1 = pointVerts.index[static_cast<size_t>(pThis)]; pv1 >= 0 && !linked; pv1 = pointVerts.entries[static_cast<size_t>(pv1)].next) {
                        for (int32_t pv2 = pointVerts.index[static_cast<size_t>(pPrev)]; pv2 >= 0; pv2 = pointVerts.entries[static_cast<size_t>(pv2)].next) {
                            const PointVert& a = pointVerts.entries[static_cast<size_t>(pv1)];
                            const PointVert& b = pointVerts.entries[static_cast<size_t>(pv2)];
                            if (a.node == b.node && a.node != static_cast<int32_t>(iNode)) {
                                const BspNode& other = model.nodes[static_cast<size_t>(b.node)];
                                int delta = (other.numVertices + b.vertex - a.vertex) % other.numVertices;
                                if (delta == 1) {
                                    int32_t otherVert = b.vertex;
                                    int32_t side = model.verts[static_cast<size_t>(other.vertPool + otherVert)].side;
                                    if (side == -1) {
                                        side = model.numSharedSides++;
                                    }
                                    model.verts[static_cast<size_t>(node.vertPool + thisVert)].side = side;
                                    model.verts[static_cast<size_t>(other.vertPool + otherVert)].side = side;
                                    linked = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!linked) {
                        errors++;
                    }
                }
            }
        }

        void buildBounds(UModel& model)
        {
            if (model.nodes.empty()) {
                return;
            }
            buildZoneMasks(model, 0);
            const float e = 32768.0f;
            Vector corners[6][4] = {
                { { -e, -e, e }, { e, -e, e }, { e, e, e }, { -e, e, e } },
                { { -e, e, -e }, { e, e, -e }, { e, -e, -e }, { -e, -e, -e } },
                { { -e, e, -e }, { -e, e, e }, { e, e, e }, { e, e, -e } },
                { { e, -e, -e }, { e, -e, e }, { -e, -e, e }, { -e, -e, -e } },
                { { e, e, -e }, { e, e, e }, { e, -e, e }, { e, -e, -e } },
                { { -e, -e, -e }, { -e, -e, e }, { -e, e, e }, { -e, e, -e } },
            };
            Vector normals[6] = { { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { -1, 0, 0 } };
            std::deque<Poly> arena;
            std::vector<Poly*> list;
            for (int i = 0; i < 6; i++) {
                Poly& poly = arena.emplace_back(emptyPoly());
                poly.vertices.assign(corners[i], corners[i] + 4);
                poly.normal = normals[i];
                poly.base = poly.vertices[0];
                list.push_back(&poly);
            }
            model.nodeBounds.clear();
            model.leafHulls.clear();
            for (BspNode& node : model.nodes) {
                node.renderBound = -1;
                node.collisionBound = -1;
            }
            filterBound(model, nullptr, 0, list, model.rootOutside != 0, arena);
        }

        void repartition(UModel& model, int32_t iNode, int simple)
        {
            buildFPolys(model, true, iNode);
            mergeCoplanars(model, false, false);
            build(model, Optimization::Good, 12, simple);
            refresh(model, true);
        }

        void prepMovingBrush(UObject* brushActor)
        {
            UModel* brush = cast<UModel>(actor::Brush(brushActor));
            if (!brush) {
                return;
            }
            buildBound(*brush);
            build(*brush, Optimization::Good, 15, 1);
            refresh(*brush, true);
            buildBounds(*brush);
        }

        void testVisibility(ULevel* level, UModel& model)
        {
            if (model.nodes.empty()) {
                return;
            }
            if (model.zones.size() < 64) {
                model.zones.resize(64);
            }
            Visibility visibility;
            visibility.level = level;
            visibility.model = &model;
            visibility.run();
        }

        int32_t pointLeafZone(const UModel& model, const Vector& point, int32_t* leaf)
        {
            if (model.nodes.empty()) {
                if (leaf) {
                    *leaf = -1;
                }
                return 0;
            }
            int32_t iNode = 0;
            int32_t parent = 0;
            int isFront = 0;
            while (iNode >= 0) {
                const BspNode& node = model.nodes[static_cast<size_t>(iNode)];
                isFront = node.plane.x * point.x + node.plane.y * point.y + node.plane.z * point.z - node.plane.w >= 0.0f ? 1 : 0;
                parent = iNode;
                iNode = isFront ? node.front : node.back;
            }
            if (leaf) {
                *leaf = model.nodes[static_cast<size_t>(parent)].leaf[isFront];
            }
            return model.nodes[static_cast<size_t>(parent)].zone[isFront];
        }

        Stats csgRebuild(ULevel* level)
        {
            Stats stats;
            if (!level || !level->model) {
                return stats;
            }
            UModel& model = *level->model;
            objects::ensureLoaded(&model);
            objects::flushLoads();
            fastRebuild = true;
            size_t totalErrors = 0;
            ensurePolys(model);
            emptyModel(model, true, true);
            if (model.zones.size() < 64) {
                model.zones.resize(64);
            }
            std::vector<UObject*> brushes = staticBrushes(level);
            for (UObject* a : brushes) {
                uint32_t flags = static_cast<uint32_t>(brush::PolyFlags(a));
                uint8_t oper = brush::CsgOper(a);
                if (!(flags & PF_Semisolid) || oper != CsgAdd || (flags & PF_Portal)) {
                    if (flags & PF_Portal) {
                        flags = (flags & ~PF_Semisolid) | PF_NotSolid;
                        brush::PolyFlags(a) = static_cast<int32_t>(flags);
                    }
                    brushCSG(a, model, flags, oper, false);
                    totalErrors += errors;
                    stats.brushes++;
                }
            }
            repartition(model, 0, 0);
            testVisibility(level, model);
            std::vector<int32_t> fronts;
            std::vector<int32_t> backs;
            if (!model.nodes.empty()) {
                enlistLeaves(model, fronts, backs, 0);
            }
            for (UObject* a : brushes) {
                uint32_t flags = static_cast<uint32_t>(brush::PolyFlags(a));
                if ((flags & PF_Semisolid) && !(flags & PF_Portal) && brush::CsgOper(a) == CsgAdd) {
                    brushCSG(a, model, flags, CsgAdd, false);
                    totalErrors += errors;
                    stats.brushes++;
                }
            }
            for (int32_t front : fronts) {
                int32_t child = model.nodes[static_cast<size_t>(front)].front;
                if (child >= 0) {
                    repartition(model, child, 2);
                }
            }
            for (int32_t back : backs) {
                int32_t child = model.nodes[static_cast<size_t>(back)].back;
                if (child >= 0) {
                    repartition(model, child, 2);
                }
            }
            optGeom(model);
            buildBounds(model);
            fastRebuild = false;
            errors = totalErrors;
            stats.nodes = model.nodes.size();
            stats.surfs = model.surfs.size();
            stats.leaves = model.leaves.size();
            stats.errors = totalErrors;
            for (const ConvexLeaf& leaf : model.leaves) {
                stats.zones = std::max(stats.zones, static_cast<size_t>(leaf.zone + 1));
            }
            model.revision++;
            return stats;
        }

        Stats bspRebuild(ULevel* level, Optimization opt, int balance, bool zones, bool optimizeGeometry)
        {
            Stats stats;
            if (!level || !level->model) {
                return stats;
            }
            UModel& model = *level->model;
            errors = 0;
            buildFPolys(model, true, 0);
            mergeCoplanars(model, false, false);
            build(model, opt, balance, 0);
            if (zones) {
                testVisibility(level, model);
            }
            if (optimizeGeometry) {
                optGeom(model);
            }
            if (model.polys) {
                model.polys->polys.clear();
            }
            stats.nodes = model.nodes.size();
            stats.surfs = model.surfs.size();
            stats.leaves = model.leaves.size();
            stats.errors = errors;
            for (const ConvexLeaf& leaf : model.leaves) {
                stats.zones = std::max(stats.zones, static_cast<size_t>(leaf.zone + 1));
            }
            model.revision++;
            return stats;
        }
    }
}
