#include "engine/map_writer.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <unordered_set>

namespace dx {
    namespace mapwriter {
        namespace {
            constexpr uint32_t RF_LoadFlags = 0x00070000;
            constexpr uint32_t PersistentFlags = RF_Transactional | RF_Public | RF_LoadFlags | RF_Standalone | RF_NotForClient | RF_NotForServer | RF_NotForEdit;

            void writeVector(Writer& writer, const Vector& v)
            {
                writer.f32(v.x);
                writer.f32(v.y);
                writer.f32(v.z);
            }

            void writeBox(Writer& writer, const BoundingBox& box)
            {
                writeVector(writer, box.min);
                writeVector(writer, box.max);
                writer.u8(box.valid ? 1 : 0);
            }

            int32_t usedZones(const UModel& model)
            {
                if (model.zones.empty()) {
                    return 0;
                }
                int32_t count = 0;
                for (size_t i = 0; i < model.zones.size(); i++) {
                    const ZoneProperties& zone = model.zones[i];
                    if (zone.zoneActor || zone.connectivity || zone.visibility) {
                        count = static_cast<int32_t>(i) + 1;
                    }
                }
                for (const ConvexLeaf& leaf : model.leaves) {
                    count = std::max(count, leaf.zone + 1);
                }
                for (const BspNode& node : model.nodes) {
                    count = std::max(count, static_cast<int32_t>(std::max(node.zone[0], node.zone[1])) + 1);
                }
                return std::min(count, static_cast<int32_t>(std::min<size_t>(model.zones.size(), 64)));
            }

            void visitReferences(UStruct* structType, uint8_t* data, const std::function<void(UObject*)>& visit)
            {
                if (!structType || !data) {
                    return;
                }
                for (UProperty* property : structType->properties) {
                    if (property->propertyFlags & (CPF_Transient | CPF_Native)) {
                        continue;
                    }
                    for (int32_t index = 0; index < property->arrayDim; index++) {
                        uint8_t* memory = data + property->offset + static_cast<size_t>(index) * property->elementSize();
                        if (property->type == PropType::Object || property->type == PropType::Class) {
                            visit(*reinterpret_cast<UObject**>(memory));
                        } else if (property->type == PropType::Struct) {
                            visitReferences(property->structType, memory, visit);
                        }
                    }
                }
            }

            void visitNativeReferences(UObject* object, const std::function<void(UObject*)>& visit)
            {
                if (ULevel* level = cast<ULevel>(object)) {
                    for (UObject* a : level->actors) {
                        visit(a);
                    }
                    visit(level->model);
                    for (const ReachSpec& spec : level->reachSpecs) {
                        visit(spec.start);
                        visit(spec.end);
                    }
                    for (UObject* block : level->textBlocks) {
                        visit(block);
                    }
                } else if (UModel* model = cast<UModel>(object)) {
                    visit(model->polys);
                    for (const BspSurf& surf : model->surfs) {
                        visit(surf.actor);
                    }
                    for (const ZoneProperties& zone : model->zones) {
                        visit(zone.zoneActor);
                    }
                    for (UObject* light : model->lights) {
                        visit(light);
                    }
                } else if (UPolys* polys = cast<UPolys>(object)) {
                    for (const Poly& poly : polys->polys) {
                        visit(poly.actor);
                    }
                }
            }
        }

        void writePrimitive(Writer& writer, const UPrimitive& primitive)
        {
            writeBox(writer, primitive.bounds);
            writer.f32(primitive.sphere.x);
            writer.f32(primitive.sphere.y);
            writer.f32(primitive.sphere.z);
            writer.f32(primitive.sphere.w);
        }

        void writeModel(PackageWriter& package, const UModel& model, Writer& writer)
        {
            writePrimitive(writer, model);
            writer.index(static_cast<int32_t>(model.vectors.size()));
            for (const Vector& v : model.vectors) {
                writeVector(writer, v);
            }
            writer.index(static_cast<int32_t>(model.points.size()));
            for (const Vector& v : model.points) {
                writeVector(writer, v);
            }
            writer.index(static_cast<int32_t>(model.nodes.size()));
            for (const BspNode& node : model.nodes) {
                writer.f32(node.plane.x);
                writer.f32(node.plane.y);
                writer.f32(node.plane.z);
                writer.f32(node.plane.w);
                writer.u64(node.zoneMask);
                writer.u8(node.nodeFlags);
                writer.index(node.vertPool);
                writer.index(node.surf);
                writer.index(node.back);
                writer.index(node.front);
                writer.index(node.iPlane);
                writer.index(node.collisionBound);
                writer.index(node.renderBound);
                writer.index(node.zone[0]);
                writer.index(node.zone[1]);
                writer.u8(node.numVertices);
                writer.i32(node.leaf[0]);
                writer.i32(node.leaf[1]);
            }
            writer.index(static_cast<int32_t>(model.surfs.size()));
            for (const BspSurf& surf : model.surfs) {
                package.writeObject(writer, surf.texture);
                writer.u32(surf.polyFlags);
                writer.index(surf.pBase);
                writer.index(surf.vNormal);
                writer.index(surf.vTextureU);
                writer.index(surf.vTextureV);
                writer.index(surf.lightMap);
                writer.index(surf.brushPoly);
                writer.i16(surf.panU);
                writer.i16(surf.panV);
                package.writeObject(writer, surf.actor);
            }
            writer.index(static_cast<int32_t>(model.verts.size()));
            for (const BspVert& vert : model.verts) {
                writer.index(vert.vertex);
                writer.index(vert.side);
            }
            writer.i32(model.numSharedSides);
            int32_t zoneCount = usedZones(model);
            writer.i32(zoneCount);
            for (int32_t i = 0; i < zoneCount; i++) {
                const ZoneProperties& zone = model.zones[static_cast<size_t>(i)];
                package.writeObject(writer, zone.zoneActor);
                writer.u64(zone.connectivity);
                writer.u64(zone.visibility);
            }
            package.writeObject(writer, model.polys);
            writer.index(static_cast<int32_t>(model.lightMaps.size()));
            for (const LightMapIndex& entry : model.lightMaps) {
                writer.i32(entry.dataOffset);
                writeVector(writer, entry.pan);
                writer.index(entry.uClamp);
                writer.index(entry.vClamp);
                writer.f32(entry.uScale);
                writer.f32(entry.vScale);
                writer.i32(entry.lightActors);
            }
            writer.index(static_cast<int32_t>(model.lightBits.size()));
            writer.bytes(model.lightBits.data(), model.lightBits.size());
            writer.index(static_cast<int32_t>(model.nodeBounds.size()));
            for (const BoundingBox& box : model.nodeBounds) {
                writeBox(writer, box);
            }
            writer.index(static_cast<int32_t>(model.leafHulls.size()));
            for (int32_t hull : model.leafHulls) {
                writer.i32(hull);
            }
            writer.index(static_cast<int32_t>(model.leaves.size()));
            for (const ConvexLeaf& leaf : model.leaves) {
                writer.index(leaf.zone);
                writer.index(leaf.permeating);
                writer.index(leaf.volumetric);
                writer.u64(leaf.visibleZones);
            }
            writer.index(static_cast<int32_t>(model.lights.size()));
            for (UObject* light : model.lights) {
                package.writeObject(writer, light);
            }
            writer.i32(model.rootOutside);
            writer.i32(model.linked);
        }

        void writePolys(PackageWriter& package, const UPolys& polys, Writer& writer)
        {
            int32_t count = static_cast<int32_t>(polys.polys.size());
            writer.i32(count);
            writer.i32(count);
            for (const Poly& poly : polys.polys) {
                writer.index(static_cast<int32_t>(poly.vertices.size()));
                writeVector(writer, poly.base);
                writeVector(writer, poly.normal);
                writeVector(writer, poly.textureU);
                writeVector(writer, poly.textureV);
                for (const Vector& vertex : poly.vertices) {
                    writeVector(writer, vertex);
                }
                writer.u32(poly.polyFlags);
                package.writeObject(writer, poly.actor);
                package.writeObject(writer, poly.texture);
                package.writeName(writer, poly.itemName);
                writer.index(poly.link);
                writer.index(poly.brushPoly);
                writer.i16(poly.panU);
                writer.i16(poly.panV);
            }
        }

        void writeLevel(PackageWriter& package, const ULevel& level, Writer& writer)
        {
            std::vector<UObject*> kept;
            for (size_t i = 0; i < level.actors.size(); i++) {
                UObject* a = level.actors[i];
                bool alive = a && !a->destroyed && package.isExport(a);
                if (alive || i < 2) {
                    kept.push_back(alive ? a : nullptr);
                }
            }
            int32_t count = static_cast<int32_t>(kept.size());
            writer.i32(count);
            writer.i32(count);
            for (UObject* a : kept) {
                package.writeObject(writer, a);
            }
            writer.string("deusex");
            writer.string("");
            writer.string("Index.dx");
            writer.string("");
            writer.index(0);
            writer.i32(7790);
            writer.u32(1);
            package.writeObject(writer, level.model);
            std::vector<const ReachSpec*> specs;
            for (const ReachSpec& spec : level.reachSpecs) {
                specs.push_back(&spec);
            }
            writer.index(static_cast<int32_t>(specs.size()));
            for (const ReachSpec* spec : specs) {
                writer.i32(spec->distance);
                package.writeObject(writer, spec->start);
                package.writeObject(writer, spec->end);
                writer.i32(spec->collisionRadius);
                writer.i32(spec->collisionHeight);
                writer.i32(spec->reachFlags);
                writer.u8(spec->pruned);
            }
            writer.f32(level.timeSeconds);
            package.writeObject(writer, nullptr);
            for (UObject* block : level.textBlocks) {
                package.writeObject(writer, block);
            }
            writer.index(0);
        }

        bool saveMap(ULevel* level, const std::filesystem::path& file, std::string* error)
        {
            if (!level) {
                if (error) {
                    *error = "no level";
                }
                return false;
            }
            UObject* mapPackage = level->outermost();
            Linker* linker = level->linker;
            PackageWriter writer(Name(file.stem().string()));
            std::unordered_set<UObject*> liveActors;
            for (UObject* a : level->actors) {
                if (a && !a->destroyed) {
                    liveActors.insert(a);
                }
            }
            UClass* actorClass = objects::findClass("Engine.Actor");
            auto belongs = [&](UObject* object) {
                if (!object || object->destroyed || object == mapPackage || object->outermost() != mapPackage) {
                    return false;
                }
                if (actorClass && object->isA(actorClass) && !liveActors.count(object)) {
                    return false;
                }
                return true;
            };
            std::deque<UObject*> queue;
            auto add = [&](UObject* object) {
                if (!belongs(object) || writer.isExport(object)) {
                    return;
                }
                objects::ensureLoaded(object);
                writer.addExport(object, object->outer == mapPackage ? nullptr : object->outer);
                queue.push_back(object);
            };
            add(level);
            if (linker && linker->packageObject == mapPackage) {
                size_t count = linker->file().exports().size();
                for (size_t i = 0; i < count; i++) {
                    add(linker->object(static_cast<int32_t>(i + 1)));
                }
            }
            objects::flushLoads();
            while (!queue.empty()) {
                UObject* object = queue.front();
                queue.pop_front();
                if (object->outer != mapPackage) {
                    add(object->outer);
                }
                visitReferences(object->cls, object->data, add);
                visitNativeReferences(object, add);
            }
            writer.setFilter([&](UObject* object) {
                if (!object->outer) {
                    return object != mapPackage;
                }
                return object->outermost() != mapPackage && (object->linker || cast<UClass>(object));
            });
            writer.setFlagSelector([&](UObject* object) {
                uint32_t flags = object->flags & PersistentFlags;
                if (!(flags & RF_LoadFlags)) {
                    flags |= RF_LoadFlags;
                }
                if (object == level) {
                    flags |= RF_Transactional;
                }
                return flags;
            });
            writer.setNativeSerializer([&](PackageWriter& out, UObject* object, Writer& data) {
                if (ULevel* asLevel = cast<ULevel>(object)) {
                    writeLevel(out, *asLevel, data);
                } else if (UModel* model = cast<UModel>(object)) {
                    writeModel(out, *model, data);
                } else if (UPolys* polys = cast<UPolys>(object)) {
                    writePolys(out, *polys, data);
                } else if (UPrimitive* primitive = cast<UPrimitive>(object)) {
                    writePrimitive(data, *primitive);
                } else if (UTextBuffer* text = cast<UTextBuffer>(object)) {
                    data.u32(0);
                    data.u32(0);
                    data.string(text->text);
                }
            });
            return writer.save(file, error);
        }
    }
}
