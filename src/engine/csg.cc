#include "engine/csg.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/brush_builder.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace dx {
    namespace csg {
        namespace {
            constexpr uint32_t LoadFlags = 0x00070000;
            constexpr float ScriptPi = 3.1415926535897932f;

            struct BuilderPoly {
                std::vector<int32_t> indices;
                int32_t direction = 1;
                Name item;
                uint32_t flags = 0;
            };

            void finishPolys(UPolys& polys, const std::vector<Vector>& vertices, std::vector<BuilderPoly> builderPolys, UObject* texture)
            {
                polys.polys.clear();
                for (BuilderPoly& builderPoly : builderPolys) {
                    if (builderPoly.direction < 0) {
                        std::reverse(builderPoly.indices.begin(), builderPoly.indices.end());
                    }
                    for (;;) {
                        size_t count = std::min<size_t>(16, builderPoly.indices.size());
                        Poly poly;
                        poly.link = -1;
                        poly.brushPoly = -1;
                        poly.itemName = builderPoly.item;
                        poly.base = vertices[static_cast<size_t>(builderPoly.indices[0])];
                        poly.polyFlags = builderPoly.flags;
                        for (size_t j = 0; j < count; j++) {
                            poly.vertices.push_back(vertices[static_cast<size_t>(builderPoly.indices[j])]);
                        }
                        bsp::finalizePoly(poly);
                        poly.texture = texture;
                        polys.polys.push_back(poly);
                        if (count == builderPoly.indices.size()) {
                            break;
                        }
                        builderPoly.indices.erase(builderPoly.indices.begin() + 1, builderPoly.indices.begin() + 1 + static_cast<std::ptrdiff_t>(count - 2));
                    }
                }
                for (size_t i = 0; i < polys.polys.size(); i++) {
                    polys.polys[i].link = static_cast<int32_t>(i);
                }
            }

            UModel* copyBrushModel(UObject* source, UObject* package, uint32_t flags)
            {
                UModel* sourceModel = cast<UModel>(actor::Brush(source));
                if (!sourceModel) {
                    return nullptr;
                }
                objects::ensureLoaded(sourceModel);
                objects::flushLoads();
                if (!sourceModel->polys) {
                    return nullptr;
                }
                objects::ensureLoaded(sourceModel->polys);
                objects::flushLoads();
                if (sourceModel->polys->polys.empty()) {
                    return nullptr;
                }
                UModel* model = cast<UModel>(objects::newObject(nativeClass("Model"), package, Name(), flags));
                UPolys* polys = cast<UPolys>(objects::newObject(nativeClass("Polys"), package, Name(), flags));
                if (!model || !polys) {
                    return nullptr;
                }
                model->loaded = true;
                polys->loaded = true;
                model->rootOutside = sourceModel->rootOutside;
                polys->polys = sourceModel->polys->polys;
                for (Poly& poly : polys->polys) {
                    if (!poly.texture) {
                        poly.texture = bsp::currentTexture();
                    }
                    poly.brushPoly = -1;
                }
                model->polys = polys;
                bsp::buildBound(*model);
                return model;
            }

            void copyPosRotScale(UObject* dest, UObject* source)
            {
                actor::Location(dest) = actor::Location(source);
                actor::Rotation(dest) = actor::Rotation(source);
                actor::PrePivot(dest) = actor::PrePivot(source);
                std::memcpy(brush::MainScale(dest), brush::MainScale(source), sizeof(Vector) + sizeof(float) + 1);
                std::memcpy(brush::PostScale(dest), brush::PostScale(source), sizeof(Vector) + sizeof(float) + 1);
            }
        }

        void updateBounds(UModel& model)
        {
            bsp::buildBound(model);
        }

        void buildCube(UPolys& polys, float width, float depth, float height, UObject* texture)
        {
            std::vector<Vector> vertices;
            for (int i = -1; i < 2; i += 2) {
                for (int j = -1; j < 2; j += 2) {
                    for (int k = -1; k < 2; k += 2) {
                        vertices.push_back({ static_cast<float>(i) * width / 2.0f, static_cast<float>(j) * depth / 2.0f, static_cast<float>(k) * height / 2.0f });
                    }
                }
            }
            std::vector<BuilderPoly> list = {
                { { 0, 1, 3, 2 }, 1, Name(), 0 },
                { { 2, 3, 7, 6 }, 1, Name(), 0 },
                { { 6, 7, 5, 4 }, 1, Name(), 0 },
                { { 4, 5, 1, 0 }, 1, Name(), 0 },
                { { 3, 1, 5, 7 }, 1, Name(), 0 },
                { { 0, 2, 6, 4 }, 1, Name(), 0 },
            };
            finishPolys(polys, vertices, list, texture);
        }

        void buildCylinder(UPolys& polys, float radius, float height, int sides, UObject* texture)
        {
            sides = std::max(sides, 3);
            std::vector<Vector> vertices;
            for (int i = 0; i < sides; i++) {
                for (int j = -1; j < 2; j += 2) {
                    float angle = static_cast<float>(2 * i) * ScriptPi / static_cast<float>(sides);
                    vertices.push_back({ radius * std::sin(angle), radius * std::cos(angle), static_cast<float>(j) * height / 2.0f });
                }
            }
            std::vector<BuilderPoly> list;
            for (int i = 0; i < sides; i++) {
                list.push_back({ { i * 2, i * 2 + 1, (i * 2 + 3) % (2 * sides), (i * 2 + 2) % (2 * sides) }, 1, Name("Wall"), 0 });
            }
            for (int j = -1; j < 2; j += 2) {
                BuilderPoly cap;
                cap.direction = j;
                cap.item = Name("Cap");
                for (int i = 0; i < sides; i++) {
                    cap.indices.push_back(i * 2 + (1 - j) / 2);
                }
                list.push_back(cap);
            }
            finishPolys(polys, vertices, list, texture);
        }

        void buildSheet(UPolys& polys, float width, float height, UObject* texture)
        {
            float x = width * 0.5f;
            float y = height * 0.5f;
            std::vector<Vector> vertices = { { x, y, 0 }, { x, -y, 0 }, { -x, -y, 0 }, { -x, y, 0 } };
            finishPolys(polys, vertices, { { { 0, 1, 2, 3 }, 1, Name("Sheet"), PolyNotSolid | PolyTwoSided } }, texture);
        }

        UClass* nativeClass(const char* name)
        {
            UClass* cls = objects::findClass(std::string("Engine.") + name);
            if (!cls) {
                cls = objects::intrinsicClass(Name(name));
            }
            return cls;
        }

        UObject* addBrush(ULevel* level, UObject* builder, uint8_t csgOper, uint32_t polyFlags)
        {
            if (!level || !builder) {
                return nullptr;
            }
            UObject* package = level->outermost();
            UClass* brushClass = objects::findClass("Engine.Brush");
            nativeClass("Model");
            nativeClass("Polys");
            objects::flushLoads();
            if (!brushClass) {
                return nullptr;
            }
            uint32_t editorOnly = RF_Transactional | LoadFlags | RF_NotForClient | RF_NotForServer;
            UModel* model = copyBrushModel(builder, package, editorOnly);
            if (!model) {
                return nullptr;
            }
            UObject* actorObject = objects::newObject(brushClass, package, Name(), editorOnly);
            if (!actorObject) {
                return nullptr;
            }
            actorObject->loaded = true;
            brush::PolyFlags(actorObject) = static_cast<int32_t>(polyFlags);
            copyPosRotScale(actorObject, builder);
            brush::CsgOper(actorObject) = csgOper;
            actor::Brush(actorObject) = model;
            actor::Level(actorObject) = level->actors.empty() ? nullptr : level->actors[0];
            level->actors.push_back(actorObject);
            if (level->model) {
                bsp::ensureLevelPolys(*level->model);
                bsp::brushCSG(actorObject, *level->model, polyFlags, csgOper, true);
            }
            return actorObject;
        }

        UObject* addMover(ULevel* level, UObject* builder, UClass* moverClass)
        {
            if (!level || !builder) {
                return nullptr;
            }
            UClass* base = objects::findClass("Engine.Mover");
            if (!moverClass || !base || !moverClass->isChildOf(base)) {
                moverClass = base;
            }
            if (!moverClass) {
                return nullptr;
            }
            UObject* package = level->outermost();
            UModel* model = copyBrushModel(builder, package, RF_Transactional | LoadFlags);
            if (!model) {
                return nullptr;
            }
            UObject* mover = objects::newObject(moverClass, package, Name(), RF_Transactional | LoadFlags);
            if (!mover) {
                return nullptr;
            }
            mover->loaded = true;
            actor::Location(mover) = actor::Location(builder);
            mover::BasePos(mover) = actor::Location(mover);
            mover::BaseRot(mover) = actor::Rotation(mover);
            brush::PolyFlags(mover) = 0;
            actor::Brush(mover) = model;
            copyPosRotScale(mover, builder);
            bsp::prepMovingBrush(mover);
            actor::Level(mover) = level->actors.empty() ? nullptr : level->actors[0];
            uint8_t& keyNum = mover::KeyNum(mover);
            keyNum = static_cast<uint8_t>(std::clamp<int>(keyNum, 0, 7));
            Rotator rotation = actor::Rotation(mover);
            Rotator oldRotation = mover::OldRot(mover);
            mover::BasePos(mover) = actor::Location(mover) - mover::OldPos(mover);
            mover::BaseRot(mover) = { rotation.pitch - oldRotation.pitch, rotation.yaw - oldRotation.yaw, rotation.roll - oldRotation.roll };
            mover::OldPos(mover) = mover::KeyPos(mover, keyNum);
            mover::OldRot(mover) = mover::KeyRot(mover, keyNum);
            actor::Location(mover) = mover::BasePos(mover) + mover::OldPos(mover);
            Rotator baseRot = mover::BaseRot(mover);
            Rotator keyRot = mover::OldRot(mover);
            actor::Rotation(mover) = { baseRot.pitch + keyRot.pitch, baseRot.yaw + keyRot.yaw, baseRot.roll + keyRot.roll };
            if (keyNum == 0) {
                rotation = actor::Rotation(mover);
                oldRotation = mover::OldRot(mover);
                mover::BasePos(mover) = actor::Location(mover) - mover::OldPos(mover);
                mover::BaseRot(mover) = { rotation.pitch - oldRotation.pitch, rotation.yaw - oldRotation.yaw, rotation.roll - oldRotation.roll };
            } else {
                Rotator baseRotation = mover::BaseRot(mover);
                Rotator firstRotation = mover::KeyRot(mover, 0);
                rotation = actor::Rotation(mover);
                mover::KeyPos(mover, keyNum) = actor::Location(mover) - (mover::BasePos(mover) + mover::KeyPos(mover, 0));
                mover::KeyRot(mover, keyNum) = { rotation.pitch - (baseRotation.pitch + firstRotation.pitch), rotation.yaw - (baseRotation.yaw + firstRotation.yaw), rotation.roll - (baseRotation.roll + firstRotation.roll) };
                mover::OldPos(mover) = mover::KeyPos(mover, keyNum);
                mover::OldRot(mover) = mover::KeyRot(mover, keyNum);
            }
            actor::Location(mover) = mover::BasePos(mover) + mover::KeyPos(mover, keyNum);
            level->actors.push_back(mover);
            return mover;
        }

        bool brushFromIntersection(ULevel* level, UObject* builder, bool deintersect)
        {
            if (!level || !level->model || !builder) {
                return false;
            }
            UModel* brush = cast<UModel>(actor::Brush(builder));
            if (!brush || !brush->polys) {
                return false;
            }
            bsp::brushCSG(builder, *level->model, 0, deintersect ? CsgDeintersect : CsgIntersect, false);
            return true;
        }

        Stats rebuild(ULevel* level, const RebuildOptions& options)
        {
            Stats stats;
            if (!level || !level->model) {
                return stats;
            }
            if (options.geometry) {
                stats = bsp::csgRebuild(level);
            }
            if (options.bsp) {
                size_t brushes = stats.brushes;
                stats = bsp::bspRebuild(level, options.optimization, options.balance, options.zones, options.optimizeGeometry);
                stats.brushes = brushes;
            }
            return stats;
        }

        bool pointSolid(const UModel& model, const Vector& point)
        {
            if (model.nodes.empty()) {
                return model.rootOutside == 0;
            }
            bool outside = model.rootOutside != 0;
            int32_t index = 0;
            while (index >= 0) {
                const BspNode& node = model.nodes[static_cast<size_t>(index)];
                bool front = node.plane.x * point.x + node.plane.y * point.y + node.plane.z * point.z - node.plane.w >= 0.0f;
                bool csgNode = bsp::isCsg(node, 0);
                outside = front ? (outside || csgNode) : (outside && !csgNode);
                index = front ? node.front : node.back;
            }
            return !outside;
        }

        int32_t pointZone(const UModel& model, const Vector& point)
        {
            return bsp::pointLeafZone(model, point, nullptr);
        }
    }
}
