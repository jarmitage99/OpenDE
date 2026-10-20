#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <set>
#include <string>
#include <string_view>

namespace dx {
    namespace editorops {
        enum class TexAlign {
            Default,
            Floor,
            WallDir,
            WallPan,
            OneTile,
        };

        void updateMaster(UModel& model, int32_t surf, bool updateTexCoords, bool updateBase);
        void setTexture(UModel& model, const std::set<int32_t>& surfs, UObject* texture);
        void setAndClearFlags(UModel& model, const std::set<int32_t>& surfs, uint32_t setBits, uint32_t clearBits);
        void texPan(UModel& model, const std::set<int32_t>& surfs, int panU, int panV, bool absolute);
        void texScale(UModel& model, const std::set<int32_t>& surfs, float uu, float uv, float vu, float vv, bool absolute);
        void texAlign(UModel& model, const std::set<int32_t>& surfs, TexAlign align);
        std::string exportPolys(const UPolys& polys);
        void importPolys(UPolys& polys, std::string_view text);
    }
}
