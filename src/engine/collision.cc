#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace dx {
    namespace {
        struct DVec {
            double x = 0;
            double y = 0;
            double z = 0;
        };

        struct DPlane {
            double x = 0;
            double y = 0;
            double z = 0;
            double w = 0;
        };

        DVec toD(const Vector& v)
        {
            return { v.x, v.y, v.z };
        }

        Vector toF(const DVec& v)
        {
            return { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) };
        }

        double dot3(const DVec& a, const DVec& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        DVec cross3(const DVec& a, const DVec& b)
        {
            return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
        }

        DVec normalize3(const DVec& v)
        {
            double length = std::sqrt(dot3(v, v));
            if (length < 1e-12) {
                return {};
            }
            return { v.x / length, v.y / length, v.z / length };
        }

        DPlane nodePlane(const UModel& model, int32_t index)
        {
            const Plane& p = model.nodes[static_cast<size_t>(index)].plane;
            return { p.x, p.y, p.z, p.w };
        }

        int classify(const DVec& center, const DVec& extent, const DPlane& plane)
        {
            double e = extent.x * std::abs(plane.x) + extent.y * std::abs(plane.y) + extent.z * std::abs(plane.z);
            double s = center.x * plane.x + center.y * plane.y + center.z * plane.z - plane.w;
            if (s - e > 0) {
                return -1;
            }
            if (s + e < 0) {
                return 1;
            }
            return 0;
        }

        struct HullScratch {
            std::vector<DPlane> planes;
            std::vector<int32_t> planeNodes;
        };

        bool hullBounds(const UModel& model, int32_t bound, DVec& min, DVec& max)
        {
            if (bound < 0 || static_cast<size_t>(bound) >= model.leafHulls.size()) {
                return false;
            }
            size_t index = static_cast<size_t>(bound);
            while (index < model.leafHulls.size() && model.leafHulls[index] >= 0) {
                int32_t node = model.leafHulls[index] & ~0x40000000;
                if (node < 0 || static_cast<size_t>(node) >= model.nodes.size()) {
                    return false;
                }
                index++;
            }
            index++;
            if (index + 6 > model.leafHulls.size()) {
                return false;
            }
            float box[6];
            for (int i = 0; i < 6; i++) {
                int32_t raw = model.leafHulls[index + static_cast<size_t>(i)];
                std::memcpy(&box[i], &raw, 4);
            }
            min = { box[0], box[1], box[2] };
            max = { box[3], box[4], box[5] };
            return true;
        }

        void hullPlanes(const UModel& model, int32_t bound, HullScratch& scratch)
        {
            scratch.planes.clear();
            scratch.planeNodes.clear();
            size_t index = static_cast<size_t>(bound);
            while (index < model.leafHulls.size() && model.leafHulls[index] >= 0) {
                int32_t entry = model.leafHulls[index];
                bool flip = (entry & 0x40000000) != 0;
                int32_t node = entry & ~0x40000000;
                DPlane plane = nodePlane(model, node);
                if (flip) {
                    plane = { -plane.x, -plane.y, -plane.z, -plane.w };
                }
                scratch.planes.push_back(plane);
                scratch.planeNodes.push_back(node);
                index++;
            }
        }

        bool overlapNode(const UModel& model, int32_t nodeIndex, const DVec& center, const DVec& extent, int depth, HullScratch& scratch)
        {
            while (nodeIndex >= 0 && static_cast<size_t>(nodeIndex) < model.nodes.size() && depth < 4096) {
                depth++;
                const BspNode& node = model.nodes[static_cast<size_t>(nodeIndex)];
                DVec min;
                DVec max;
                if (node.collisionBound >= 0 && hullBounds(model, node.collisionBound, min, max)) {
                    bool boxes = !(min.x > center.x + extent.x || center.x - extent.x > max.x || min.y > center.y + extent.y || center.y - extent.y > max.y || min.z > center.z + extent.z || center.z - extent.z > max.z);
                    if (boxes) {
                        hullPlanes(model, node.collisionBound, scratch);
                        bool outside = false;
                        for (const DPlane& plane : scratch.planes) {
                            if (classify(center, extent, plane) == -1) {
                                outside = true;
                                break;
                            }
                        }
                        if (!outside) {
                            return true;
                        }
                    }
                }
                DVec padded { extent.x * 1.1, extent.y * 1.1, extent.z * 1.1 };
                int side = classify(center, padded, nodePlane(model, nodeIndex));
                bool front = node.front >= 0 && side <= 0;
                bool back = node.back >= 0 && side >= 0;
                if (front && back) {
                    if (overlapNode(model, node.front, center, extent, depth, scratch)) {
                        return true;
                    }
                    nodeIndex = node.back;
                } else if (front) {
                    nodeIndex = node.front;
                } else if (back) {
                    nodeIndex = node.back;
                } else {
                    break;
                }
            }
            return false;
        }

        bool modelOverlap(const UModel* model, const Vector& center, const Vector& extent)
        {
            if (!model || model->nodes.empty()) {
                return false;
            }
            static HullScratch scratch;
            return overlapNode(*model, 0, toD(center), toD(extent), 0, scratch);
        }

        struct BrushTransform {
            Vector location;
            Coords axes;
            Vector scale { 1, 1, 1 };
            Vector prePivot;
        };

        BrushTransform brushTransform(UObject* a)
        {
            BrushTransform t;
            t.location = actor::Location(a);
            t.axes = rotationAxes(actor::Rotation(a));
            t.prePivot = actor::PrePivot(a);
            Vector* mainScale = reinterpret_cast<Vector*>(brush::MainScale(a));
            if (mainScale && mainScale->x != 0 && mainScale->y != 0 && mainScale->z != 0) {
                t.scale = *mainScale;
            }
            return t;
        }

        Vector worldToBrush(const BrushTransform& t, const Vector& point)
        {
            Vector local = transformVector(t.axes, point - t.location);
            return { local.x / t.scale.x + t.prePivot.x, local.y / t.scale.y + t.prePivot.y, local.z / t.scale.z + t.prePivot.z };
        }

        Vector brushNormalToWorld(const BrushTransform& t, const Vector& normal)
        {
            Vector scaled { normal.x / t.scale.x, normal.y / t.scale.y, normal.z / t.scale.z };
            return untransformVector(t.axes, scaled).normal();
        }

        bool isCsg(const BspNode& node, uint32_t extraFlags)
        {
            return node.numVertices > 0 && !(node.nodeFlags & (0x20 | 0x01 | extraFlags));
        }

        bool childOutside(const BspNode& node, int child, bool outside, uint32_t extraFlags = 0)
        {
            return child ? (outside || isCsg(node, extraFlags)) : (outside && !isCsg(node, extraFlags));
        }

        DPlane tracePlane(const UModel& model, const BrushTransform* owner, int32_t index)
        {
            DPlane p = nodePlane(model, index);
            if (!owner) {
                return p;
            }
            Vector n = brushNormalToWorld(*owner, { static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z) });
            Vector q { static_cast<float>(p.x * p.w), static_cast<float>(p.y * p.w), static_cast<float>(p.z * p.w) };
            Vector scaled { (q.x - owner->prePivot.x) * owner->scale.x, (q.y - owner->prePivot.y) * owner->scale.y, (q.z - owner->prePivot.z) * owner->scale.z };
            Vector world = untransformVector(owner->axes, scaled) + owner->location;
            return { n.x, n.y, n.z, static_cast<double>(n.dot(world)) };
        }

        struct BoxLineTrace {
            const UModel& model;
            const BrushTransform* owner;
            DVec start;
            DVec end;
            DVec extent;
            double hitTime = 2.0;
            DVec hitNormal;
            int32_t hitNode = -1;
            bool didHit = false;
            std::vector<DPlane> hulls;
            std::vector<int32_t> hullNodes;
            std::vector<int> flags;
            DVec min;
            DVec max;
            DVec localHit;
            int32_t localNode = -1;
            double t0 = 0;
            double t1 = 0;

            BoxLineTrace(const UModel& traceModel, const BrushTransform* traceOwner, const DVec& traceStart, const DVec& traceEnd, const DVec& traceExtent)
                : model(traceModel)
                , owner(traceOwner)
                , start(traceStart)
                , end(traceEnd)
                , extent(traceExtent)
            {
            }

            DPlane plane(int32_t index) const
            {
                return tracePlane(model, owner, index);
            }

            bool clipTo(const DPlane& hull, int32_t item)
            {
                double pushOut = std::abs(hull.x * extent.x) + std::abs(hull.y * extent.y) + std::abs(hull.z * extent.z);
                double d0 = hull.x * start.x + hull.y * start.y + hull.z * start.z - hull.w;
                double d1 = hull.x * end.x + hull.y * end.y + hull.z * end.z - hull.w;
                double adjD0 = d0 - pushOut;
                if (d0 > d1 && adjD0 >= -pushOut && adjD0 < 0) {
                    adjD0 = 0.0;
                }
                double t = adjD0 / (d0 - d1);
                if ((d0 - d1) < -0.00001) {
                    if (t < t1) {
                        t1 = t;
                    }
                } else if ((d0 - d1) > 0.00001) {
                    if (t > t0) {
                        t0 = t;
                        localHit = { hull.x, hull.y, hull.z };
                        localNode = item;
                    }
                } else if (d0 > pushOut && d1 > pushOut) {
                    return false;
                }
                return t0 < t1;
            }

            void setupHulls(const BspNode& node)
            {
                hulls.clear();
                hullNodes.clear();
                flags.clear();
                size_t index = static_cast<size_t>(node.collisionBound);
                while (index < model.leafHulls.size() && model.leafHulls[index] >= 0 && hulls.size() < 64) {
                    int32_t entry = model.leafHulls[index];
                    int32_t hullNode = entry & ~0x40000000;
                    DPlane hull = plane(hullNode);
                    if (entry & 0x40000000) {
                        hull = { -hull.x, -hull.y, -hull.z, -hull.w };
                    }
                    hulls.push_back(hull);
                    hullNodes.push_back(hullNode);
                    flags.push_back((hull.x < 0 ? 0x01 : hull.x > 0 ? 0x02
                                                                    : 0)
                        | (hull.y < 0 ? 0x04 : hull.y > 0 ? 0x08
                                                          : 0)
                        | (hull.z < 0 ? 0x10 : hull.z > 0 ? 0x20
                                                          : 0));
                    index++;
                }
                float box[6] = {};
                size_t boxIndex = static_cast<size_t>(node.collisionBound) + hulls.size() + 1;
                for (size_t i = 0; i < 6 && boxIndex + i < model.leafHulls.size(); i++) {
                    int32_t raw = model.leafHulls[boxIndex + i];
                    std::memcpy(&box[i], &raw, 4);
                }
                min = { box[0], box[1], box[2] };
                max = { box[3], box[4], box[5] };
            }

            bool convolveEdge(size_t i, size_t j, const DVec& axis, int mask)
            {
                if (((flags[i] | flags[j]) & mask) != mask) {
                    return true;
                }
                DVec ni { hulls[i].x, hulls[i].y, hulls[i].z };
                DVec nj { hulls[j].x, hulls[j].y, hulls[j].z };
                if (dot3(cross3(axis, ni), cross3(axis, nj)) <= 0.001) {
                    return true;
                }
                DVec d = cross3(ni, nj);
                double dd = dot3(d, d);
                if (dd < 0.001 * 0.001) {
                    return t0 < t1;
                }
                DVec c1 = cross3(nj, d);
                DVec c2 = cross3(d, ni);
                DVec point { (hulls[i].w * c1.x + hulls[j].w * c2.x) / dd, (hulls[i].w * c1.y + hulls[j].w * c2.y) / dd, (hulls[i].w * c1.z + hulls[j].w * c2.z) / dd };
                DVec a = normalize3(cross3(axis, normalize3(d)));
                if (dot3(ni, a) < 0.0) {
                    a = { -a.x, -a.y, -a.z };
                }
                return clipTo({ a.x, a.y, a.z, dot3(point, a) }, -1);
            }

            bool clipPrimitive()
            {
                for (size_t i = 0; i < hulls.size(); i++) {
                    if (!clipTo(hulls[i], hullNodes[i])) {
                        return false;
                    }
                }
                if (!owner) {
                    if (!clipTo({ 0, 0, -1, 0.1 - min.z }, -1) || !clipTo({ 0, 0, 1, max.z + 0.1 }, -1) || !clipTo({ -1, 0, 0, 0.1 - min.x }, -1) || !clipTo({ 1, 0, 0, max.x - 0.1 }, -1) || !clipTo({ 0, -1, 0, 0.1 - min.y }, -1) || !clipTo({ 0, 1, 0, max.y - 0.1 }, -1)) {
                        return false;
                    }
                }
                for (size_t i = 0; i < hulls.size(); i++) {
                    for (size_t j = 0; j < i; j++) {
                        if (!convolveEdge(i, j, { 1, 0, 0 }, 0x03) || !convolveEdge(i, j, { 0, 1, 0 }, 0x0c) || !convolveEdge(i, j, { 0, 0, 1 }, 0x30)) {
                            return false;
                        }
                    }
                }
                return true;
            }

            void check(int32_t parent, int32_t nodeIndex, bool outside)
            {
                while (nodeIndex >= 0) {
                    const BspNode& node = model.nodes[static_cast<size_t>(nodeIndex)];
                    DPlane p = plane(nodeIndex);
                    double d0 = p.x * start.x + p.y * start.y + p.z * start.z - p.w;
                    double d1 = p.x * end.x + p.y * end.y + p.z * end.z - p.w;
                    double pushOut = std::abs(p.x * extent.x * 1.1) + std::abs(p.y * extent.y * 1.1) + std::abs(p.z * extent.z * 1.1);
                    bool use[2] = { d0 <= pushOut || d1 <= pushOut, d0 >= -pushOut || d1 >= -pushOut };
                    int frontFirst = d0 >= d1 ? 1 : 0;
                    if (use[frontFirst]) {
                        check(nodeIndex, frontFirst ? node.front : node.back, childOutside(node, frontFirst, outside));
                    }
                    if (!use[1 - frontFirst]) {
                        return;
                    }
                    parent = nodeIndex;
                    nodeIndex = frontFirst ? node.back : node.front;
                    outside = childOutside(node, 1 - frontFirst, outside);
                }
                const BspNode& parentNode = model.nodes[static_cast<size_t>(parent)];
                if (!outside && parentNode.collisionBound >= 0) {
                    setupHulls(parentNode);
                    t0 = -1.0;
                    t1 = hitTime;
                    localHit = {};
                    localNode = -1;
                    if (clipPrimitive() && t0 > -1.0 && t0 < t1 && t1 > 0.0) {
                        hitTime = t0;
                        hitNormal = localHit;
                        hitNode = localNode >= 0 ? localNode : parent;
                        didHit = true;
                    }
                }
            }
        };

        struct PointLineTrace {
            const UModel& model;
            const BrushTransform* owner;
            uint8_t nodeFlags;
            DVec location;
            int32_t item = -1;
            bool outOfCorner = false;

            PointLineTrace(const UModel& traceModel, const BrushTransform* traceOwner, uint8_t traceFlags)
                : model(traceModel)
                , owner(traceOwner)
                , nodeFlags(traceFlags)
            {
            }

            DPlane plane(int32_t index) const
            {
                return tracePlane(model, owner, index);
            }

            bool check(int32_t hitIndex, int32_t nodeIndex, DVec end, DVec start, bool outside)
            {
                while (nodeIndex >= 0) {
                    const BspNode& node = model.nodes[static_cast<size_t>(nodeIndex)];
                    DPlane p = plane(nodeIndex);
                    double dist1 = p.x * start.x + p.y * start.y + p.z * start.z - p.w;
                    double dist2 = p.x * end.x + p.y * end.y + p.z * end.z - p.w;
                    if (dist1 > -0.001 && dist2 > -0.001) {
                        outside = outside || isCsg(node, nodeFlags & ~0x10u);
                        nodeIndex = node.front;
                    } else if (dist1 < 0.001 && dist2 < 0.001) {
                        outside = outside && !isCsg(node, nodeFlags & ~0x10u);
                        nodeIndex = node.back;
                    } else {
                        double f = dist1 / (dist2 - dist1);
                        DVec middle { start.x + (start.x - end.x) * f, start.y + (start.y - end.y) * f, start.z + (start.z - end.z) * f };
                        int frontFirst = dist1 > 0.0 ? 1 : 0;
                        if (!check(hitIndex, frontFirst ? node.front : node.back, middle, start, childOutside(node, frontFirst, outside, nodeFlags))) {
                            return false;
                        }
                        outside = childOutside(node, 1 - frontFirst, outside, nodeFlags);
                        hitIndex = nodeIndex;
                        nodeIndex = frontFirst ? node.back : node.front;
                        start = middle;
                    }
                }
                if (!outside) {
                    if (outOfCorner || !(nodeFlags & 0x10)) {
                        location = start;
                        item = hitIndex;
                    } else {
                        outside = true;
                    }
                } else {
                    outOfCorner = true;
                }
                return outside;
            }
        };

        bool modelTrace(const UModel* model, const BrushTransform* owner, const Vector& start, const Vector& end, const Vector& extent, float& time, Vector& normal, int32_t* hitNode = nullptr, uint8_t nodeFlags = 0)
        {
            if (!model || model->nodes.empty()) {
                return false;
            }
            Vector delta = end - start;
            if (extent.zero()) {
                PointLineTrace trace(*model, owner, nodeFlags);
                if (trace.check(0, 0, toD(end), toD(start), model->rootOutside != 0)) {
                    return false;
                }
                Vector location = toF(trace.location);
                time = std::clamp((location - start).dot(delta) / delta.dot(delta) - 0.5f / delta.size(), 0.0f, 1.0f);
                DPlane p = trace.plane(trace.item);
                normal = { static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z) };
                if (hitNode) {
                    *hitNode = trace.item;
                }
                return true;
            }
            BoxLineTrace trace(*model, owner, toD(start), toD(end), toD(extent));
            trace.check(0, 0, model->rootOutside != 0);
            if (!trace.didHit) {
                return false;
            }
            float dist = delta.size();
            time = std::clamp(static_cast<float>(trace.hitTime) - std::max(0.1f, 0.1f / dist), 0.0f, 1.0f);
            if (time == 1.0f) {
                return false;
            }
            normal = toF(trace.hitNormal);
            if (hitNode) {
                *hitNode = trace.hitNode;
            }
            return true;
        }

        bool brushTrace(UObject* a, const Vector& start, const Vector& end, const Vector& extent, float& time, Vector& normal)
        {
            UModel* model = cast<UModel>(actor::Brush(a));
            if (!model) {
                return false;
            }
            BrushTransform t = brushTransform(a);
            return modelTrace(model, &t, start, end, extent, time, normal);
        }

        bool brushOverlap(UObject* a, const Vector& center, const Vector& extent)
        {
            UModel* model = cast<UModel>(actor::Brush(a));
            if (!model) {
                return false;
            }
            BrushTransform t = brushTransform(a);
            return modelOverlap(model, worldToBrush(t, center), extent);
        }

        bool cylinderTrace(UObject* target, const Vector& start, const Vector& end, const Vector& extent, float& time, Vector& normal)
        {
            Vector center = actor::Location(target);
            float radius = actor::CollisionRadius(target) + extent.x;
            float height = actor::CollisionHeight(target) + extent.z;
            if ((start.x > center.x + radius && end.x > center.x + radius) || (start.x < center.x - radius && end.x < center.x - radius)) {
                return false;
            }
            if ((start.y > center.y + radius && end.y > center.y + radius) || (start.y < center.y - radius && end.y < center.y - radius)) {
                return false;
            }
            float topZ = center.z + height;
            float botZ = center.z - height;
            if ((start.z > topZ && end.z > topZ) || (start.z < botZ && end.z < botZ)) {
                return false;
            }
            float t0 = 0;
            float t1 = 1;
            Vector hitNormal;
            if (start.z > topZ && end.z < topZ) {
                float t = (topZ - start.z) / (end.z - start.z);
                if (t > t0) {
                    t0 = t;
                    hitNormal = { 0, 0, 1 };
                }
            } else if (start.z < topZ && end.z > topZ) {
                t1 = std::min(t1, (topZ - start.z) / (end.z - start.z));
            }
            if (start.z < botZ && end.z > botZ) {
                float t = (botZ - start.z) / (end.z - start.z);
                if (t > t0) {
                    t0 = t;
                    hitNormal = { 0, 0, -1 };
                }
            } else if (start.z > botZ && end.z < botZ) {
                t1 = std::min(t1, (botZ - start.z) / (end.z - start.z));
            }
            if (t0 >= t1) {
                return false;
            }
            float kx = start.x - center.x;
            float ky = start.y - center.y;
            float vx = end.x - start.x;
            float vy = end.y - start.y;
            float a = vx * vx + vy * vy;
            float b = 2.0f * (kx * vx + ky * vy);
            float c = kx * kx + ky * ky - radius * radius;
            float discriminant = b * b - 4.0f * a * c;
            if (c < 1.0f && start.z > botZ && start.z < topZ) {
                float dir = vx * kx + vy * ky;
                if (dir < -0.1f) {
                    time = 0;
                    normal = Vector { kx, ky, 0 }.normal();
                    return true;
                }
                return false;
            }
            if (discriminant < 0) {
                return false;
            }
            if (a < 1e-8f) {
                if (c > 0) {
                    return false;
                }
            } else {
                discriminant = std::sqrt(discriminant);
                float r2a = 0.5f / a;
                t1 = std::min(t1, (discriminant - b) * r2a);
                float t = -(discriminant + b) * r2a;
                if (t > t0) {
                    t0 = t;
                    hitNormal = Vector { kx + vx * t0, ky + vy * t0, 0 }.normal();
                }
                if (t0 >= t1) {
                    return false;
                }
            }
            time = std::clamp(t0 - 0.001f, 0.0f, 1.0f);
            normal = hitNormal;
            return true;
        }

        bool actorCollides(UObject* other, UObject* source, uint32_t flags)
        {
            if (!other || other == source || other->destroyed || !actor::bCollideActors(other)) {
                return false;
            }
            World* world = World::current;
            if (world && other == world->levelInfo) {
                return false;
            }
            int guard = 0;
            for (UObject* owner = source ? actor::Owner(source) : nullptr; owner && guard < 64; owner = actor::Owner(owner), guard++) {
                if (owner == other) {
                    return false;
                }
            }
            if (isMover(other)) {
                return (flags & TRACE_Movers) != 0;
            }
            if (isBrush(other)) {
                return false;
            }
            if (isPawn(other)) {
                return (flags & TRACE_Pawns) != 0;
            }
            if (flags & TRACE_OnlyProjActor) {
                return actor::bProjTarget(other) || (actor::bBlockActors(other) && actor::bBlockPlayers(other));
            }
            if (flags & TRACE_Blocking) {
                return source ? isBlockedBy(source, other) : (actor::bBlockActors(other) != 0 || actor::bBlockPlayers(other) != 0);
            }
            return (flags & TRACE_Others) != 0;
        }
    }

    int32_t World::pointLeaf(const Vector& location)
    {
        int32_t leaf = -1;
        zoneAt(location, &leaf, nullptr);
        return leaf;
    }

    UObject* World::zoneAt(const Vector& location, int32_t* leaf, uint8_t* zoneNumber)
    {
        UModel* model = level ? level->model : nullptr;
        if (leaf) {
            *leaf = -1;
        }
        if (zoneNumber) {
            *zoneNumber = 0;
        }
        if (!model || model->nodes.empty()) {
            return levelInfo;
        }
        int32_t index = 0;
        int guard = 0;
        while (guard++ < 10000) {
            const BspNode& node = model->nodes[static_cast<size_t>(index)];
            float distance = node.plane.x * location.x + node.plane.y * location.y + node.plane.z * location.z - node.plane.w;
            int side = distance > 0 ? 1 : 0;
            int32_t next = side ? node.front : node.back;
            if (next < 0) {
                uint8_t zone = node.zone[side];
                if (leaf) {
                    *leaf = node.leaf[side];
                }
                if (zoneNumber) {
                    *zoneNumber = zone;
                }
                UObject* zoneActor = zone < model->zones.size() ? model->zones[zone].zoneActor : nullptr;
                return zoneActor ? zoneActor : levelInfo;
            }
            index = next;
        }
        return levelInfo;
    }

    void World::setLocationZone(UObject* a, const Vector& location, bool test)
    {
        if (!a || a->destroyed) {
            return;
        }
        if (a == levelInfo) {
            region(a) = { levelInfo, -1, 0 };
            return;
        }
        PointRegion newRegion;
        newRegion.zone = zoneAt(location, &newRegion.iLeaf, &newRegion.zoneNumber);
        PointRegion& oldRegion = region(a);
        if (newRegion.zone != oldRegion.zone && test) {
            oldRegion = newRegion;
        } else if (newRegion.zone != oldRegion.zone) {
            UObject* oldZone = oldRegion.zone;
            if (oldZone) {
                vm::Call(oldZone, Name("ActorLeaving")).obj(a).run();
            }
            vm::Call(a, Name("ZoneChange")).obj(newRegion.zone).run();
            if (a->destroyed) {
                return;
            }
            region(a) = newRegion;
            vm::Call(newRegion.zone, Name("ActorEntered")).obj(a).run();
        } else {
            oldRegion = newRegion;
        }
        if (isPawn(a) && !a->destroyed) {
            PointRegion& foot = *reinterpret_cast<PointRegion*>(pawn::FootRegion(a));
            PointRegion newFoot;
            newFoot.zone = zoneAt(actor::Location(a) - Vector { 0, 0, actor::CollisionHeight(a) }, &newFoot.iLeaf, &newFoot.zoneNumber);
            if (newFoot.zone != foot.zone && !test) {
                vm::Call(a, Name("FootZoneChange")).obj(newFoot.zone).run();
            }
            *reinterpret_cast<PointRegion*>(pawn::FootRegion(a)) = newFoot;
            PointRegion& head = *reinterpret_cast<PointRegion*>(pawn::HeadRegion(a));
            PointRegion newHead;
            newHead.zone = zoneAt(actor::Location(a) + Vector { 0, 0, pawn::EyeHeight(a) }, &newHead.iLeaf, &newHead.zoneNumber);
            if (newHead.zone != head.zone && !test) {
                vm::Call(a, Name("HeadZoneChange")).obj(newHead.zone).run();
            }
            *reinterpret_cast<PointRegion*>(pawn::HeadRegion(a)) = newHead;
        }
    }

    namespace {
        bool signBitClear(float value)
        {
            uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            return (bits & 0x80000000u) == 0;
        }

        bool fastLineNode(const UModel& model, int32_t nodeIndex, Vector end, Vector start, bool outside)
        {
            while (nodeIndex >= 0) {
                const BspNode& node = model.nodes[static_cast<size_t>(nodeIndex)];
                float dist1 = node.plane.x * start.x + node.plane.y * start.y + node.plane.z * start.z - node.plane.w;
                float dist2 = node.plane.x * end.x + node.plane.y * end.y + node.plane.z * end.z - node.plane.w;
                bool notCsg = (node.nodeFlags & 0x01) != 0;
                bool g1 = signBitClear(dist1);
                bool g2 = signBitClear(dist2);
                if (g1 != g2) {
                    float alpha = dist1 / (dist1 - dist2);
                    Vector middle = start + (end - start) * alpha;
                    if (!fastLineNode(model, g2 ? node.front : node.back, middle, end, notCsg ? outside : g2)) {
                        return false;
                    }
                    end = middle;
                }
                outside = notCsg ? outside : g1;
                nodeIndex = g1 ? node.front : node.back;
            }
            return outside;
        }
    }

    bool World::fastLineCheck(const Vector& end, const Vector& start)
    {
        UModel* model = level ? level->model : nullptr;
        if (!model) {
            return true;
        }
        if (model->nodes.empty()) {
            return model->rootOutside != 0;
        }
        return fastLineNode(*model, 0, end, start, model->rootOutside != 0);
    }

    bool World::bspLineCheck(CheckResult& hit, const Vector& start, const Vector& end, const Vector& extent, uint8_t nodeFlags)
    {
        float time = 1;
        Vector normal;
        int32_t node = -1;
        UModel* model = level ? level->model : nullptr;
        if (!modelTrace(model, nullptr, start, end, extent, time, normal, &node, nodeFlags)) {
            return false;
        }
        hit.actor = levelInfo;
        hit.time = time;
        hit.normal = normal;
        hit.location = start + (end - start) * time;
        hit.item = node;
        if (node >= 0 && static_cast<size_t>(node) < model->nodes.size()) {
            int32_t surf = model->nodes[static_cast<size_t>(node)].surf;
            if (surf >= 0 && static_cast<size_t>(surf) < model->surfs.size()) {
                hit.material = model->surfs[static_cast<size_t>(surf)].texture;
            }
        }
        return true;
    }

    std::vector<CheckResult> World::multiLineCheck(const Vector& start, const Vector& end, const Vector& extent, UObject* source, uint32_t flags, uint8_t nodeFlags)
    {
        std::vector<CheckResult> hits;
        Vector actorEnd = end;
        float dilation = 1.0f;
        bool onlyMovers = false;
        if (flags & (TRACE_Level | TRACE_LevelGeometry)) {
            CheckResult hit;
            if (bspLineCheck(hit, start, end, extent, nodeFlags)) {
                float distance = (hit.location - start).size();
                if (distance > 0) {
                    dilation = std::min(1.0f, hit.time * (distance + 5.0f) / distance);
                }
                actorEnd = start + (end - start) * dilation;
                onlyMovers = hit.time < 0.01f;
                hits.push_back(hit);
            }
        }
        if (flags & (TRACE_Pawns | TRACE_Movers | TRACE_Others | TRACE_OnlyProjActor | TRACE_Blocking)) {
            Vector boxMin { std::min(start.x, actorEnd.x) - extent.x, std::min(start.y, actorEnd.y) - extent.y, std::min(start.z, actorEnd.z) - extent.z };
            Vector boxMax { std::max(start.x, actorEnd.x) + extent.x, std::max(start.y, actorEnd.y) + extent.y, std::max(start.z, actorEnd.z) + extent.z };
            for (UObject* other : collisionHash.query(boxMin, boxMax)) {
                if (!other || other->destroyed || !actor::bCollideActors(other)) {
                    continue;
                }
                if (onlyMovers && !isMover(other)) {
                    continue;
                }
                bool brush = isBrush(other);
                if (!brush) {
                    Vector location = actor::Location(other);
                    float radius = actor::CollisionRadius(other);
                    float height = actor::CollisionHeight(other);
                    if (location.x + radius < boxMin.x || location.x - radius > boxMax.x || location.y + radius < boxMin.y || location.y - radius > boxMax.y || location.z + height < boxMin.z || location.z - height > boxMax.z) {
                        continue;
                    }
                }
                if (!actorCollides(other, source, flags)) {
                    continue;
                }
                float time = 1;
                Vector normal;
                bool collided = false;
                if (brush) {
                    collided = brushTrace(other, start, actorEnd, extent, time, normal);
                } else {
                    collided = cylinderTrace(other, start, actorEnd, extent, time, normal);
                }
                if (collided) {
                    CheckResult hit;
                    hit.actor = other;
                    hit.time = time * dilation;
                    hit.normal = normal;
                    hit.location = start + (actorEnd - start) * time;
                    if (!hits.empty() && hits.front().actor == levelInfo && isMover(other) && (hit.normal - hits.front().normal).sizeSquared() < 1e-6f && (hit.location - hits.front().location).sizeSquared() < 4.0f) {
                        Vector direction = (actorEnd - start).normal();
                        hit.location = hits.front().location - direction * 2.0f;
                        hit.time = (hit.location - start).size() / (end - start).size();
                    }
                    hits.push_back(hit);
                }
            }
        }
        std::stable_sort(hits.begin(), hits.end(), [](const CheckResult& a, const CheckResult& b) {
            return a.time < b.time;
        });
        return hits;
    }

    bool World::lineCheck(CheckResult& hit, const Vector& start, const Vector& end, const Vector& extent, UObject* source, uint32_t flags)
    {
        hit = CheckResult();
        hit.location = end;
        std::vector<CheckResult> hits = multiLineCheck(start, end, extent, source, flags);
        if (hits.empty()) {
            return false;
        }
        hit = hits.front();
        return true;
    }

    bool World::pointCheck(CheckResult& hit, const Vector& location, const Vector& extent, UObject* source, bool checkActors)
    {
        hit = CheckResult();
        if (modelOverlap(level ? level->model : nullptr, location, extent)) {
            hit.actor = levelInfo;
            hit.location = location;
            hit.time = 0;
            return true;
        }
        if (!checkActors) {
            return false;
        }
        for (UObject* other : collisionHash.query(location - extent, location + extent)) {
            if (!actorCollides(other, source, TRACE_Pawns | TRACE_Movers | TRACE_Others)) {
                continue;
            }
            bool overlap = false;
            if (isBrush(other)) {
                overlap = brushOverlap(other, location, extent);
            } else {
                Vector d = actor::Location(other) - location;
                float radius = actor::CollisionRadius(other) + extent.x;
                float height = actor::CollisionHeight(other) + extent.z;
                overlap = d.x * d.x + d.y * d.y < radius * radius && std::abs(d.z) < height;
            }
            if (overlap) {
                hit.actor = other;
                hit.location = location;
                hit.time = 0;
                return true;
            }
        }
        return false;
    }

    bool World::findSpot(const Vector& extent, Vector& location, bool checkActors, bool assumeFit)
    {
        CheckResult hit;
        if (extent.zero()) {
            return !pointCheck(hit, location, extent, nullptr, checkActors);
        }
        if (assumeFit && !pointCheck(hit, location, extent, nullptr, checkActors)) {
            return true;
        }
        Vector adjusted = location;
        auto adjust = [this](Vector& spot, const Vector& destination, float length) {
            CheckResult trace;
            if (lineCheck(trace, spot, destination, {}, nullptr, TRACE_World) && trace.time < 1.0f) {
                spot = spot + trace.normal * ((1.05f - trace.time) * length);
            }
        };
        for (int i = -1; i < 2; i += 2) {
            float sign = static_cast<float>(i);
            adjust(adjusted, adjusted + Vector { sign * extent.x, 0, 0 }, extent.x);
            adjust(adjusted, adjusted + Vector { 0, sign * extent.y, 0 }, extent.y);
            adjust(adjusted, adjusted + Vector { 0, 0, sign * extent.z }, extent.z);
        }
        if (!pointCheck(hit, adjusted, extent, nullptr, checkActors)) {
            location = adjusted;
            return true;
        }
        float length = extent.size() + 2.0f;
        for (int i = -1; i < 2; i += 2) {
            for (int j = -1; j < 2; j += 2) {
                for (int k = -1; k < 2; k += 2) {
                    adjust(adjusted, adjusted + Vector { static_cast<float>(i) * extent.x, static_cast<float>(j) * extent.y, static_cast<float>(k) * extent.z }, length);
                }
            }
        }
        if ((adjusted - location).sizeSquared() > 1.5f * extent.sizeSquared()) {
            return false;
        }
        if (!pointCheck(hit, adjusted, extent, nullptr, checkActors)) {
            location = adjusted;
            return true;
        }
        return false;
    }

    bool World::encroaches(UObject* a, const Vector& location)
    {
        if (!actor::bCollideActors(a) && !actor::bBlockActors(a) && !actor::bBlockPlayers(a) && !isMovingBrush(a)) {
            return false;
        }
        std::vector<UObject*> encroached;
        Vector extent = cylinderExtent(a);
        const std::vector<UObject*>& candidates = isMovingBrush(a) ? actors : collisionHash.query(location - extent, location + extent);
        for (UObject* other : candidates) {
            if (!other || other == a || other->destroyed || other == levelInfo || !actor::bCollideActors(other)) {
                continue;
            }
            bool overlap = false;
            if (isMovingBrush(a)) {
                if (isBrush(other)) {
                    continue;
                }
                Vector saved = actor::Location(a);
                actor::Location(a) = location;
                overlap = brushOverlap(a, actor::Location(other), cylinderExtent(other));
                actor::Location(a) = saved;
            } else if (isBrush(other)) {
                overlap = brushOverlap(other, location, cylinderExtent(a));
            } else {
                Vector d = actor::Location(other) - location;
                float radius = actor::CollisionRadius(other) + actor::CollisionRadius(a);
                float height = actor::CollisionHeight(other) + actor::CollisionHeight(a);
                overlap = d.x * d.x + d.y * d.y < radius * radius && std::abs(d.z) < height;
            }
            if (overlap) {
                encroached.push_back(other);
            }
        }
        for (UObject* other : encroached) {
            if (!isBlockedBy(a, other)) {
                continue;
            }
            if (isMovingBrush(a) && !isMovingBrush(other)) {
                Vector push = location - actor::Location(a);
                Vector old = actor::Location(other);
                moveSmooth(other, push);
                Vector saved = actor::Location(a);
                actor::Location(a) = location;
                bool still = brushOverlap(a, actor::Location(other), cylinderExtent(other));
                actor::Location(a) = saved;
                if (!still) {
                    continue;
                }
                actor::Location(a) = location;
                moveSmooth(other, push * -1.0f);
                actor::Location(a) = saved;
                (void)old;
            }
            vm::Call call(a, Name("EncroachingOn"));
            if (call.valid()) {
                call.obj(other).run();
                uint8_t* result = call.returnValue();
                if (result && *reinterpret_cast<uint32_t*>(result)) {
                    return true;
                }
            }
        }
        for (UObject* other : encroached) {
            if (isBlockedBy(a, other)) {
                vm::Call(other, Name("EncroachedBy")).obj(a).run();
            }
        }
        return false;
    }

    bool World::farMoveActor(UObject* a, const Vector& destination, bool test, bool noCheck)
    {
        if (actor::bStatic(a) || !actor::bMovable(a)) {
            return false;
        }
        Vector newLocation = destination;
        bool result = true;
        if ((!noCheck && actor::bCollideWorld(a)) || actor::bCollideWhenPlacing(a)) {
            result = findSpot(cylinderExtent(a), newLocation, false, false);
        }
        if (result && !test && !noCheck) {
            result = !encroaches(a, newLocation);
        }
        if (result) {
            if (!test) {
                if (actor::StandingCount(a) > 0) {
                    for (UObject* other : actors) {
                        if (other && actor::Base(other) == a) {
                            setBase(other, nullptr, true);
                        }
                    }
                }
                actor::bJustTeleported(a) = 1;
            }
            actor::Location(a) = newLocation;
            actor::OldLocation(a) = newLocation;
            collisionHash.update(a);
            setLocationZone(a, newLocation, test);
        }
        return result;
    }

    bool World::moveActor(UObject* a, const Vector& delta, const Rotator& rotation, CheckResult& hit, bool test, bool ignorePawns, bool ignoreBases, bool noFail)
    {
        if (!a || a->destroyed || actor::bStatic(a) || !actor::bMovable(a)) {
            return false;
        }
        hit = CheckResult();
        bool movingBrush = isMovingBrush(a);
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 1e-8f) {
            if (rotation == actor::Rotation(a)) {
                return true;
            }
            if (!actor::StandingCount(a) && !movingBrush) {
                actor::Rotation(a) = rotation;
                return true;
            }
        }
        float deltaSize = delta.size();
        Vector deltaDir = deltaSize > 0 ? delta / deltaSize : Vector {};
        const float testAdjust = 2.0f;
        Vector testDelta = delta + deltaDir * testAdjust;
        bool maybeTouched = false;
        std::vector<CheckResult> hits;
        Vector location = actor::Location(a);
        if ((actor::bCollideActors(a) || actor::bCollideWorld(a)) && !movingBrush && deltaSize > 0) {
            uint32_t flags = 0;
            if (actor::bCollideActors(a)) {
                flags |= TRACE_Pawns | TRACE_Movers | TRACE_Others;
            }
            if (actor::bCollideWorld(a)) {
                flags |= TRACE_Level | TRACE_LevelGeometry | TRACE_Movers;
            }
            hits = multiLineCheck(location, location + testDelta, cylinderExtent(a), nullptr, flags);
            hits.erase(std::remove_if(hits.begin(), hits.end(), [a](const CheckResult& candidate) {
                return candidate.actor == a;
            }),
                hits.end());
            if (actor::bCollideWorld(a) || actor::bBlockActors(a) || actor::bBlockPlayers(a)) {
                for (const CheckResult& candidate : hits) {
                    UObject* other = candidate.actor;
                    bool pawnLike = isPawn(other) || other->isA(engineClasses().decoration);
                    if ((!ignorePawns || actor::bStatic(other) || !pawnLike) && (!ignoreBases || !isBasedOn(a, other)) && !isBasedOn(other, a)) {
                        maybeTouched = true;
                        if (isBlockedBy(a, other)) {
                            hit = candidate;
                            break;
                        }
                    }
                }
            }
        }
        Vector finalDelta = delta;
        if (hit.time < 1.0f && !noFail) {
            float finalSize = (deltaSize + testAdjust) * hit.time;
            if (finalSize <= testAdjust) {
                finalDelta = {};
                hit.time = 0;
            } else {
                finalDelta = testDelta * hit.time - deltaDir * testAdjust;
                hit.time = (finalSize - testAdjust) / deltaSize;
            }
        }
        if (actor::StandingCount(a) && !test) {
            Rotator oldRotation = actor::Rotation(a);
            int32_t deltaYaw = rotation.yaw - oldRotation.yaw;
            for (UObject* other : std::vector<UObject*>(actors)) {
                if (!other || other->destroyed || actor::Base(other) != a) {
                    continue;
                }
                Vector rotationMotion;
                if (!(rotation == oldRotation)) {
                    Rotator reduced { 0, (rotation.yaw & 65535) - (oldRotation.yaw & 65535), 0 };
                    Vector pointer = actor::Location(a) - actor::Location(other);
                    Coords spin = rotationAxes(reduced);
                    rotationMotion = pointer - transformVector(spin, pointer);
                }
                CheckResult otherHit;
                Rotator otherRotation = actor::Rotation(other);
                otherRotation.yaw += deltaYaw;
                moveActor(other, finalDelta + rotationMotion, otherRotation, otherHit, false, false, true, false);
                if (isPawn(other)) {
                    pawn::ViewRotation(other).yaw += deltaYaw;
                }
            }
        }
        if (!test && !noFail && !isPawn(a)) {
            Vector saved = actor::Location(a);
            Rotator savedRotation = actor::Rotation(a);
            actor::Rotation(a) = rotation;
            bool blocked = encroaches(a, saved + finalDelta);
            actor::Rotation(a) = savedRotation;
            if (blocked) {
                return false;
            }
        }
        actor::Location(a) = actor::Location(a) + finalDelta;
        actor::Rotation(a) = rotation;
        collisionHash.update(a);
        if (!test) {
            UObject* hitActor = hit.actor;
            if (hitActor && hitActor != levelInfo && !isBasedOn(a, hitActor)) {
                vm::Call(hitActor, Name("Bump")).obj(a).run();
                if (!a->destroyed) {
                    vm::Call(a, Name("Bump")).obj(hitActor).run();
                }
            }
            if (!a->destroyed && (maybeTouched || !actor::bBlockActors(a) || !actor::bBlockPlayers(a))) {
                for (const CheckResult& candidate : hits) {
                    if (candidate.time >= hit.time) {
                        break;
                    }
                    UObject* other = candidate.actor;
                    if (other == levelInfo || other->destroyed || a->destroyed) {
                        continue;
                    }
                    if (!isBasedOn(other, a) && (!ignoreBases || !isBasedOn(a, other)) && !isBlockedBy(a, other)) {
                        beginTouch(a, other);
                    }
                }
            }
            if (!a->destroyed) {
                for (int i = 0; i < 4; i++) {
                    UObject* touching = actor::Touching(a, i);
                    if (touching && !isOverlapping(a, touching)) {
                        endTouch(a, touching, false);
                    }
                }
            }
        }
        if (!a->destroyed) {
            setLocationZone(a, actor::Location(a), test);
        }
        return hit.time > 0.0f;
    }

    bool moveSmooth(UObject* a, const Vector& delta)
    {
        World* world = World::current;
        if (!world) {
            return false;
        }
        CheckResult hit;
        bool moved = world->moveActor(a, delta, actor::Rotation(a), hit, false, false, false, false);
        if (hit.time < 1.0f && !a->destroyed) {
            Vector adjusted = (delta - hit.normal * delta.dot(hit.normal)) * (1.0f - hit.time);
            if (delta.dot(adjusted) >= 0) {
                Vector oldNormal = hit.normal;
                Vector desired = delta.normal();
                world->moveActor(a, adjusted, actor::Rotation(a), hit, false, false, false, false);
                if (hit.time < 1.0f && !a->destroyed) {
                    vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                    if (oldNormal.dot(hit.normal) <= 0) {
                        Vector direction = hit.normal.cross(oldNormal).normal();
                        adjusted = direction * (adjusted.dot(direction) * (1.0f - hit.time));
                        if (desired.dot(adjusted) < 0) {
                            adjusted = adjusted * -1.0f;
                        }
                    } else {
                        adjusted = (adjusted - hit.normal * adjusted.dot(hit.normal)) * (1.0f - hit.time);
                        if (adjusted.dot(desired) <= 0) {
                            adjusted = {};
                        }
                    }
                    world->moveActor(a, adjusted, actor::Rotation(a), hit, false, false, false, false);
                }
            }
        }
        return moved;
    }
}
