#include "engine/brush_edit.h"

#include "engine/brush_builder.h"
#include "engine/bsp.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace dx {
    namespace brushedit {
        namespace {
            constexpr float VertexMatch = 0.1f;
            constexpr float PlanarThreshold = 0.1f;

            Rotator addRotators(const Rotator& a, const Rotator& b)
            {
                return { a.pitch + b.pitch, a.yaw + b.yaw, a.roll + b.roll };
            }

            Rotator subtractRotators(const Rotator& a, const Rotator& b)
            {
                return { a.pitch - b.pitch, a.yaw - b.yaw, a.roll - b.roll };
            }

            void writeUnitScale(uint8_t* data)
            {
                Vector unit { 1, 1, 1 };
                float sheer = 0;
                std::memcpy(data, &unit, sizeof(Vector));
                std::memcpy(data + sizeof(Vector), &sheer, sizeof(float));
                data[sizeof(Vector) + sizeof(float)] = 0;
            }

            UModel* brushModel(UObject* brush)
            {
                UModel* model = cast<UModel>(actor::Brush(brush));
                if (!model || !model->polys) {
                    return nullptr;
                }
                return model;
            }

            void filterInside(const UModel& model, int32_t index, const Poly& poly, bool outside, std::vector<Poly>& out)
            {
                if (index < 0) {
                    if (!outside) {
                        out.push_back(poly);
                    }
                    return;
                }
                const BspNode& node = model.nodes[static_cast<size_t>(index)];
                Vector normal { node.plane.x, node.plane.y, node.plane.z };
                bool csgNode = bsp::isCsg(node, 0);
                Poly front;
                Poly back;
                int side = bsp::splitWithPlane(poly, normal * node.plane.w, normal, &front, &back, false);
                if (side == bsp::SplitCoplanar) {
                    side = poly.normal.dot(normal) > 0.0f ? bsp::SplitBack : bsp::SplitFront;
                }
                if (side == bsp::SplitFront) {
                    filterInside(model, node.front, poly, outside || csgNode, out);
                } else if (side == bsp::SplitBack) {
                    filterInside(model, node.back, poly, outside && !csgNode, out);
                } else {
                    filterInside(model, node.front, front, outside || csgNode, out);
                    filterInside(model, node.back, back, outside && !csgNode, out);
                }
            }

            void axesFor(const Vector& normal, Vector& u, Vector& v)
            {
                Vector reference = std::fabs(normal.z) < 0.9f ? Vector { 0, 0, 1 } : Vector { 1, 0, 0 };
                u = reference.cross(normal).normal();
                v = normal.cross(u);
            }
        }

        Vector mainScale(UObject* brush)
        {
            Vector scale;
            std::memcpy(&scale, brush::MainScale(brush), sizeof(Vector));
            return scale;
        }

        void setMainScale(UObject* brush, const Vector& scale)
        {
            std::memcpy(brush::MainScale(brush), &scale, sizeof(Vector));
        }

        void mirror(UObject* brush, int axis)
        {
            Vector scale = mainScale(brush);
            if (axis == 0) {
                scale.x = -scale.x;
            } else if (axis == 1) {
                scale.y = -scale.y;
            } else {
                scale.z = -scale.z;
            }
            setMainScale(brush, scale);
        }

        void resetScale(UObject* brush)
        {
            writeUnitScale(brush::MainScale(brush));
            writeUnitScale(brush::PostScale(brush));
        }

        void resetRotation(UObject* brush)
        {
            actor::Rotation(brush) = {};
        }

        void resetAll(UObject* brush)
        {
            resetScale(brush);
            actor::Location(brush) = {};
            actor::Rotation(brush) = {};
            actor::PrePivot(brush) = {};
        }

        void applyTransform(UObject* brush)
        {
            UModel* model = brushModel(brush);
            if (!model) {
                return;
            }
            bsp::ModelCoords coords;
            float orientation = bsp::buildCoords(brush, &coords, nullptr);
            for (Poly& poly : model->polys->polys) {
                bsp::transformPoly(poly, coords, Vector {}, Vector {}, orientation);
                bsp::calcNormal(poly);
                if (!poly.vertices.empty()) {
                    poly.base = poly.vertices[0];
                }
            }
            actor::PrePivot(brush) = bsp::transformVector(actor::PrePivot(brush), coords.pointXform);
            resetScale(brush);
            actor::Rotation(brush) = {};
            bsp::buildBound(*model);
        }

        Vector toWorld(UObject* brush, const Vector& local)
        {
            bsp::ModelCoords coords;
            bsp::buildCoords(brush, &coords, nullptr);
            return bsp::transformVector(local - actor::PrePivot(brush), coords.pointXform) + actor::Location(brush);
        }

        Vector toLocal(UObject* brush, const Vector& world)
        {
            bsp::ModelCoords uncoords;
            bsp::buildCoords(brush, nullptr, &uncoords);
            return bsp::transformVector(world - actor::Location(brush), uncoords.pointXform) + actor::PrePivot(brush);
        }

        std::vector<Poly> worldPolys(UObject* brush)
        {
            UModel* model = brushModel(brush);
            if (!model) {
                return {};
            }
            bsp::ModelCoords coords;
            float orientation = bsp::buildCoords(brush, &coords, nullptr);
            std::vector<Poly> result = model->polys->polys;
            for (Poly& poly : result) {
                bsp::transformPoly(poly, coords, actor::PrePivot(brush), actor::Location(brush), orientation);
            }
            return result;
        }

        std::vector<Vector> localVertices(const UModel& model)
        {
            std::vector<Vector> result;
            if (!model.polys) {
                return result;
            }
            for (const Poly& poly : model.polys->polys) {
                for (const Vector& vertex : poly.vertices) {
                    bool known = std::any_of(result.begin(), result.end(), [&](const Vector& other) {
                        return (other - vertex).sizeSquared() < VertexMatch * VertexMatch;
                    });
                    if (!known) {
                        result.push_back(vertex);
                    }
                }
            }
            return result;
        }

        void snapVertices(UObject* brush, const Vector& grid)
        {
            UModel* model = brushModel(brush);
            if (!model) {
                return;
            }
            auto snap = [](float value, float size) {
                return size <= 0.0f ? value : std::round(value / size) * size;
            };
            for (Poly& poly : model->polys->polys) {
                for (Vector& vertex : poly.vertices) {
                    Vector world = toWorld(brush, vertex);
                    vertex = toLocal(brush, { snap(world.x, grid.x), snap(world.y, grid.y), snap(world.z, grid.z) });
                }
            }
            validate(*model);
        }

        void setPivot(UObject* brush, const Vector& world)
        {
            bsp::ModelCoords uncoords;
            bsp::buildCoords(brush, nullptr, &uncoords);
            actor::PrePivot(brush) += bsp::transformVector(world - actor::Location(brush), uncoords.pointXform);
            actor::Location(brush) = world;
        }

        void resetPivot(UObject* brush)
        {
            bsp::ModelCoords coords;
            bsp::buildCoords(brush, &coords, nullptr);
            actor::Location(brush) -= bsp::transformVector(actor::PrePivot(brush), coords.pointXform);
            actor::PrePivot(brush) = {};
        }

        size_t moveVertices(UModel& model, const std::vector<Vector>& from, const Vector& offset)
        {
            size_t count = 0;
            if (!model.polys) {
                return count;
            }
            for (Poly& poly : model.polys->polys) {
                for (Vector& vertex : poly.vertices) {
                    bool chosen = std::any_of(from.begin(), from.end(), [&](const Vector& other) {
                        return (other - vertex).sizeSquared() < VertexMatch * VertexMatch;
                    });
                    if (chosen) {
                        vertex += offset;
                        count++;
                    }
                }
            }
            return count;
        }

        std::vector<std::pair<Vector, Vector>> localEdges(const UModel& model)
        {
            std::vector<std::pair<Vector, Vector>> result;
            if (!model.polys) {
                return result;
            }
            auto near = [](const Vector& a, const Vector& b) {
                return (a - b).sizeSquared() < VertexMatch * VertexMatch;
            };
            for (const Poly& poly : model.polys->polys) {
                size_t count = poly.vertices.size();
                for (size_t i = 0; i < count; i++) {
                    const Vector& a = poly.vertices[i];
                    const Vector& b = poly.vertices[(i + 1) % count];
                    bool known = std::any_of(result.begin(), result.end(), [&](const std::pair<Vector, Vector>& edge) {
                        return (near(edge.first, a) && near(edge.second, b)) || (near(edge.first, b) && near(edge.second, a));
                    });
                    if (!known) {
                        result.emplace_back(a, b);
                    }
                }
            }
            return result;
        }

        size_t transformVertices(UModel& model, const std::vector<Vector>& from, const std::function<Vector(const Vector&)>& transform)
        {
            size_t count = 0;
            if (!model.polys) {
                return count;
            }
            for (Poly& poly : model.polys->polys) {
                for (Vector& vertex : poly.vertices) {
                    bool chosen = std::any_of(from.begin(), from.end(), [&](const Vector& other) {
                        return (other - vertex).sizeSquared() < VertexMatch * VertexMatch;
                    });
                    if (chosen) {
                        vertex = transform(vertex);
                        count++;
                    }
                }
            }
            return count;
        }

        int32_t findPoly(const UModel& model, const std::vector<Vector>& vertices)
        {
            if (!model.polys) {
                return -1;
            }
            const std::vector<Poly>& polys = model.polys->polys;
            for (size_t i = 0; i < polys.size(); i++) {
                if (polys[i].vertices.size() != vertices.size()) {
                    continue;
                }
                bool same = std::all_of(vertices.begin(), vertices.end(), [&](const Vector& vertex) {
                    return std::any_of(polys[i].vertices.begin(), polys[i].vertices.end(), [&](const Vector& other) {
                        return (other - vertex).sizeSquared() < VertexMatch * VertexMatch;
                    });
                });
                if (same) {
                    return static_cast<int32_t>(i);
                }
            }
            return -1;
        }

        bool extrudeFace(UModel& model, size_t index, const Vector& offset)
        {
            if (!model.polys || index >= model.polys->polys.size() || offset.sizeSquared() < 0.0001f) {
                return false;
            }
            Poly face = model.polys->polys[index];
            if (offset.dot(face.normal) <= 0.0f) {
                moveVertices(model, face.vertices, offset);
                validate(model);
                return true;
            }
            std::vector<Poly>& polys = model.polys->polys;
            size_t first = polys.size();
            size_t count = face.vertices.size();
            for (size_t i = 0; i < count; i++) {
                const Vector& a = face.vertices[i];
                const Vector& b = face.vertices[(i + 1) % count];
                Poly side = face;
                side.vertices = { a, b, b + offset, a + offset };
                side.base = a;
                side.normal = {};
                side.textureU = {};
                side.textureV = {};
                side.panU = 0;
                side.panV = 0;
                bsp::finalizePoly(side);
                polys.push_back(side);
            }
            Poly& moved = polys[index];
            for (Vector& vertex : moved.vertices) {
                vertex += offset;
            }
            moved.base += offset;
            for (size_t i = 0; i < polys.size(); i++) {
                polys[i].link = static_cast<int32_t>(i);
            }
            for (size_t s = first; s < polys.size(); s++) {
                for (size_t j = 0; j < polys.size(); j++) {
                    Poly& other = polys[j];
                    if (j == s || other.link != static_cast<int32_t>(j) || other.texture != polys[s].texture || other.normal.dot(polys[s].normal) < 0.9999f) {
                        continue;
                    }
                    if (std::fabs((other.base - polys[s].base).dot(polys[s].normal)) < 0.001f) {
                        other.link = polys[s].link;
                    }
                }
            }
            bsp::mergeCoplanars(model, false, true);
            validate(model);
            return true;
        }

        int32_t rayPoly(const std::vector<Poly>& polys, const Vector& start, const Vector& direction, float& distance)
        {
            int32_t best = -1;
            distance = 1e9f;
            for (size_t i = 0; i < polys.size(); i++) {
                const Poly& poly = polys[i];
                if (poly.vertices.size() < 3) {
                    continue;
                }
                float facing = poly.normal.dot(direction);
                if (std::fabs(facing) < 1e-6f) {
                    continue;
                }
                float t = poly.normal.dot(poly.vertices[0] - start) / facing;
                if (t <= 0.0f || t >= distance) {
                    continue;
                }
                Vector hit = start + direction * t;
                bool front = true;
                bool back = true;
                size_t count = poly.vertices.size();
                for (size_t j = 0; j < count; j++) {
                    const Vector& a = poly.vertices[j];
                    const Vector& b = poly.vertices[(j + 1) % count];
                    float side = (b - a).cross(hit - a).dot(poly.normal);
                    front = front && side >= -0.01f;
                    back = back && side <= 0.01f;
                }
                if (front || back) {
                    distance = t;
                    best = static_cast<int32_t>(i);
                }
            }
            return best;
        }

        void validate(UModel& model)
        {
            if (!model.polys) {
                return;
            }
            std::vector<Poly> result;
            for (Poly poly : model.polys->polys) {
                poly.polyFlags &= ~(bsp::PF_EdCut | bsp::PF_EdProcessed);
                if (bsp::fixPoly(poly) < 3 || !bsp::calcNormal(poly)) {
                    continue;
                }
                const Vector& first = poly.vertices[0];
                bool planar = std::all_of(poly.vertices.begin(), poly.vertices.end(), [&](const Vector& vertex) {
                    return std::fabs((vertex - first).dot(poly.normal)) < PlanarThreshold;
                });
                if (planar) {
                    poly.base = first;
                    result.push_back(poly);
                    continue;
                }
                for (size_t i = 1; i + 1 < poly.vertices.size(); i++) {
                    Poly triangle = poly;
                    triangle.vertices = { first, poly.vertices[i], poly.vertices[i + 1] };
                    if (bsp::calcNormal(triangle)) {
                        triangle.base = first;
                        result.push_back(triangle);
                    }
                }
            }
            model.polys->polys = result;
            brushbuilder::validateBrush(model, true);
        }

        std::vector<Poly> clipPolys(const std::vector<Poly>& polys, const Vector& base, const Vector& normal)
        {
            std::vector<Poly> result;
            Vector center;
            size_t count = 0;
            for (const Poly& poly : polys) {
                Poly front;
                Poly back;
                int side = bsp::splitWithPlane(poly, base, normal, &front, &back, false);
                if (side == bsp::SplitBack) {
                    result.push_back(poly);
                } else if (side == bsp::SplitSplit) {
                    back.polyFlags &= ~bsp::PF_EdCut;
                    result.push_back(back);
                }
                if (side == bsp::SplitBack || side == bsp::SplitSplit) {
                    result.back().link = static_cast<int32_t>(result.size() - 1);
                }
                for (const Vector& vertex : poly.vertices) {
                    center += vertex;
                    count++;
                }
            }
            if (count == 0) {
                return result;
            }
            center = center / static_cast<float>(count);
            float extent = 256.0f;
            for (const Poly& poly : polys) {
                for (const Vector& vertex : poly.vertices) {
                    extent = std::max(extent, (vertex - center).size() * 2.0f + 256.0f);
                }
            }
            center = center - normal * normal.dot(center - base);
            UModel solid;
            UPolys solidPolys;
            solid.polys = &solidPolys;
            solid.rootOutside = 1;
            bsp::emptyModel(solid, true, true);
            for (const Poly& poly : polys) {
                if (!(poly.polyFlags & (bsp::PF_NotSolid | bsp::PF_Semisolid))) {
                    solidPolys.polys.push_back(poly);
                }
            }
            bsp::build(solid, bsp::Optimization::Lame, 0, 1);
            Vector u;
            Vector v;
            axesFor(normal, u, v);
            Poly cap;
            cap.normal = normal;
            cap.textureU = u;
            cap.textureV = v;
            cap.texture = polys[0].texture;
            cap.vertices = { center - u * extent - v * extent, center + u * extent - v * extent, center + u * extent + v * extent, center - u * extent + v * extent };
            cap.base = cap.vertices[0];
            std::vector<Poly> caps;
            if (!solid.nodes.empty()) {
                filterInside(solid, 0, cap, solid.rootOutside != 0, caps);
            }
            int32_t capLink = static_cast<int32_t>(result.size());
            for (Poly& piece : caps) {
                piece.polyFlags &= ~bsp::PF_EdCut;
                piece.link = capLink;
                result.push_back(piece);
            }
            return result;
        }

        bool clipBrush(UModel& model, const Vector& base, const Vector& normal)
        {
            if (!model.polys) {
                return false;
            }
            std::vector<Poly> result = clipPolys(model.polys->polys, base, normal);
            if (result.size() < 4) {
                return false;
            }
            model.polys->polys = result;
            bsp::mergeCoplanars(model, false, true);
            validate(model);
            return model.polys->polys.size() >= 4;
        }

        bool clipPlane(UObject* brush, const std::vector<Vector>& markers, int viewAxis, Vector& base, Vector& normal)
        {
            if (markers.size() < 2) {
                return false;
            }
            Vector third = markers[0];
            if (markers.size() >= 3) {
                third = markers[2];
            } else if (viewAxis == 0) {
                third.x -= 64.0f;
            } else if (viewAxis == 1) {
                third.y -= 64.0f;
            } else {
                third.z -= 64.0f;
            }
            float orientation = bsp::buildCoords(brush, nullptr, nullptr);
            Vector a = toLocal(brush, markers[0]);
            Vector b = toLocal(brush, markers[1]);
            Vector c = toLocal(brush, third);
            Vector cross = (b - a).cross(c - a) * orientation;
            if (cross.sizeSquared() < 0.0001f) {
                return false;
            }
            normal = cross.normal();
            base = a;
            return true;
        }

        void showKey(UObject* mover, int key)
        {
            uint8_t& keyNum = mover::KeyNum(mover);
            keyNum = static_cast<uint8_t>(std::clamp(key, 0, 7));
            mover::BasePos(mover) = actor::Location(mover) - mover::OldPos(mover);
            mover::BaseRot(mover) = subtractRotators(actor::Rotation(mover), mover::OldRot(mover));
            mover::OldPos(mover) = mover::KeyPos(mover, keyNum);
            mover::OldRot(mover) = mover::KeyRot(mover, keyNum);
            actor::Location(mover) = mover::BasePos(mover) + mover::OldPos(mover);
            actor::Rotation(mover) = addRotators(mover::BaseRot(mover), mover::OldRot(mover));
            moverMoved(mover);
        }

        void moverMoved(UObject* mover)
        {
            int keyNum = std::clamp<int>(mover::KeyNum(mover), 0, 7);
            if (keyNum == 0) {
                mover::BasePos(mover) = actor::Location(mover) - mover::OldPos(mover);
                mover::BaseRot(mover) = subtractRotators(actor::Rotation(mover), mover::OldRot(mover));
            } else {
                mover::KeyPos(mover, keyNum) = actor::Location(mover) - (mover::BasePos(mover) + mover::KeyPos(mover, 0));
                mover::KeyRot(mover, keyNum) = subtractRotators(actor::Rotation(mover), addRotators(mover::BaseRot(mover), mover::KeyRot(mover, 0)));
                mover::OldPos(mover) = mover::KeyPos(mover, keyNum);
                mover::OldRot(mover) = mover::KeyRot(mover, keyNum);
            }
            actor::Location(mover) = mover::BasePos(mover) + mover::KeyPos(mover, keyNum);
        }
    }
}
