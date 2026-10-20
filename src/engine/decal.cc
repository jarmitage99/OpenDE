#include "engine/decal.h"

#include "core/objects.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace dx {
    namespace {
        constexpr uint32_t PF_AutoUPan = 0x00000200;
        constexpr uint32_t PF_AutoVPan = 0x00000400;
        constexpr uint8_t NF_ShootThrough = 0x02;
        constexpr uint8_t NF_NotVisBlocking = 0x04;

        float decalRandom()
        {
            static std::mt19937 generator(5150);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        Vector unitOrSame(const Vector& value)
        {
            double squared = static_cast<double>(value.sizeSquared());
            if (squared < 1e-8) {
                return value;
            }
            return value * static_cast<float>(1.0 / std::sqrt(squared));
        }

        Vector edgeNormal(const Vector& from, const Vector& to, const Vector& normal)
        {
            Vector edge = to - from;
            return { edge.y * normal.z - edge.z * normal.y, edge.z * normal.x - edge.x * normal.z, edge.x * normal.y - edge.y * normal.x };
        }

        const Vector& nodePoint(UModel* model, const BspNode& node, int32_t index)
        {
            return model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + index)].vertex)];
        }

        Vector planeHit(const Vector& start, const Vector& through, const Vector& base, const Vector& normal)
        {
            float t = (base - start).dot(normal) / (through - start).dot(normal);
            return start + (through - start) * t;
        }

        const std::vector<int32_t>& surfNodes(World& world, UModel* model, int32_t surf)
        {
            DecalRegistry& registry = world.decals;
            if (registry.model != model || registry.surfNodes.size() != model->surfs.size()) {
                registry.model = model;
                registry.surfNodes.assign(model->surfs.size(), {});
                for (size_t i = 0; i < model->nodes.size(); i++) {
                    int32_t owner = model->nodes[i].surf;
                    if (owner >= 0 && static_cast<size_t>(owner) < model->surfs.size()) {
                        registry.surfNodes[static_cast<size_t>(owner)].push_back(static_cast<int32_t>(i));
                    }
                }
            }
            return registry.surfNodes[static_cast<size_t>(surf)];
        }

        void collectNodes(World& world, UModel* model, DecalPiece& piece)
        {
            for (int32_t node : surfNodes(world, model, piece.surf)) {
                if (model->nodes[static_cast<size_t>(node)].numVertices == 0) {
                    continue;
                }
                std::vector<Vector> polygon(piece.vertices.begin(), piece.vertices.end());
                clipDecalPolygon(model, node, polygon);
                if (!polygon.empty()) {
                    piece.nodes.push_back(node);
                }
            }
        }

        bool surfAccepts(UModel* model, int32_t surf)
        {
            return !(model->surfs[static_cast<size_t>(surf)].polyFlags & (PF_AutoUPan | PF_AutoVPan));
        }
    }

    int32_t decalFindSurf(UModel* model, int32_t node, const Vector& point)
    {
        while (node >= 0 && static_cast<size_t>(node) < model->nodes.size()) {
            const BspNode& current = model->nodes[static_cast<size_t>(node)];
            if (current.numVertices > 0) {
                const Vector& normal = model->vectors[static_cast<size_t>(model->surfs[static_cast<size_t>(current.surf)].vNormal)];
                float previous = 0;
                bool inside = true;
                for (int32_t i = 0; i < current.numVertices; i++) {
                    const Vector& from = nodePoint(model, current, i == 0 ? current.numVertices - 1 : i - 1);
                    const Vector& to = nodePoint(model, current, i);
                    Vector side = edgeNormal(from, to, normal);
                    float distance = side.dot(point) - side.dot(to);
                    if ((distance < 0 && previous > 0) || (distance > 0 && previous < 0)) {
                        inside = false;
                        break;
                    }
                    previous = distance;
                }
                if (inside) {
                    return current.surf;
                }
            }
            node = current.iPlane;
        }
        return -1;
    }

    void clipDecalPolygon(UModel* model, int32_t node, std::vector<Vector>& polygon)
    {
        clipDecalSide(model, node, polygon, 1.0f);
    }

    void clipDecalToNode(UModel* model, int32_t node, std::vector<Vector>& polygon)
    {
        const BspNode& current = model->nodes[static_cast<size_t>(node)];
        const Vector& normal = model->vectors[static_cast<size_t>(model->surfs[static_cast<size_t>(current.surf)].vNormal)];
        Vector center;
        for (int32_t i = 0; i < current.numVertices; i++) {
            center += nodePoint(model, current, i);
        }
        center = center / static_cast<float>(current.numVertices);
        Vector side = edgeNormal(nodePoint(model, current, current.numVertices - 1), nodePoint(model, current, 0), normal);
        clipDecalSide(model, node, polygon, side.dot(center) - side.dot(nodePoint(model, current, 0)) < 0 ? -1.0f : 1.0f);
    }

    void clipDecalSide(UModel* model, int32_t node, std::vector<Vector>& polygon, float inside)
    {
        const BspNode& current = model->nodes[static_cast<size_t>(node)];
        const Vector& normal = model->vectors[static_cast<size_t>(model->surfs[static_cast<size_t>(current.surf)].vNormal)];
        std::vector<float> distances;
        std::vector<Vector> clipped;
        for (int32_t i = 0; i < current.numVertices && !polygon.empty(); i++) {
            const Vector& from = nodePoint(model, current, i == 0 ? current.numVertices - 1 : i - 1);
            const Vector& to = nodePoint(model, current, i);
            Vector side = edgeNormal(from, to, normal) * inside;
            float offset = side.dot(to);
            distances.clear();
            for (const Vector& point : polygon) {
                distances.push_back(side.dot(point) - offset);
            }
            clipped.clear();
            for (size_t k = 0; k < polygon.size(); k++) {
                size_t next = (k + 1) % polygon.size();
                if (distances[k] >= 0) {
                    clipped.push_back(polygon[k]);
                }
                if ((distances[k] > 0 && distances[next] < 0) || (distances[k] < 0 && distances[next] > 0)) {
                    Vector delta = polygon[next] - polygon[k];
                    clipped.push_back(polygon[k] + delta * ((offset - side.dot(polygon[k])) / side.dot(delta)));
                }
            }
            polygon.swap(clipped);
        }
    }

    bool attachDecal(World& world, UObject* decal, float traceDistance, Vector decalDir)
    {
        UModel* model = world.level ? world.level->model : nullptr;
        UTexture* texture = cast<UTexture>(actor::Texture(decal));
        if (!model || model->nodes.empty() || !texture) {
            return false;
        }
        objects::ensureLoaded(texture);
        decal::MultiDecalLevel(decal) = std::min(decal::MultiDecalLevel(decal), 4);
        Vector forward = rotationVector(actor::Rotation(decal));
        Vector trace = unitOrSame(forward * -1.0f) * traceDistance;
        bool randomDirection = false;
        if (decalDir.x == 0 && decalDir.y == 0 && decalDir.z == 0) {
            do {
                decalDir.x = decalRandom() * 2.0f - 1.0f;
                decalDir.y = decalRandom() * 2.0f - 1.0f;
                decalDir.z = decalRandom() * 2.0f - 1.0f;
            } while (decalDir.sizeSquared() > 1.0f);
            decalDir = decalDir.normal();
            randomDirection = true;
        }
        Vector location = actor::Location(decal);
        Vector end = location + trace;
        CheckResult hit;
        if (!world.bspLineCheck(hit, location, end, {}, NF_ShootThrough | NF_NotVisBlocking) || hit.item < 0) {
            return false;
        }
        const BspSurf& hitSurf = model->surfs[static_cast<size_t>(model->nodes[static_cast<size_t>(hit.item)].surf)];
        Vector firstHit = planeHit(location, end, model->points[static_cast<size_t>(hitSurf.pBase)], model->vectors[static_cast<size_t>(hitSurf.vNormal)]);
        int32_t surf = decalFindSurf(model, hit.item, firstHit);
        if (surf < 0) {
            return false;
        }
        const BspSurf& primary = model->surfs[static_cast<size_t>(surf)];
        const Vector& normal = model->vectors[static_cast<size_t>(primary.vNormal)];
        const Vector& base = model->points[static_cast<size_t>(primary.pBase)];
        Vector hitPoint = planeHit(location, end, base, normal);
        if (forward.dot(normal) == 0 || std::abs((base - hitPoint).dot(normal)) > 0.001f || !surfAccepts(model, surf)) {
            return false;
        }
        std::vector<DecalPiece>& pieces = world.decals.pieces[decal];
        std::vector<int32_t> surfList;
        for (const DecalPiece& existing : pieces) {
            surfList.push_back(existing.surf);
        }
        surfList.push_back(surf);
        DecalPiece piece;
        piece.surf = surf;
        float radius = static_cast<float>(std::sqrt(static_cast<double>(static_cast<float>(texture->width()) * actor::DrawScale(decal) * actor::DrawScale(decal) * static_cast<float>(texture->width()) * 0.5f)));
        Vector first;
        Vector second;
        Vector projected = decalDir - normal * decalDir.dot(normal);
        bool degenerate = std::abs(projected.x) < 0.0001f && std::abs(projected.y) < 0.0001f && std::abs(projected.z) < 0.0001f;
        if (!randomDirection && !degenerate) {
            Vector across { projected.z * normal.y - projected.y * normal.z, projected.x * normal.z - projected.z * normal.x, projected.y * normal.x - projected.x * normal.y };
            projected = unitOrSame(projected);
            across = unitOrSame(across);
            first = across + projected;
            second = projected - across;
        } else {
            Vector direction = randomDirection ? decalDir : base - hitPoint;
            first = direction - normal * direction.dot(normal);
            second = { normal.y * first.z - normal.z * first.y, normal.z * first.x - normal.x * first.z, normal.x * first.y - normal.y * first.x };
        }
        first = unitOrSame(first) * radius;
        second = unitOrSame(second) * radius;
        piece.vertices = { hitPoint + first, hitPoint + second, hitPoint - first, hitPoint - second };
        collectNodes(world, model, piece);
        pieces.push_back(piece);
        int32_t levels = decal::MultiDecalLevel(decal);
        Vector lift = unitOrSame(normal) * -16.0f;
        Vector edgeU = piece.vertices[1] - piece.vertices[0];
        Vector edgeV = piece.vertices[3] - piece.vertices[0];
        for (int32_t i = 0; i < levels; i++) {
            for (int32_t j = 0; j < levels; j++) {
                Vector sample = piece.vertices[0] + edgeU * static_cast<float>((static_cast<double>(i) + 1.0) / static_cast<double>(levels)) + edgeV * static_cast<float>((static_cast<double>(j) + 1.0) / static_cast<double>(levels));
                Vector start = sample - lift;
                Vector stop = sample + lift;
                CheckResult sampleHit;
                if (!world.bspLineCheck(sampleHit, start, stop, {}, NF_ShootThrough | NF_NotVisBlocking) || sampleHit.item < 0) {
                    continue;
                }
                const BspSurf& sampleSurf = model->surfs[static_cast<size_t>(model->nodes[static_cast<size_t>(sampleHit.item)].surf)];
                Vector samplePoint = planeHit(start, stop, model->points[static_cast<size_t>(sampleSurf.pBase)], model->vectors[static_cast<size_t>(sampleSurf.vNormal)]);
                int32_t other = decalFindSurf(model, sampleHit.item, samplePoint);
                if (other < 0 || std::find(surfList.begin(), surfList.end(), other) != surfList.end() || !surfAccepts(model, other)) {
                    continue;
                }
                const BspSurf& otherSurf = model->surfs[static_cast<size_t>(other)];
                const Vector& otherNormal = model->vectors[static_cast<size_t>(otherSurf.vNormal)];
                const Vector& otherBase = model->points[static_cast<size_t>(otherSurf.pBase)];
                float cosine = otherNormal.dot(normal) / (static_cast<float>(std::sqrt(static_cast<double>(normal.sizeSquared()))) * static_cast<float>(std::sqrt(static_cast<double>(otherNormal.sizeSquared()))));
                if (std::abs(cosine) <= 0.7f) {
                    continue;
                }
                surfList.push_back(other);
                DecalPiece extra;
                extra.surf = other;
                for (size_t k = 0; k < 4; k++) {
                    const Vector& vertex = piece.vertices[k];
                    Vector along = planeHit(vertex - normal, vertex, otherBase, otherNormal);
                    Vector projectedVertex = planeHit(vertex - otherNormal, vertex, otherBase, otherNormal);
                    float projectedDistance = (vertex - projectedVertex).size() / cosine;
                    float alongDistance = (vertex - along).size() / cosine;
                    Vector slide = (projectedVertex - along).normal();
                    extra.vertices[k] = projectedVertex - slide * (alongDistance - projectedDistance);
                }
                collectNodes(world, model, extra);
                pieces.push_back(extra);
            }
        }
        return true;
    }

    void detachDecal(World& world, UObject* decal)
    {
        world.decals.pieces.erase(decal);
    }
}
