#include "engine/brush_builder.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

namespace dx {
    namespace brushbuilder {
        namespace {
            struct State {
                std::vector<Vector> vertices;
                std::vector<BuilderPolygon> polys;
                Name group;
                bool mergeCoplanars = false;
            };

            std::unordered_map<UObject*, State>& states()
            {
                static std::unordered_map<UObject*, State> map;
                return map;
            }

            std::map<UClass*, UObject*>& instances()
            {
                static std::map<UClass*, UObject*> map;
                return map;
            }

            UObject*& target()
            {
                static UObject* brush = nullptr;
                return brush;
            }

            std::string& message()
            {
                static std::string text;
                return text;
            }

            float snapValue(float location, float grid)
            {
                if (grid == 0.0f) {
                    return location;
                }
                return std::floor((location + 0.5f * grid) / grid) * grid;
            }

            int32_t snapAngle(int32_t value, int32_t grid)
            {
                return static_cast<int32_t>(snapValue(static_cast<float>(value), static_cast<float>(grid)));
            }

            void nativeBeginBrush(UObject* self, Frame& stack, void*)
            {
                bool merge = vm::getBool(stack);
                Name group = vm::get<Name>(stack);
                vm::finish(stack);
                State& state = states()[self];
                state.mergeCoplanars = merge;
                state.group = group;
                state.vertices.clear();
                state.polys.clear();
            }

            void nativeEndBrush(UObject* self, Frame& stack, void* result)
            {
                vm::finish(stack);
                State& state = states()[self];
                vm::retBool(result, finishBrush(target(), state.vertices, state.polys, state.group, state.mergeCoplanars));
            }

            void nativeGetVertexCount(UObject* self, Frame& stack, void* result)
            {
                vm::finish(stack);
                vm::ret<int32_t>(result, static_cast<int32_t>(states()[self].vertices.size()));
            }

            void nativeGetVertex(UObject* self, Frame& stack, void* result)
            {
                int32_t i = vm::get<int32_t>(stack);
                vm::finish(stack);
                State& state = states()[self];
                Vector v;
                if (i >= 0 && static_cast<size_t>(i) < state.vertices.size()) {
                    v = state.vertices[static_cast<size_t>(i)];
                }
                vm::ret<Vector>(result, v);
            }

            void nativeGetPolyCount(UObject* self, Frame& stack, void* result)
            {
                vm::finish(stack);
                vm::ret<int32_t>(result, static_cast<int32_t>(states()[self].polys.size()));
            }

            void nativeBadParameters(UObject*, Frame& stack, void* result)
            {
                std::string text = vm::getOpt<std::string>(stack, std::string());
                vm::finish(stack);
                message() = text.empty() ? std::string("Bad parameters in brush builder") : text;
                vm::retBool(result, false);
            }

            void nativeVertexv(UObject* self, Frame& stack, void* result)
            {
                Vector v = vm::get<Vector>(stack);
                vm::finish(stack);
                State& state = states()[self];
                state.vertices.push_back(v);
                vm::ret<int32_t>(result, static_cast<int32_t>(state.vertices.size() - 1));
            }

            void nativeVertex3f(UObject* self, Frame& stack, void* result)
            {
                float x = vm::get<float>(stack);
                float y = vm::get<float>(stack);
                float z = vm::get<float>(stack);
                vm::finish(stack);
                State& state = states()[self];
                state.vertices.push_back({ x, y, z });
                vm::ret<int32_t>(result, static_cast<int32_t>(state.vertices.size() - 1));
            }

            void addPoly(UObject* self, int32_t direction, const std::vector<int32_t>& indices, Name item, int32_t flags)
            {
                BuilderPolygon poly;
                poly.direction = direction;
                poly.indices = indices;
                poly.item = item;
                poly.flags = flags;
                states()[self].polys.push_back(poly);
            }

            void nativePoly3i(UObject* self, Frame& stack, void*)
            {
                int32_t direction = vm::get<int32_t>(stack);
                int32_t i = vm::get<int32_t>(stack);
                int32_t j = vm::get<int32_t>(stack);
                int32_t k = vm::get<int32_t>(stack);
                Name item = vm::getOpt<Name>(stack, Name());
                int32_t flags = vm::getOpt<int32_t>(stack, 0);
                vm::finish(stack);
                addPoly(self, direction, { i, j, k }, item, flags);
            }

            void nativePoly4i(UObject* self, Frame& stack, void*)
            {
                int32_t direction = vm::get<int32_t>(stack);
                int32_t i = vm::get<int32_t>(stack);
                int32_t j = vm::get<int32_t>(stack);
                int32_t k = vm::get<int32_t>(stack);
                int32_t l = vm::get<int32_t>(stack);
                Name item = vm::getOpt<Name>(stack, Name());
                int32_t flags = vm::getOpt<int32_t>(stack, 0);
                vm::finish(stack);
                addPoly(self, direction, { i, j, k, l }, item, flags);
            }

            void nativePolyBegin(UObject* self, Frame& stack, void*)
            {
                int32_t direction = vm::get<int32_t>(stack);
                Name item = vm::getOpt<Name>(stack, Name());
                int32_t flags = vm::getOpt<int32_t>(stack, 0);
                vm::finish(stack);
                addPoly(self, direction, {}, item, flags);
            }

            void nativePolyi(UObject* self, Frame& stack, void*)
            {
                int32_t i = vm::get<int32_t>(stack);
                vm::finish(stack);
                State& state = states()[self];
                if (!state.polys.empty()) {
                    state.polys.back().indices.push_back(i);
                }
            }

            void nativePolyEnd(UObject*, Frame& stack, void*)
            {
                vm::finish(stack);
            }
        }

        bool finishBrush(UObject* brushActor, const std::vector<Vector>& vertices, std::vector<BuilderPolygon> polys, Name group, bool mergeCoplanars)
        {
            UModel* brush = brushActor ? cast<UModel>(actor::Brush(brushActor)) : nullptr;
            if (!brush || !brush->polys) {
                return false;
            }
            actor::Group(brushActor) = group;
            Constraints& c = constraints();
            Rotator& rotation = actor::Rotation(brushActor);
            if (c.rotGridEnabled) {
                rotation = { snapAngle(rotation.pitch, c.rotGridSize.pitch), snapAngle(rotation.yaw, c.rotGridSize.yaw), snapAngle(rotation.roll, c.rotGridSize.roll) };
            }
            Vector& location = actor::Location(brushActor);
            if (c.gridEnabled) {
                location = { snapValue(location.x, c.gridSize.x), snapValue(location.y, c.gridSize.y), snapValue(location.z, c.gridSize.z) };
            }
            bsp::ModelCoords coords;
            bsp::buildCoords(brushActor, &coords, nullptr);
            location -= bsp::transformVector(actor::PrePivot(brushActor), coords.pointXform);
            actor::PrePivot(brushActor) = {};
            brush->polys->polys.clear();
            for (BuilderPolygon& builderPoly : polys) {
                if (builderPoly.direction < 0) {
                    std::reverse(builderPoly.indices.begin(), builderPoly.indices.end());
                }
                for (;;) {
                    size_t count = std::min<size_t>(16, builderPoly.indices.size());
                    Poly poly;
                    poly.link = -1;
                    poly.brushPoly = -1;
                    poly.itemName = builderPoly.item;
                    if (!builderPoly.indices.empty()) {
                        poly.base = vertices[static_cast<size_t>(builderPoly.indices[0])];
                    }
                    poly.polyFlags = static_cast<uint32_t>(builderPoly.flags);
                    for (size_t j = 0; j < count; j++) {
                        poly.vertices.push_back(vertices[static_cast<size_t>(builderPoly.indices[j])]);
                    }
                    bsp::finalizePoly(poly);
                    brush->polys->polys.push_back(poly);
                    if (count == builderPoly.indices.size()) {
                        break;
                    }
                    builderPoly.indices.erase(builderPoly.indices.begin() + 1, builderPoly.indices.begin() + 1 + static_cast<std::ptrdiff_t>(count - 2));
                }
            }
            if (mergeCoplanars) {
                bsp::mergeCoplanars(*brush, false, true);
            }
            validateBrush(*brush, true);
            return true;
        }

        void registerNatives()
        {
            vm::registerNative("BrushBuilder", "BeginBrush", nativeBeginBrush);
            vm::registerNative("BrushBuilder", "EndBrush", nativeEndBrush);
            vm::registerNative("BrushBuilder", "GetVertexCount", nativeGetVertexCount);
            vm::registerNative("BrushBuilder", "GetVertex", nativeGetVertex);
            vm::registerNative("BrushBuilder", "GetPolyCount", nativeGetPolyCount);
            vm::registerNative("BrushBuilder", "BadParameters", nativeBadParameters);
            vm::registerNative("BrushBuilder", "Vertexv", nativeVertexv);
            vm::registerNative("BrushBuilder", "Vertex3f", nativeVertex3f);
            vm::registerNative("BrushBuilder", "Poly3i", nativePoly3i);
            vm::registerNative("BrushBuilder", "Poly4i", nativePoly4i);
            vm::registerNative("BrushBuilder", "PolyBegin", nativePolyBegin);
            vm::registerNative("BrushBuilder", "Polyi", nativePolyi);
            vm::registerNative("BrushBuilder", "PolyEnd", nativePolyEnd);
        }

        void setTarget(UObject* builderBrush)
        {
            target() = builderBrush;
        }

        Constraints& constraints()
        {
            static Constraints value;
            return value;
        }

        std::vector<UClass*> builderClasses()
        {
            std::vector<UClass*> result;
            static bool registered = false;
            if (!registered) {
                registerNatives();
                registered = true;
            }
            if (!objects::loadPackage(Name("Editor"))) {
                return result;
            }
            objects::flushLoads();
            UClass* base = objects::findClass("Editor.BrushBuilder");
            const char* names[] = { "CubeBuilder", "CylinderBuilder", "ConeBuilder", "SheetBuilder", "TetrahedronBuilder" };
            for (const char* name : names) {
                UClass* cls = objects::findClass(std::string("Editor.") + name);
                if (cls && base && cls->isChildOf(base)) {
                    result.push_back(cls);
                }
            }
            return result;
        }

        UObject* builderFor(UClass* cls)
        {
            if (!cls) {
                return nullptr;
            }
            auto found = instances().find(cls);
            if (found != instances().end()) {
                return found->second;
            }
            UObject* object = objects::newObject(cls, objects::packageObject(Name("Transient")), Name(), 0);
            if (object) {
                object->loaded = true;
            }
            instances()[cls] = object;
            return object;
        }

        bool build(UObject* builder)
        {
            message().clear();
            if (!builder) {
                return false;
            }
            vm::Call call(builder, Name("Build"));
            if (!call.valid()) {
                return false;
            }
            call.run();
            uint8_t* value = call.returnValue();
            return value && *reinterpret_cast<uint32_t*>(value) != 0;
        }

        std::string lastMessage()
        {
            return message();
        }

        void validateBrush(UModel& brush, bool force)
        {
            if (brush.polys && (force || !brush.linked)) {
                brush.linked = 1;
                std::vector<Poly>& polys = brush.polys->polys;
                for (size_t i = 0; i < polys.size(); i++) {
                    polys[i].link = static_cast<int32_t>(i);
                }
                for (size_t i = 0; i < polys.size(); i++) {
                    Poly& edPoly = polys[i];
                    if (edPoly.link != static_cast<int32_t>(i)) {
                        continue;
                    }
                    for (size_t j = i + 1; j < polys.size(); j++) {
                        Poly& other = polys[j];
                        if (other.link == static_cast<int32_t>(j) && other.texture == edPoly.texture && other.normal.dot(edPoly.normal) > 0.9999f) {
                            float dist = (other.base - edPoly.base).dot(edPoly.normal);
                            if (dist > -0.001f && dist < 0.001f) {
                                other.link = static_cast<int32_t>(i);
                            }
                        }
                    }
                }
            }
            bsp::buildBound(brush);
        }
    }
}
