#pragma once

#include "engine/resources.h"

#include <cstddef>
#include <unordered_set>

namespace dx {
    namespace lightbuild {
        struct Stats {
            size_t surfaces = 0;
            size_t lights = 0;
            size_t texels = 0;
            size_t lightLinks = 0;
        };

        Stats rebuild(ULevel* level, const std::unordered_set<UObject*>* selected = nullptr);
    }
}
