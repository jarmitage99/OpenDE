#pragma once

#include "engine/resources.h"

#include <string>
#include <string_view>
#include <vector>

namespace dx {
    namespace t3d {
        std::string exportActors(const std::vector<UObject*>& actors);
        std::vector<UObject*> importActors(ULevel* level, std::string_view text, bool replaceLevel);
    }
}
