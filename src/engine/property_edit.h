#pragma once

#include "core/object.h"

#include <array>
#include <vector>

namespace dx {
    class ULevel;

    namespace propedit {
        bool accepts(const UProperty* property, UObject* value);
        UObject* pickCurrent(const UProperty* property, const std::vector<UObject*>& candidates);
        bool assignObject(const UProperty* property, uint8_t* memory, UObject* value);
        bool isColor(const UProperty* property);
        std::array<float, 4> colorValue(const UProperty* property, const uint8_t* memory);
        void setColor(const UProperty* property, uint8_t* memory, const std::array<float, 4>& color);
        std::vector<UObject*> levelPropertyObjects(ULevel* level);
    }
}
