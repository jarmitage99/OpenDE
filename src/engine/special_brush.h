#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dx {
    namespace specialbrush {
        enum class Solidity {
            Solid,
            Semisolid,
            NotSolid,
        };

        struct Options {
            bool masked = false;
            bool transparent = false;
            bool invisible = false;
            bool zonePortal = false;
            Solidity solidity = Solidity::Solid;
            bool twoSided = false;
        };

        struct Preset {
            std::string name;
            uint32_t flags = 0;
            Options options;
        };

        uint32_t flags(const Options& options);
        std::vector<Preset> presets();
        std::vector<Preset> advancedZones();
    }
}
