#pragma once

#include <cstdint>
#include <string>

namespace dx {
    constexpr int32_t TRAVEL_Absolute = 0;
    constexpr int32_t TRAVEL_Partial = 1;
    constexpr int32_t TRAVEL_Relative = 2;

    std::string travelUrl(const std::string& base, const std::string& text, int32_t travelType);
}
