#include "engine/special_brush.h"

#include "engine/bsp.h"

namespace dx {
    namespace specialbrush {
        namespace {
            constexpr uint32_t PF_Translucent = 0x00000004;

            Preset preset(const char* name, const Options& options)
            {
                return { name, flags(options), options };
            }
        }

        uint32_t flags(const Options& options)
        {
            uint32_t result = 0;
            if (options.masked) {
                result |= bsp::PF_Masked;
            }
            if (options.transparent) {
                result |= PF_Translucent;
            }
            if (options.invisible) {
                result |= bsp::PF_Invisible;
            }
            if (options.zonePortal) {
                result |= bsp::PF_Portal;
            }
            if (options.solidity == Solidity::Semisolid) {
                result |= bsp::PF_Semisolid;
            } else if (options.solidity == Solidity::NotSolid) {
                result |= bsp::PF_NotSolid;
            }
            if (options.twoSided) {
                result |= bsp::PF_TwoSided;
            }
            return result;
        }

        std::vector<Preset> presets()
        {
            return {
                preset("Regular Brush", {}),
                preset("Invisible collision hull", { false, false, true, false, Solidity::Semisolid, false }),
                preset("Zone Portal", { false, false, true, true, Solidity::NotSolid, true }),
                preset("Semisolid Pillar", { false, false, false, false, Solidity::Semisolid, false }),
                preset("Transparent Window", { false, true, false, false, Solidity::NotSolid, false }),
                preset("Masked Decoration", { true, false, false, false, Solidity::NotSolid, false }),
                preset("Masked Wall", { true, false, false, false, Solidity::Semisolid, false }),
                preset("Water", { false, true, false, true, Solidity::NotSolid, true }),
            };
        }

        std::vector<Preset> advancedZones()
        {
            return {
                preset("Cutaway Zone", {}),
                preset("No-Terrain Zone", {}),
                preset("No-Cut Zone", {}),
            };
        }
    }
}
