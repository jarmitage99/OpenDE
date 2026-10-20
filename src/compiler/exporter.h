#pragma once

#include "core/object.h"

#include <string>

namespace dx::compiler {
    bool portable(const UProperty* property);
    bool identical(const UProperty* property, const uint8_t* a, const uint8_t* b);
    std::string exportTextItem(const UProperty* property, const uint8_t* value, const uint8_t* defaults, bool delimited);
    std::string exportProperties(const UStruct* structType, const uint8_t* data, const UClass* diffClass, const uint8_t* diff, int indent);
    std::string exportClass(UClass* cls);
}
