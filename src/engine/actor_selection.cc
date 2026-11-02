#include "engine/actor_selection.h"

#include "engine/actor.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>

namespace dx {
    namespace actorselect {
        namespace {
            float axis(const Vector& v, int index)
            {
                if (index == 0) {
                    return v.x;
                }
                if (index == 1) {
                    return v.y;
                }
                return v.z;
            }

            int32_t units(float radians)
            {
                return static_cast<int32_t>(std::lround(radians * 32768.0f / Pi));
            }

            int32_t wrapped(int32_t value)
            {
                return static_cast<int16_t>(static_cast<uint16_t>(value & 65535));
            }
        }

        std::vector<UObject*> visible(const std::vector<UObject*>& actors)
        {
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                if (!actor::bHiddenEd(actor)) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        std::vector<UObject*> ofClass(const std::vector<UObject*>& actors, const UClass* cls)
        {
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                if (actor->cls == cls && !actor::bHiddenEd(actor)) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        std::vector<UObject*> inverted(const std::vector<UObject*>& actors, const std::unordered_set<UObject*>& selection, const UObject* builder)
        {
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                if (actor != builder && !actor::bHiddenEd(actor) && !selection.count(actor)) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        std::vector<UObject*> inBox(const std::vector<UObject*>& actors, int horizontal, int vertical, float h0, float v0, float h1, float v1)
        {
            float minH = std::min(h0, h1);
            float maxH = std::max(h0, h1);
            float minV = std::min(v0, v1);
            float maxV = std::max(v0, v1);
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                const Vector& location = actor::Location(actor);
                float h = axis(location, horizontal);
                float v = axis(location, vertical);
                if (!actor::bHiddenEd(actor) && h >= minH && h <= maxH && v >= minV && v <= maxV) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        std::vector<UObject*> toUnhide(const std::vector<UObject*>& actors)
        {
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                UObject* defaults = actor->cls->defaultObject;
                if (actor::bHiddenEd(actor) && (!defaults || !actor::bHiddenEd(defaults))) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        UClass* commonClass(const std::vector<UObject*>& actors)
        {
            if (actors.empty()) {
                return nullptr;
            }
            UClass* common = actors.front()->cls;
            for (UObject* actor : actors) {
                while (common && !actor->cls->isChildOf(common)) {
                    common = common->superClass();
                }
            }
            return common;
        }

        bool commonText(const std::vector<UObject*>& actors, const UProperty* property, int32_t index, std::string& text)
        {
            size_t at = property->offset + static_cast<size_t>(index) * property->elementSize();
            for (UObject* actor : actors) {
                if (!property->equals(actor->data + at, actors.front()->data + at)) {
                    text.clear();
                    return false;
                }
            }
            text = actors.empty() ? std::string() : property->toText(actors.front()->data + at);
            return true;
        }

        bool setText(const std::vector<UObject*>& actors, const UProperty* property, int32_t index, std::string_view text)
        {
            size_t at = property->offset + static_cast<size_t>(index) * property->elementSize();
            bool done = !actors.empty();
            for (UObject* actor : actors) {
                done = property->fromText(actor->data + at, text) && done;
            }
            return done;
        }

        bool clipZ(Vector& a, Vector& b, float zMin, float zMax)
        {
            if ((a.z < zMin && b.z < zMin) || (a.z > zMax && b.z > zMax)) {
                return false;
            }
            Vector start = a;
            Vector delta = b - a;
            auto at = [&](float z) {
                float t = (z - start.z) / delta.z;
                return start + delta * t;
            };
            if (a.z < zMin) {
                a = at(zMin);
            } else if (a.z > zMax) {
                a = at(zMax);
            }
            if (b.z < zMin) {
                b = at(zMin);
            } else if (b.z > zMax) {
                b = at(zMax);
            }
            return true;
        }

        Rotator rotatorFromAxes(const Coords& axes)
        {
            const Vector& x = axes.xAxis;
            Rotator result;
            result.pitch = units(std::atan2(x.z, std::sqrt(x.x * x.x + x.y * x.y)));
            result.yaw = units(std::atan2(x.y, x.x));
            result.roll = units(std::atan2(-axes.yAxis.z, axes.zAxis.z));
            return result;
        }

        Rotator rotationDelta(const Rotator& from, const Rotator& to)
        {
            return { wrapped(to.pitch - from.pitch), wrapped(to.yaw - from.yaw), wrapped(to.roll - from.roll) };
        }
    }
}
