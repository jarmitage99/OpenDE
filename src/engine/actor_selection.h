#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace dx {
    namespace actorselect {
        std::vector<UObject*> visible(const std::vector<UObject*>& actors);
        std::vector<UObject*> ofClass(const std::vector<UObject*>& actors, const UClass* cls);
        std::vector<UObject*> inverted(const std::vector<UObject*>& actors, const std::unordered_set<UObject*>& selection, const UObject* builder);
        std::vector<UObject*> inBox(const std::vector<UObject*>& actors, int horizontal, int vertical, float h0, float v0, float h1, float v1);
        std::vector<UObject*> toUnhide(const std::vector<UObject*>& actors);
        UClass* commonClass(const std::vector<UObject*>& actors);
        bool commonText(const std::vector<UObject*>& actors, const UProperty* property, int32_t index, std::string& text);
        bool setText(const std::vector<UObject*>& actors, const UProperty* property, int32_t index, std::string_view text);
        bool clipZ(Vector& a, Vector& b, float zMin, float zMax);
        Rotator rotatorFromAxes(const Coords& axes);
        Rotator rotationDelta(const Rotator& from, const Rotator& to);
    }
}
